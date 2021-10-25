/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
#include <boost/optional.hpp>

#include "ReconstructMethodByPlateId.h"

#include "PlateVelocityUtils.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructMethodFiniteRotation.h"
#include "ReconstructParams.h"
#include "ReconstructUtils.h"
#include "TopologyReconstructedFeatureGeometry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/FiniteRotation.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * The transform used to reconstruct by plate id.
		 */
		class Transform :
				public ReconstructMethodFiniteRotation
		{
		public:
			// Convenience typedefs for a shared pointer to a @a Transform.
			typedef GPlatesUtils::non_null_intrusive_ptr<Transform> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Transform> non_null_ptr_to_const_type;


			//! Create a transform if have a reconstruction plate id.
			static
			non_null_ptr_type
			create(
					const GPlatesMaths::FiniteRotation &finite_rotation,
					GPlatesModel::integer_plate_id_type reconstruction_plate_id)
			{
				return non_null_ptr_type(new Transform(finite_rotation, reconstruction_plate_id));
			}

			//! Create an identity transform if do *not* have a reconstruction plate id.
			static
			non_null_ptr_type
			create()
			{
				return non_null_ptr_type(new Transform());
			}

		private:
			boost::optional<GPlatesModel::integer_plate_id_type> d_reconstruction_plate_id;

			Transform(
					const GPlatesMaths::FiniteRotation &finite_rotation,
					GPlatesModel::integer_plate_id_type reconstruction_plate_id) :
				ReconstructMethodFiniteRotation(ReconstructMethod::BY_PLATE_ID, finite_rotation),
				d_reconstruction_plate_id(reconstruction_plate_id)
			{  }

			Transform() :
				ReconstructMethodFiniteRotation(
						ReconstructMethod::BY_PLATE_ID,
						// Create the identify rotation...
						GPlatesMaths::FiniteRotation::create_identity_rotation())
			{  }

			virtual
			bool
			less_than_compare_finite_rotation_parameters(
					const ReconstructMethodFiniteRotation &rhs) const
			{
				// Comparing the plate id is a lot faster than comparing the finite rotation.
				return d_reconstruction_plate_id <
					// See 'operator<' in base class for why we can cast...
					dynamic_cast<const Transform &>(rhs).d_reconstruction_plate_id;
			}
		};


		/**
		 * Used to determine if can reconstruct a feature.
		 */
		class CanReconstructFeature :
				public GPlatesModel::ConstFeatureVisitor
		{
		public:
			CanReconstructFeature() :
				d_can_reconstruct(false),
				d_has_geometry(false)
			{  }

			//! Returns true any features visited by us can be reconstructed.
			bool
			can_reconstruct()
			{
				return d_can_reconstruct;
			}

		private:

			virtual
			bool
			initialise_pre_feature_properties(
					const GPlatesModel::FeatureHandle &feature_handle)
			{
				const GPlatesModel::FeatureHandle::const_weak_ref feature_ref = feature_handle.reference();

				// Feature must have a plate id or a BY_PLATE_ID reconstruct method property.
				// We are lenient here because a lot of features have a reconstruction plate id
				// but don't have 'ReconstructMethod::BY_PLATE_ID' specified.
				//
				// Update: In fact we're now even more lenient and only require geometry.
				// Some features have no plate id in which case we can still rotate them using
				// the identity rotation so they stay in their present day positions.
				// This leniency should not interfere with other reconstruct methods because we,
				// enumeration ReconstructMethod::BY_PLATE_ID, are listed first in the enumeration
				// sequence which also lists least specialised to most specialised reconstruct
				// methods and so we are the least specialised and also get queried last - so if
				// there are any more specialised methods then they will have precedence.
				//
				// NOTE: A regular, *non-topological* geometry is required here so this won't
				// pick up topological features (which are handled by a different framework).

#if 0
				// Firstly find a reconstruction plate ID and reconstruct method.
				ReconstructionFeatureProperties reconstruction_params;
				reconstruction_params.visit_feature(feature_ref);

				if (!reconstruction_params.get_recon_plate_id() &&
					reconstruction_params.get_reconstruction_method() != ReconstructMethod::BY_PLATE_ID)
				{
					return false;
				}
#endif

				d_has_geometry = false;

				return true;
			}

			virtual
			void
			finalise_post_feature_properties(
					const GPlatesModel::FeatureHandle &feature_handle)
			{
				if (d_has_geometry)
				{
					d_can_reconstruct = true;
				}
			}

			virtual
			void
			visit_gml_line_string(
					const GPlatesPropertyValues::GmlLineString &gml_line_string)
			{
				d_has_geometry = true;
			}

			virtual
			void
			visit_gml_multi_point(
					const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				d_has_geometry = true;
			}

			virtual
			void
			visit_gml_orientable_curve(
					const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
			{
				d_has_geometry = true;
			}

			virtual
			void
			visit_gml_point(
					const GPlatesPropertyValues::GmlPoint &gml_point)
			{
				d_has_geometry = true;
			}
			
			virtual
			void
			visit_gml_polygon(
					const GPlatesPropertyValues::GmlPolygon &gml_polygon)
			{
				d_has_geometry = true;
			}

			virtual
			void
			visit_gpml_constant_value(
					const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}


			bool d_can_reconstruct;

			bool d_has_geometry;
		};


		/**
		 * Finds the present day geometries of a feature.
		 */
		class GetPresentDayGeometries :
				public GPlatesModel::FeatureVisitor
		{
		public:
			GetPresentDayGeometries(
					std::vector<ReconstructMethodInterface::Geometry> &present_day_geometries) :
				d_present_day_geometries(present_day_geometries)
			{  }

		private:

			virtual
			void
			visit_gml_line_string(
					GPlatesPropertyValues::GmlLineString &gml_line_string)
			{
				d_present_day_geometries.push_back(
						ReconstructMethodInterface::Geometry(
								*current_top_level_propiter(),
								gml_line_string.polyline()));
			}

			virtual
			void
			visit_gml_multi_point(
					GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				d_present_day_geometries.push_back(
						ReconstructMethodInterface::Geometry(
								*current_top_level_propiter(),
								gml_multi_point.multipoint()));
			}

			virtual
			void
			visit_gml_orientable_curve(
					GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
			{
				gml_orientable_curve.base_curve()->accept_visitor(*this);
			}

			virtual
			void
			visit_gml_point(
					GPlatesPropertyValues::GmlPoint &gml_point)
			{
				d_present_day_geometries.push_back(
						ReconstructMethodInterface::Geometry(
								*current_top_level_propiter(),
								gml_point.point()));
			}
			
			virtual
			void
			visit_gml_polygon(
					GPlatesPropertyValues::GmlPolygon &gml_polygon)
			{
				d_present_day_geometries.push_back(
						ReconstructMethodInterface::Geometry(
								*current_top_level_propiter(),
								gml_polygon.polygon()));
			}

			virtual
			void
			visit_gpml_constant_value(
					GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}


			std::vector<ReconstructMethodInterface::Geometry> &d_present_day_geometries;
		};
	}
}


