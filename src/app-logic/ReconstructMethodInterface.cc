/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <boost/foreach.hpp>

#include "ReconstructMethodInterface.h"

#include "GeometryUtils.h"
#include "PlateVelocityUtils.h"
#include "ReconstructionFeatureProperties.h"

#include "maths/CalculateVelocity.h"

#include "model/types.h"


void
GPlatesAppLogic::ReconstructMethodInterface::reconstruct_feature_velocities_by_plate_id(
		std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
		const ReconstructHandle::type &reconstruct_handle,
		const Context &context,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type)
{
	// Get the feature's reconstruction plate id and begin/end time.
	ReconstructionFeatureProperties reconstruction_feature_properties;
	reconstruction_feature_properties.visit_feature(get_feature_ref());

	// The feature must be defined at the reconstruction time, *unless* we've been requested to
	// reconstruct for all times (even times when the feature is not defined - but we only do
	// this for rigid rotations since it affects geometry positioning when deformation is present).
	if (!context.reconstruct_params.get_reconstruct_by_plate_id_outside_active_time_period() &&
		!reconstruction_feature_properties.is_feature_defined_at_recon_time(reconstruction_time))
	{
		return;
	}

	// If we can't get a reconstruction plate ID then we'll just use plate id zero (spin axis)
	// which can still give a non-identity rotation if the anchor plate id is non-zero.
	GPlatesModel::integer_plate_id_type reconstruction_plate_id = 0;
	if (reconstruction_feature_properties.get_recon_plate_id())
	{
		reconstruction_plate_id = reconstruction_feature_properties.get_recon_plate_id().get();
	}

	const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			context.reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);
	const GPlatesMaths::FiniteRotation &finite_rotation =
			reconstruction_tree->get_composed_absolute_rotation(reconstruction_plate_id).first;

	const std::pair<double, double> time_range = VelocityDeltaTime::get_time_range(
			velocity_delta_time_type, reconstruction_time, velocity_delta_time);

	// Iterate over the feature's present day geometries and rotate each one.
	std::vector<Geometry> present_day_geometries;
	get_present_day_feature_geometries(present_day_geometries);
	BOOST_FOREACH(const Geometry &present_day_geometry, present_day_geometries)
	{
		// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
		// that stores a multi-point domain and a corresponding velocity field but the
		// geometry property iterator (referenced by the MultiPointVectorField) could be a
		// non-multi-point geometry.
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
				GeometryUtils::convert_geometry_to_multi_point(*present_day_geometry.geometry);

		// Rotate the velocity domain.
		// We do this even if the plate id is zero because the anchor plate might be non-zero.
		velocity_domain = finite_rotation * velocity_domain;

		// Create an RFG purely for the purpose of representing the feature that generated the
		// plate ID (ie, this feature).
		// This is required in order for the velocity arrows to be coloured correctly -
		// because the colouring code requires a reconstruction geometry (it will then
		// lookup the plate ID or other feature property(s) depending on the colour scheme).
		const ReconstructedFeatureGeometry::non_null_ptr_type plate_id_rfg =
				ReconstructedFeatureGeometry::create(
						reconstruction_tree,
						context.reconstruction_tree_creator,
						*get_feature_ref(),
						present_day_geometry.property_iterator,
						velocity_domain,
						ReconstructMethod::BY_PLATE_ID,
						reconstruction_plate_id,
						reconstruction_feature_properties.get_time_of_appearance(),
						reconstruct_handle);

		GPlatesMaths::MultiPointOnSphere::const_iterator domain_iter = velocity_domain->begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator domain_end = velocity_domain->end();

		MultiPointVectorField::non_null_ptr_type vector_field =
				MultiPointVectorField::create_empty(
						reconstruction_time,
						velocity_domain,
						*get_feature_ref(),
						present_day_geometry.property_iterator,
						reconstruct_handle);
		MultiPointVectorField::codomain_type::iterator field_iter = vector_field->begin();

		// Iterate over the domain points and calculate their velocities.
		for ( ; domain_iter != domain_end; ++domain_iter, ++field_iter)
		{
			// Calculate the velocity.
			const GPlatesMaths::Vector3D vector_xyz =
					PlateVelocityUtils::calculate_velocity_vector(
							*domain_iter,
							context.reconstruction_tree_creator,
							time_range.second/*young*/,
							time_range.first/*old*/,
							reconstruction_plate_id);

			*field_iter = MultiPointVectorField::CodomainElement(
					vector_xyz,
					MultiPointVectorField::CodomainElement::ReconstructedDomainPoint,
					reconstruction_plate_id,
					ReconstructionGeometry::maybe_null_ptr_to_const_type(plate_id_rfg.get()));
		}

		reconstructed_feature_velocities.push_back(vector_field);
	}
}
