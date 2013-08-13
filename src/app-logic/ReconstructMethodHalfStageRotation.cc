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

#include "ReconstructMethodHalfStageRotation.h"

#include "GeometryUtils.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructMethodFiniteRotation.h"
#include "ReconstructUtils.h"

#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"
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


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * The transform used to reconstruct by half-stage-rotation of left/right plate ids.
		 */
		class Transform :
				public ReconstructMethodFiniteRotation
		{
		public:
			// Convenience typedefs for a shared pointer to a @a Transform.
			typedef GPlatesUtils::non_null_intrusive_ptr<Transform> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Transform> non_null_ptr_to_const_type;


			//! Create a transform if have a left/right plate ids.
			static
			non_null_ptr_type
			create(
					const GPlatesMaths::FiniteRotation &finite_rotation,
					GPlatesModel::integer_plate_id_type left_plate_id,
					GPlatesModel::integer_plate_id_type right_plate_id)
			{
				return non_null_ptr_type(new Transform(finite_rotation, left_plate_id, right_plate_id));
			}

			//! Create an identity transform if do *not* have a left/right plate ids.
			static
			non_null_ptr_type
			create()
			{
				return non_null_ptr_type(new Transform());
			}

		private:
			boost::optional<GPlatesModel::integer_plate_id_type> d_left_plate_id;
			boost::optional<GPlatesModel::integer_plate_id_type> d_right_plate_id;


			Transform(
					const GPlatesMaths::FiniteRotation &finite_rotation,
					GPlatesModel::integer_plate_id_type left_plate_id,
					GPlatesModel::integer_plate_id_type right_plate_id) :
				ReconstructMethodFiniteRotation(ReconstructMethod::HALF_STAGE_ROTATION, finite_rotation),
				d_left_plate_id(left_plate_id),
				d_right_plate_id(right_plate_id)
			{  }

			Transform() :
				ReconstructMethodFiniteRotation(
						ReconstructMethod::HALF_STAGE_ROTATION,
						// Create the identify rotation...
						GPlatesMaths::FiniteRotation::create(
								GPlatesMaths::UnitQuaternion3D::create_identity_rotation(),
								boost::none))
			{  }

			virtual
			bool
			less_than_compare_finite_rotation_parameters(
					const ReconstructMethodFiniteRotation &rhs_base) const
			{
				// Comparing the plate ids is a lot faster than comparing the finite rotation.
				// See 'operator<' in base class for why we can cast...
				const Transform &rhs = dynamic_cast<const Transform &>(rhs_base);

				if (d_left_plate_id < rhs.d_left_plate_id)
				{
					return true;
				}

				if (d_left_plate_id == rhs.d_left_plate_id)
				{
					return d_right_plate_id < rhs.d_right_plate_id;
				}

				return false;
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

				// Firstly find left/right plate IDs and reconstruct method.
				ReconstructionFeatureProperties reconstruction_params;
				reconstruction_params.visit_feature(feature_ref);

				// Must have the correct reconstruct method property and left/right plate ids.
				if (reconstruction_params.get_reconstruction_method() != ReconstructMethod::HALF_STAGE_ROTATION ||
					!reconstruction_params.get_left_plate_id() ||
					!reconstruction_params.get_right_plate_id())
				{
					return false;
				}

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
				// TODO: Add interior polygons when PolygonOnSphere contains interior polygons.
				d_present_day_geometries.push_back(
						ReconstructMethodInterface::Geometry(
								*current_top_level_propiter(),
								gml_polygon.exterior()));
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


		/**
		 * Reconstructs a feature using its present day geometry and left/right plate Ids.
		 */
		class ReconstructFeature :
				public GPlatesModel::FeatureVisitor
		{
		public:
			explicit
			ReconstructFeature(
					std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
					const ReconstructHandle::type &reconstruct_handle,
					const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
					const ReconstructionTreeCreator &reconstruction_tree_creator) :
				d_reconstruct_handle(reconstruct_handle),
				d_reconstruction_tree(reconstruction_tree),
				d_reconstruction_tree_creator(reconstruction_tree_creator),
				d_reconstruction_params(reconstruction_tree->get_reconstruction_time()),
				d_reconstructed_feature_geometries(reconstructed_feature_geometries)
			{  }

		protected:
			bool
			initialise_pre_feature_properties(
					GPlatesModel::FeatureHandle &feature_handle)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature_handle.reference();

				// Firstly find the left/right plate IDs.
				d_reconstruction_params.visit_feature(feature_ref);

				// Secondly the feature must be defined at the reconstruction time.
				if (!d_reconstruction_params.is_feature_defined_at_recon_time())
				{
					// Don't reconstruct.
					return false;
				}

				// If we can't get left/right plate IDs then we'll just use plate id zero (spin axis)
				// which can still give a non-identity rotation if the anchor plate id is non-zero.
				GPlatesModel::integer_plate_id_type left_plate_id = 0;
				GPlatesModel::integer_plate_id_type right_plate_id = 0;
				if (d_reconstruction_params.get_left_plate_id())
				{
					left_plate_id = d_reconstruction_params.get_left_plate_id().get();
				}
				if (d_reconstruction_params.get_right_plate_id())
				{
					right_plate_id = d_reconstruction_params.get_right_plate_id().get();
				}

				// Get the half-stage rotation.
				const GPlatesMaths::FiniteRotation finite_rotation =
						ReconstructUtils::get_half_stage_rotation(
								*d_reconstruction_tree,
								left_plate_id,
								right_plate_id);
				d_reconstruction_rotation = Transform::create(
						finite_rotation, left_plate_id, right_plate_id);

				// Now visit the feature to reconstruct any geometries we find.
				return true;
			}


			void
			visit_gml_line_string(
					GPlatesPropertyValues::GmlLineString &gml_line_string)
			{
				GPlatesModel::FeatureHandle::iterator property = *current_top_level_propiter();

				const ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
						ReconstructedFeatureGeometry::create(
								d_reconstruction_tree,
								d_reconstruction_tree_creator,
								*property.handle_weak_ref(),
								property,
								gml_line_string.polyline(),
								d_reconstruction_rotation.get(),
								ReconstructMethod::HALF_STAGE_ROTATION,
								d_reconstruction_params.get_recon_plate_id(),
								d_reconstruction_params.get_time_of_appearance(),
								d_reconstruct_handle);
				d_reconstructed_feature_geometries.push_back(rfg_ptr);
			}


			void
			visit_gml_multi_point(
					GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				GPlatesModel::FeatureHandle::iterator property = *current_top_level_propiter();

				const ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
						ReconstructedFeatureGeometry::create(
								d_reconstruction_tree,
								d_reconstruction_tree_creator,
								*property.handle_weak_ref(),
								property,
								gml_multi_point.multipoint(),
								d_reconstruction_rotation.get(),
								ReconstructMethod::HALF_STAGE_ROTATION,
								d_reconstruction_params.get_recon_plate_id(),
								d_reconstruction_params.get_time_of_appearance(),
								d_reconstruct_handle);
				d_reconstructed_feature_geometries.push_back(rfg_ptr);
			}


			void
			visit_gml_orientable_curve(
					GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
			{
				gml_orientable_curve.base_curve()->accept_visitor(*this);
			}


			void
			visit_gml_point(
					GPlatesPropertyValues::GmlPoint &gml_point)
			{
				GPlatesModel::FeatureHandle::iterator property = *current_top_level_propiter();

				const ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
						ReconstructedFeatureGeometry::create(
								d_reconstruction_tree,
								d_reconstruction_tree_creator,
								*property.handle_weak_ref(),
								property,
								gml_point.point(),
								d_reconstruction_rotation.get(),
								ReconstructMethod::HALF_STAGE_ROTATION,
								d_reconstruction_params.get_recon_plate_id(),
								d_reconstruction_params.get_time_of_appearance(),
								d_reconstruct_handle);
				d_reconstructed_feature_geometries.push_back(rfg_ptr);
			}


			void
			visit_gml_polygon(
					GPlatesPropertyValues::GmlPolygon &gml_polygon)
			{
				GPlatesModel::FeatureHandle::iterator property = *current_top_level_propiter();

				const ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
						ReconstructedFeatureGeometry::create(
								d_reconstruction_tree,
								d_reconstruction_tree_creator,
								*property.handle_weak_ref(),
								property,
								gml_polygon.exterior(),
								d_reconstruction_rotation.get(),
								ReconstructMethod::HALF_STAGE_ROTATION,
								d_reconstruction_params.get_recon_plate_id(),
								d_reconstruction_params.get_time_of_appearance(),
								d_reconstruct_handle);
				d_reconstructed_feature_geometries.push_back(rfg_ptr);
					
				// Repeat the same procedure for each of the interior rings, if any.
				GPlatesPropertyValues::GmlPolygon::ring_const_iterator it = gml_polygon.interiors_begin();
				GPlatesPropertyValues::GmlPolygon::ring_const_iterator end = gml_polygon.interiors_end();
				for ( ; it != end; ++it) 
				{
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon_interior = *it;

					const ReconstructedFeatureGeometry::non_null_ptr_type rfg_p =
							ReconstructedFeatureGeometry::create(
									d_reconstruction_tree,
									d_reconstruction_tree_creator,
									*property.handle_weak_ref(),
									property,
									polygon_interior,
									d_reconstruction_rotation.get(),
									ReconstructMethod::HALF_STAGE_ROTATION,
									d_reconstruction_params.get_recon_plate_id(),
									d_reconstruction_params.get_time_of_appearance(),
									d_reconstruct_handle);
					d_reconstructed_feature_geometries.push_back(rfg_p);
				}
			}


			void
			visit_gpml_constant_value(
					GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}

		private:
			const ReconstructHandle::type &d_reconstruct_handle;
			ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;
			const ReconstructionTreeCreator &d_reconstruction_tree_creator;
			ReconstructionFeatureProperties d_reconstruction_params;
			boost::optional<Transform::non_null_ptr_type> d_reconstruction_rotation;


			/**
			 * The @a ReconstructedFeatureGeometry objects generated during reconstruction.
			 */
			std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &d_reconstructed_feature_geometries;
		};
	}
}