bool
GPlatesAppLogic::ReconstructMethodByPlateId::can_reconstruct_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref)
{
	CanReconstructFeature can_reconstruct_feature;
	can_reconstruct_feature.visit_feature(feature_weak_ref);

	return can_reconstruct_feature.can_reconstruct();
}


GPlatesAppLogic::ReconstructMethodByPlateId::ReconstructMethodByPlateId(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const Context &context) :
	ReconstructMethodInterface(ReconstructMethod::BY_PLATE_ID, feature_ref)
{
}


void
GPlatesAppLogic::ReconstructMethodByPlateId::get_present_day_feature_geometries(
		std::vector<Geometry> &present_day_geometries) const
{
	// Cache present day geometries (if not already).
	if (!d_present_day_geometries)
	{
		d_present_day_geometries = std::vector<Geometry>();

		GetPresentDayGeometries visitor(d_present_day_geometries.get());
		visitor.visit_feature(get_feature_ref());
	}

	// Copy to caller's sequence.
	present_day_geometries.insert(
			present_day_geometries.end(),
			d_present_day_geometries->begin(),
			d_present_day_geometries->end());
}


void
GPlatesAppLogic::ReconstructMethodByPlateId::reconstruct_feature_geometries(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const ReconstructHandle::type &reconstruct_handle,
		const Context &context,
		const double &reconstruction_time)
{
	boost::optional<const topology_reconstructed_geometry_time_span_sequence_type &>
			topology_reconstructed_geometry_time_spans = get_topology_reconstruction_info(context);
	if (topology_reconstructed_geometry_time_spans)
	{
		// We have topology reconstructed geometries.

		const ReconstructionInfo &reconstruction_info = get_reconstruction_info(context);

		// The feature must be defined at the reconstruction time.
		if (!reconstruction_info.valid_time.is_valid_at_recon_time(reconstruction_time))
		{
			return;
		}

		// Output an RFG for each geometry property in the feature.
		BOOST_FOREACH(
				const TopologyReconstructedGeometryTimeSpan &topology_reconstructed_geometry_time_span,
				topology_reconstructed_geometry_time_spans.get())
		{
			// If the geometry has not been subducted/consumed at the reconstruction time then
			// create a topology reconstructed feature geometry.
			if (topology_reconstructed_geometry_time_span.geometry_time_span->is_valid(reconstruction_time))
			{
				const TopologyReconstructedFeatureGeometry::non_null_ptr_type topology_reconstructed_feature_geometry =
						TopologyReconstructedFeatureGeometry::create(
								context.reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time),
								context.reconstruction_tree_creator,
								*get_feature_ref(),
								topology_reconstructed_geometry_time_span.property_iterator,
								topology_reconstructed_geometry_time_span.geometry_time_span,
								reconstruction_info.reconstruction_plate_id,
								reconstruction_info.valid_time.time_of_appearance,
								reconstruct_handle);
				reconstructed_feature_geometries.push_back(topology_reconstructed_feature_geometry);
			}
		}

		return;
	}

	//
	// We don't have topology reconstructed geometries so reconstruct using rigid rotations by plate id.
	//

	const ReconstructionInfo &reconstruction_info = get_reconstruction_info(context);

	// The feature must be defined at the reconstruction time, *unless* we've been requested to
	// reconstruct for all times (even times when the feature is not defined).
	if (!context.reconstruct_params.get_reconstruct_by_plate_id_outside_active_time_period())
	{
		if (!reconstruction_info.valid_time.is_valid_at_recon_time(reconstruction_time))
		{
			return;
		}
	}

	// Get the reconstruction tree for the current reconstruction time.
	const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			context.reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);

	// We obtained the reconstruction plate ID.  We now have all the information we
	// need to reconstruct according to the reconstruction plate ID.
	const Transform::non_null_ptr_type reconstruction_rotation =
			Transform::create(
					reconstruction_tree->get_composed_absolute_rotation(reconstruction_info.reconstruction_plate_id).first,
					reconstruction_info.reconstruction_plate_id);

	// Iterate over the feature's present day geometries and rotate each one.
	std::vector<Geometry> present_day_geometries;
	get_present_day_feature_geometries(present_day_geometries);
	BOOST_FOREACH(const Geometry &present_day_geometry, present_day_geometries)
	{
		const ReconstructedFeatureGeometry::non_null_ptr_type rigid_rfg =
				ReconstructedFeatureGeometry::create(
						reconstruction_tree,
						context.reconstruction_tree_creator,
						*get_feature_ref(),
						present_day_geometry.property_iterator,
						present_day_geometry.geometry,
						reconstruction_rotation.get(),
						ReconstructMethod::BY_PLATE_ID,
						reconstruction_info.reconstruction_plate_id,
						reconstruction_info.valid_time.time_of_appearance,
						reconstruct_handle);
		reconstructed_feature_geometries.push_back(rigid_rfg);
	}
}


