/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionGeometryVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/IntrusivePointerZeroRefCountException.h"

#include "model/WeakObserverVisitor.h"

#include "utils/Profile.h"


GPlatesAppLogic::ReconstructedFeatureGeometry::ReconstructedFeatureGeometry(
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
		GPlatesModel::FeatureHandle &feature_handle_,
		GPlatesModel::FeatureHandle::iterator property_iterator_,
		const geometry_ptr_type &reconstructed_geometry_,
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_,
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_) :
	ReconstructionGeometry(reconstruction_tree_),
	WeakObserverType(feature_handle_),
	d_property_iterator(property_iterator_),
	d_reconstructed_geometry(reconstructed_geometry_),
	d_reconstruction_plate_id(reconstruction_plate_id_),
	d_time_of_formation(time_of_formation_)
{
}


GPlatesAppLogic::ReconstructedFeatureGeometry::ReconstructedFeatureGeometry(
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
		GPlatesModel::FeatureHandle &feature_handle_,
		GPlatesModel::FeatureHandle::iterator property_iterator_,
		// NOTE: This is the *unreconstructed* geometry...
		const geometry_ptr_type &resolved_geometry_,
		const ReconstructMethodFiniteRotation::non_null_ptr_to_const_type &reconstruct_method_transform_,
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_,
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none) :
	ReconstructionGeometry(reconstruction_tree_),
	WeakObserverType(feature_handle_),
	d_property_iterator(property_iterator_),
	d_finite_rotation_reconstruction(
			FiniteRotationReconstruction(
					resolved_geometry_, reconstruct_method_transform_)),
	d_reconstruction_plate_id(reconstruction_plate_id_),
	d_time_of_formation(time_of_formation_)
{
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::ReconstructedFeatureGeometry::get_feature_ref() const
{
	if (is_valid()) {
		return feature_handle_ptr()->reference();
	} else {
		return GPlatesModel::FeatureHandle::weak_ref();
	}
}


const GPlatesAppLogic::ReconstructedFeatureGeometry::geometry_ptr_type &
GPlatesAppLogic::ReconstructedFeatureGeometry::reconstructed_geometry() const
{
	// If there's no reconstructed geometry then it means we have a reconstruction instead -
	// so reconstruct the resolved geometry now and cache the result.
	if (!d_reconstructed_geometry)
	{
		// We've set up our constructors so that if there's no reconstructed geometry then
		// there has to be a finite rotation reconstruction.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_finite_rotation_reconstruction,
				GPLATES_ASSERTION_SOURCE);

		//PROFILE_BLOCK("ReconstructedFeatureGeometry::reconstructed_geometry: transform geometry");

		d_reconstructed_geometry = d_finite_rotation_reconstruction->get_reconstructed_geometry();
	}

	return d_reconstructed_geometry.get();
}


void
GPlatesAppLogic::ReconstructedFeatureGeometry::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(this->get_non_null_pointer_to_const());
}


void
GPlatesAppLogic::ReconstructedFeatureGeometry::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(this->get_non_null_pointer());
}


void
GPlatesAppLogic::ReconstructedFeatureGeometry::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_reconstructed_feature_geometry(*this);
}