bool
GPlatesAppLogic::ReconstructMethodHalfStageRotation::can_reconstruct_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref)
{
	CanReconstructFeature can_reconstruct_feature;
	can_reconstruct_feature.visit_feature(feature_weak_ref);

	return can_reconstruct_feature.can_reconstruct();
}


void
GPlatesAppLogic::ReconstructMethodHalfStageRotation::get_present_day_feature_geometries(
		std::vector<Geometry> &present_day_geometries) const
{
	GetPresentDayGeometries visitor(present_day_geometries);

	visitor.visit_feature(get_feature_ref());
}


void
GPlatesAppLogic::ReconstructMethodHalfStageRotation::reconstruct_feature_geometries(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const ReconstructHandle::type &reconstruct_handle,
		const Context &context,
		const double &reconstruction_time)
{
	// Get the reconstruction tree for the reconstruction time.
	const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			context.reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);

	ReconstructFeature visitor(
			reconstructed_feature_geometries,
			reconstruct_handle,
			reconstruction_tree,
			context.reconstruction_tree_creator);

	visitor.visit_feature(get_feature_ref());
}


void
GPlatesAppLogic::ReconstructMethodHalfStageRotation::reconstruct_feature_velocities(
		std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
		const ReconstructHandle::type &reconstruct_handle,
		const Context &context,
		const double &reconstruction_time)
{
	// Get the feature's reconstruction left/right plate ids.
	ReconstructionFeatureProperties reconstruction_feature_properties(reconstruction_time);
	reconstruction_feature_properties.visit_feature(get_feature_ref());

	// The feature must be defined at the reconstruction time.
	if (!reconstruction_feature_properties.is_feature_defined_at_recon_time())
	{
		return;
	}

	// If we can't get left/right plate IDs then we'll just use plate id zero (spin axis)
	// which can still give a non-identity rotation if the anchor plate id is non-zero.
	GPlatesModel::integer_plate_id_type left_plate_id = 0;
	GPlatesModel::integer_plate_id_type right_plate_id = 0;
	if (reconstruction_feature_properties.get_left_plate_id())
	{
		left_plate_id = reconstruction_feature_properties.get_left_plate_id().get();
	}
	if (reconstruction_feature_properties.get_right_plate_id())
	{
		right_plate_id = reconstruction_feature_properties.get_right_plate_id().get();
	}

	// Iterate over the feature's present day geometries and rotate each one.
	std::vector<Geometry> present_day_geometries;
	get_present_day_feature_geometries(present_day_geometries);
	BOOST_FOREACH(const Geometry &present_day_geometry, present_day_geometries)
	{
		const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
				context.reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);
		const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree_2 =
				context.reconstruction_tree_creator.get_reconstruction_tree(
						// FIXME:  Should this '1' should be user controllable? ...
						reconstruction_time + 1);

		// Get the half-stage rotation.
		const GPlatesMaths::FiniteRotation finite_rotation = ReconstructUtils::get_half_stage_rotation(
				*reconstruction_tree,
				left_plate_id,
				right_plate_id);
		const GPlatesMaths::FiniteRotation finite_rotation_2 = ReconstructUtils::get_half_stage_rotation(
				*reconstruction_tree_2,
				left_plate_id,
				right_plate_id);

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
						ReconstructMethod::HALF_STAGE_ROTATION,
						reconstruction_feature_properties.get_recon_plate_id(),
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
					GPlatesMaths::calculate_velocity_vector(
							*domain_iter,
							finite_rotation,
							finite_rotation_2);

			*field_iter = MultiPointVectorField::CodomainElement(
					vector_xyz,
					MultiPointVectorField::CodomainElement::ReconstructedDomainPoint,
					reconstruction_feature_properties.get_recon_plate_id(),
					ReconstructionGeometry::maybe_null_ptr_to_const_type(plate_id_rfg.get()));
		}

		reconstructed_feature_velocities.push_back(vector_field);
	}
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructMethodHalfStageRotation::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const Context &context,
		const double &reconstruction_time,
		bool reverse_reconstruct)
{
	// Get the values of the properties at present day.
	ReconstructionFeatureProperties reconstruction_feature_properties(0/*reconstruction_time*/);

	reconstruction_feature_properties.visit_feature(get_feature_ref());

	// If we can't get left/right plate IDs then we'll just use plate id zero (spin axis)
	// which can still give a non-identity rotation if the anchor plate id is non-zero.
	GPlatesModel::integer_plate_id_type left_plate_id = 0;
	GPlatesModel::integer_plate_id_type right_plate_id = 0;
	if (reconstruction_feature_properties.get_left_plate_id())
	{
		left_plate_id = reconstruction_feature_properties.get_left_plate_id().get();
	}
	if (reconstruction_feature_properties.get_right_plate_id())
	{
		right_plate_id = reconstruction_feature_properties.get_right_plate_id().get();
	}

	ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			context.reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);

	return ReconstructUtils::reconstruct_as_half_stage(
			geometry,
			left_plate_id,
			right_plate_id,
			*reconstruction_tree,
			reverse_reconstruct);
}