void
GPlatesAppLogic::ReconstructMethodByPlateId::reconstruct_feature_velocities(
		std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
		const ReconstructHandle::type &reconstruct_handle,
		const Context &context,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type)
{
	//PROFILE_FUNC();

	// If we don't have topology reconstructed geometries then reconstruct using rigid rotations by plate id.
	boost::optional<const topology_reconstructed_geometry_time_span_sequence_type &>
			topology_reconstructed_geometry_time_spans = get_topology_reconstruction_info(context);
	if (!topology_reconstructed_geometry_time_spans)
	{
		reconstruct_feature_velocities_by_plate_id(
				reconstructed_feature_velocities,
				reconstruct_handle,
				context,
				reconstruction_time,
				velocity_delta_time,
				velocity_delta_time_type);

		return;
	}

	const ReconstructionInfo &reconstruction_info = get_reconstruction_info(context);

	// We have topology reconstructed geometries.

	// Should not be able to have topology reconstructed geometries without a topology reconstruct context.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			context.topology_reconstruct,
			GPLATES_ASSERTION_SOURCE);

	// The feature must be defined at the reconstruction time.
	if (!reconstruction_info.valid_time.is_valid_at_recon_time(reconstruction_time))
	{
		return;
	}

	// Output a multi-point velocity vector field for each geometry property in the feature.
	BOOST_FOREACH(
			const TopologyReconstructedGeometryTimeSpan &topology_reconstructed_geometry_time_span,
			topology_reconstructed_geometry_time_spans.get())
	{
		// Calculate the velocities at the topology reconstructed geometry (domain) points.
		std::vector<GPlatesMaths::PointOnSphere> domain_points;
		std::vector<GPlatesMaths::Vector3D> velocities;
		std::vector< boost::optional<const ReconstructionGeometry *> > surfaces;
		if (!topology_reconstructed_geometry_time_span.geometry_time_span->get_velocities(
				domain_points,
				velocities,
				surfaces,
				reconstruction_time,
				velocity_delta_time,
				velocity_delta_time_type))
		{
			// The geometry has been subducted/consumed at the reconstruction time so there are no domain points.
			continue;
		}

		// Create a multi-point-on-sphere with the domain points.
		const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type domain_multi_point_geometry =
				GPlatesMaths::MultiPointOnSphere::create_on_heap(domain_points);

		// Create an RFG purely for the purpose of representing this feature.
		// This is only needed when/if a domain point is outside all resolved networks.
		// This is required in order for the velocity arrows to be coloured correctly -
		// because the colouring code requires a reconstruction geometry (it will then
		// lookup the plate ID or other feature property(s) depending on the colour scheme).
		const ReconstructedFeatureGeometry::non_null_ptr_type rigid_rfg =
				ReconstructedFeatureGeometry::create(
						context.reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time),
						context.reconstruction_tree_creator,
						*get_feature_ref(),
						topology_reconstructed_geometry_time_span.property_iterator,
						domain_multi_point_geometry,
						ReconstructMethod::BY_PLATE_ID,
						reconstruction_info.reconstruction_plate_id,
						reconstruction_info.valid_time.time_of_appearance,
						reconstruct_handle);

		const MultiPointVectorField::non_null_ptr_type vector_field =
				MultiPointVectorField::create_empty(
						reconstruction_time,
						domain_multi_point_geometry,
						*get_feature_ref(),
						topology_reconstructed_geometry_time_span.property_iterator,
						reconstruct_handle);

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				domain_points.size() == velocities.size() &&
					domain_points.size() == surfaces.size(),
				GPLATES_ASSERTION_SOURCE);

		// Set the velocities in the multi-point vector field.
		std::vector<GPlatesMaths::Vector3D>::const_iterator velocities_iter = velocities.begin();
		std::vector< boost::optional<const ReconstructionGeometry *> >::const_iterator surfaces_iter = surfaces.begin();
		MultiPointVectorField::codomain_type::iterator field_iter = vector_field->begin();
		MultiPointVectorField::codomain_type::iterator field_end = vector_field->end();
		for ( ; field_iter != field_end; ++field_iter, ++velocities_iter, ++surfaces_iter)
		{
			// Set the velocity and determine the codomain reason, plate id and reconstruction geometry.
			boost::optional<const ReconstructionGeometry *> surface = *surfaces_iter;
			if (surface)
			{
				*field_iter = MultiPointVectorField::CodomainElement(
						*velocities_iter,
						// Determine if point was in the deforming region or interior rigid block of network.
						// It's either a resolved topological network or a reconstructed feature geometry...
						ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
								const ResolvedTopologicalNetwork *>(surface.get())
							? MultiPointVectorField::CodomainElement::InNetworkDeformingRegion
							: MultiPointVectorField::CodomainElement::InNetworkRigidBlock,
						ReconstructionGeometryUtils::get_plate_id(surface.get()),
						surface.get());
			}
			else // rigid rotation...
			{
				*field_iter = MultiPointVectorField::CodomainElement(
						*velocities_iter,
						MultiPointVectorField::CodomainElement::ReconstructedDomainPoint,
						reconstruction_info.reconstruction_plate_id,
						rigid_rfg.get());
			}
		}

		reconstructed_feature_velocities.push_back(vector_field);
	}
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructMethodByPlateId::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const Context &context,
		const double &reconstruction_time,
		bool reverse_reconstruct)
{
	ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			context.reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);

	const ReconstructionInfo &reconstruction_info = get_reconstruction_info(context);

	// We obtained the reconstruction plate ID so reconstruct (or reverse reconstruct) the geometry.
	//
	// Note that we don't reconstruct using topologies (even if we have topologies).
	return ReconstructUtils::reconstruct_by_plate_id(
			geometry,
			reconstruction_info.reconstruction_plate_id,
			*reconstruction_tree,
			reverse_reconstruct);
}


