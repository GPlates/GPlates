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

#include <boost/optional.hpp>

#include "ReconstructMethodByPlateId.h"

#include "PlateVelocityUtils.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructMethodFiniteRotation.h"
#include "ReconstructParams.h"
#include "ReconstructUtils.h"

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
						GPlatesMaths::FiniteRotation::create(
								GPlatesMaths::UnitQuaternion3D::create_identity_rotation(),
								boost::none))
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
		 * Reconstructs a feature using its present day geometry and its plate Id.
		 */
		class ReconstructFeature :
				public GPlatesModel::FeatureVisitorThatGuaranteesNotToModify
		{
		public:
			explicit
			ReconstructFeature(
					std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
					const ReconstructHandle::type &reconstruct_handle,
					const ReconstructParams &reconstruct_params,
					const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree) :
				d_reconstruct_handle(reconstruct_handle),
				d_reconstruct_params(reconstruct_params),
				d_reconstruction_tree(reconstruction_tree),
				d_reconstruction_params(reconstruction_tree->get_reconstruction_time()),
				d_reconstructed_feature_geometries(reconstructed_feature_geometries)
			{  }

		protected:
			bool
			initialise_pre_feature_properties(
					GPlatesModel::FeatureHandle &feature_handle)
			{
				//PROFILE_FUNC();

				const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature_handle.reference();

				// Firstly find the reconstruction plate ID.
				d_reconstruction_params.visit_feature(feature_ref);

				// Secondly the feature must be defined at the reconstruction time, *unless* we've been
				// requested to reconstruct for all times (even times when the feature is not defined).
				if (!d_reconstruct_params.get_reconstruct_by_plate_id_outside_active_time_period() &&
					!d_reconstruction_params.is_feature_defined_at_recon_time())
				{
					// Don't reconstruct.
					return false;
				}

				// If we can't get a reconstruction plate ID then we'll just reconstruct using the
				// identity rotation - we may need to make this an option - some users might not want to
				// see a reconstructed geometry if a feature has no plate ID.
				if (d_reconstruction_params.get_recon_plate_id())
				{
					const GPlatesModel::integer_plate_id_type reconstruction_plate_id =
							d_reconstruction_params.get_recon_plate_id().get();

					// We obtained the reconstruction plate ID.  We now have all the information we
					// need to reconstruct according to the reconstruction plate ID.
					d_reconstruction_rotation =
							Transform::create(
									d_reconstruction_tree->get_composed_absolute_rotation(
											reconstruction_plate_id).first,
									reconstruction_plate_id);
				}
				else
				{
					// Create the identity transform.
					d_reconstruction_rotation = Transform::create();
				}

				// A temporary hack to get around the problem of rotating MeshNodes where velocities
				// are calculated when they have a zero plate ID but the anchor plate ID is
				// *not* zero - causing a non-identity rotation.
				// FIXME: We now also allow non-MeshNode features containing multi-point geometry
				// to form the domain for velocity calculations and these will still have this
				// problem. This will get fixed when the velocity layer is made to draw the
				// points at the base of the arrows as well as the velocity arrows themselves.
				if (PlateVelocityUtils::detect_velocity_mesh_node(feature_ref))
				{
					// Create the identity transform.
					d_reconstruction_rotation = Transform::create();
				}

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
								*property.handle_weak_ref(),
								property,
								gml_line_string.polyline(),
								d_reconstruction_rotation.get(),
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
								*property.handle_weak_ref(),
								property,
								gml_multi_point.multipoint(),
								d_reconstruction_rotation.get(),
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
								*property.handle_weak_ref(),
								property,
								gml_point.point(),
								d_reconstruction_rotation.get(),
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
								*property.handle_weak_ref(),
								property,
								gml_polygon.exterior(),
								d_reconstruction_rotation.get(),
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
									*property.handle_weak_ref(),
									property,
									polygon_interior,
									d_reconstruction_rotation.get(),
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
			const ReconstructParams &d_reconstruct_params;
			ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;
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
GPlatesAppLogic::ReconstructMethodByPlateId::can_reconstruct_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref)
{
	CanReconstructFeature can_reconstruct_feature;
	can_reconstruct_feature.visit_feature(feature_weak_ref);

	return can_reconstruct_feature.can_reconstruct();
}


void
GPlatesAppLogic::ReconstructMethodByPlateId::get_present_day_geometries(
		std::vector<Geometry> &present_day_geometries,
		const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref) const
{
	GetPresentDayGeometries visitor(present_day_geometries);

	visitor.visit_feature(feature_weak_ref);
}


void
GPlatesAppLogic::ReconstructMethodByPlateId::reconstruct_feature(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref,
		const ReconstructHandle::type &reconstruct_handle,
		const ReconstructParams &reconstruct_params,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time)
{
	//PROFILE_FUNC();

	// Get the reconstruction tree for the reconstruction time.
	const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);

	ReconstructFeature recon_feature(
			reconstructed_feature_geometries, reconstruct_handle, reconstruct_params, reconstruction_tree);

	recon_feature.visit_feature(feature_weak_ref);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructMethodByPlateId::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		bool reverse_reconstruct)
{
	// Get the values of the properties at present day.
	ReconstructionFeatureProperties reconstruction_feature_properties(0/*reconstruction_time*/);

	reconstruction_feature_properties.visit_feature(reconstruction_properties);

	// If we found a reconstruction plate ID then reconstruct (or reverse reconstruct the geometry).
	if (reconstruction_feature_properties.get_recon_plate_id())
	{
		ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
				reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);

		return ReconstructUtils::reconstruct_by_plate_id(
				geometry,
				reconstruction_feature_properties.get_recon_plate_id().get(),
				*reconstruction_tree,
				reverse_reconstruct);
	}

	// Otherwise just return the original geometry.
	return geometry;
}