void
GPlatesAppLogic::ReconstructMethodByPlateId::get_topology_reconstructed_geometry_time_spans(
		topology_reconstructed_geometry_time_span_sequence_type &topology_reconstructed_geometry_time_spans,
		const Context &context)
{
	boost::optional<const topology_reconstructed_geometry_time_span_sequence_type &> geometry_time_spans =
			get_topology_reconstruction_info(context);
	if (geometry_time_spans)
	{
		topology_reconstructed_geometry_time_spans.insert(
				topology_reconstructed_geometry_time_spans.end(),
				geometry_time_spans->begin(),
				geometry_time_spans->end());
	}
}


const GPlatesAppLogic::ReconstructMethodByPlateId::ReconstructionInfo &
GPlatesAppLogic::ReconstructMethodByPlateId::get_reconstruction_info(
		const Context &context) const
{
	if (!d_reconstruction_info)
	{
		d_reconstruction_info = ReconstructionInfo();

		// Get the feature's reconstruction plate id and begin/end time.
		ReconstructionFeatureProperties reconstruction_feature_properties;
		reconstruction_feature_properties.visit_feature(get_feature_ref());

		// If we can't get a reconstruction plate ID then we'll just use plate id zero (spin axis).
		if (reconstruction_feature_properties.get_recon_plate_id())
		{
			d_reconstruction_info->reconstruction_plate_id = reconstruction_feature_properties.get_recon_plate_id().get();
		}

		d_reconstruction_info->valid_time = reconstruction_feature_properties.get_valid_time();

		// The geometry import time uses the time-of-appearance if user requested to override.
		// Otherwise use geometry import time property (if available), otherwise defaults to zero.
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> geometry_import_time;
		if (context.reconstruct_params.get_topology_reconstruction_use_time_of_appearance())
		{
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_appearance =
					reconstruction_feature_properties.get_time_of_appearance();
			// Can only set the geometry import time if it's a real number (not distant past or future).
			if (time_of_appearance &&
				time_of_appearance->is_real())
			{
				geometry_import_time = time_of_appearance;
			}
		}
		if (!geometry_import_time)
		{
			geometry_import_time = reconstruction_feature_properties.get_geometry_import_time();
		}

		// Can only set the geometry import time if it's a real number (not distant past or future).
		// The geometry import time (defaults to zero if not available).
		if (geometry_import_time &&
			geometry_import_time->is_real())
		{
			d_reconstruction_info->geometry_import_time = geometry_import_time->value();
		}
	}

	return d_reconstruction_info.get();
}


boost::optional<const GPlatesAppLogic::ReconstructMethodByPlateId::topology_reconstructed_geometry_time_span_sequence_type &>
GPlatesAppLogic::ReconstructMethodByPlateId::get_topology_reconstruction_info(
		const Context &context) const
{
	if (!context.topology_reconstruct)
	{
		// There's no reconstruction using topologies.
		return boost::none;
	}

	if (!d_topology_reconstructed_geometry_time_spans)
	{
		d_topology_reconstructed_geometry_time_spans = topology_reconstructed_geometry_time_span_sequence_type();

		const ReconstructionInfo &reconstruction_info = get_reconstruction_info(context);

		// Tessellation of polylines/polygons.
		boost::optional<double> line_tessellation_radians;
		if (context.reconstruct_params.get_topology_reconstruction_enable_line_tessellation())
		{
			// Convert degrees to radians.
			line_tessellation_radians = GPlatesMaths::convert_deg_to_rad(
					context.reconstruct_params.get_topology_reconstruction_line_tessellation_degrees());
		}

		// Lifetime detection of individual points in the reconstructed/deformed geometries.
		boost::optional<TopologyReconstruct::ActivePointParameters> active_point_parameters;
		if (context.reconstruct_params.get_topology_reconstruction_enable_lifetime_detection())
		{
			active_point_parameters = TopologyReconstruct::ActivePointParameters(
					context.reconstruct_params.get_topology_reconstruction_lifetime_detection_threshold_velocity_delta(),
					context.reconstruct_params.get_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary());
		}

		// Use natural neighbour coordinates when deforming points in topological networks.
		const bool deformation_use_natural_neighbour_interpolation =
				context.reconstruct_params.get_topology_deformation_use_natural_neighbour_interpolation();

		// Iterate over the feature's present day geometries and generate a topology reconstructed geometry
		// time span for each geometry.
		std::vector<Geometry> present_day_geometries;
		get_present_day_feature_geometries(present_day_geometries);
		BOOST_FOREACH(const Geometry &present_day_geometry, present_day_geometries)
		{
			const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type topology_reconstructed_geometry_time_span =
					context.topology_reconstruct.get()->create_geometry_time_span(
							present_day_geometry.geometry,
							reconstruction_info.reconstruction_plate_id,
							reconstruction_info.geometry_import_time,
							line_tessellation_radians,
							active_point_parameters,
							deformation_use_natural_neighbour_interpolation);

			d_topology_reconstructed_geometry_time_spans->push_back(
					TopologyReconstructedGeometryTimeSpan(
							present_day_geometry.property_iterator,
							topology_reconstructed_geometry_time_span));
		}
	}

	return d_topology_reconstructed_geometry_time_spans.get();
}
