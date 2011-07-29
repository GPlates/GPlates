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

#include "ReconstructMethodVirtualGeomagneticPole.h"

#include "ReconstructedVirtualGeomagneticPole.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructMethodFiniteRotation.h"
#include "ReconstructUtils.h"

#include "maths/FiniteRotation.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"


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
				ReconstructMethodFiniteRotation(ReconstructMethod::VIRTUAL_GEOMAGNETIC_POLE, finite_rotation),
				d_reconstruction_plate_id(reconstruction_plate_id)
			{  }

			Transform() :
				ReconstructMethodFiniteRotation(
						ReconstructMethod::VIRTUAL_GEOMAGNETIC_POLE,
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
				d_can_reconstruct(false)
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

				static const GPlatesModel::FeatureType paleomag_feature_type = 
						GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");

				if (feature_handle.feature_type() == paleomag_feature_type)
				{
					d_can_reconstruct = true;
				}

				// NOTE: We don't actually want to visit the feature's properties.
				return false;
			}

			bool d_can_reconstruct;
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
					const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
					const ReconstructParams &reconstruct_params) :
				d_reconstruction_tree(reconstruction_tree),
				d_reconstruct_params(reconstruct_params),
				d_reconstruction_params(reconstruction_tree->get_reconstruction_time()),
				d_reconstructed_feature_geometries(reconstructed_feature_geometries)
			{  }

		protected:
			bool
			initialise_pre_feature_properties(
					GPlatesModel::FeatureHandle &feature_handle)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature_handle.reference();

				// Firstly find the reconstruction plate ID.
				d_reconstruction_params.visit_feature(feature_ref);

				// Secondly the feature must be defined at the reconstruction time.
				if (!d_reconstruction_params.is_feature_defined_at_recon_time())
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

				// Now visit the feature to reconstruct any geometries we find.
				return true;
			}


			void
			finalise_post_feature_properties(
					GPlatesModel::FeatureHandle &feature_handle)
			{
				if (!d_reconstruct_params.should_draw_vgp(
						d_reconstruction_tree->get_reconstruction_time(),
						d_VGP_params.d_age))
				{
					return;
				}

				if (d_VGP_params.d_vgp_point)
				{
					ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
							ReconstructedVirtualGeomagneticPole::create(
									d_VGP_params,
									d_reconstruction_tree,
									(*d_VGP_params.d_vgp_point),
									*(*d_VGP_params.d_vgp_iterator).handle_weak_ref(),
									(*d_VGP_params.d_vgp_iterator),
									d_reconstruction_params.get_recon_plate_id(),
									d_reconstruction_params.get_time_of_appearance());
					d_reconstructed_feature_geometries.push_back(rfg_ptr);
				}

				if (d_VGP_params.d_site_point)
				{
					ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
							ReconstructedFeatureGeometry::create(
									d_reconstruction_tree,
									*(*d_VGP_params.d_site_iterator).handle_weak_ref(),
									(*d_VGP_params.d_site_iterator),
									(*d_VGP_params.d_site_point),
									d_reconstruction_params.get_recon_plate_id(),
									d_reconstruction_params.get_time_of_appearance());
					d_reconstructed_feature_geometries.push_back(rfg_ptr);
				}
			}


			void
			visit_gml_point(
					GPlatesPropertyValues::GmlPoint &gml_point)
			{
				static const GPlatesModel::PropertyName site_name = 
						GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition");
	
				static const GPlatesModel::PropertyName vgp_name =
						GPlatesModel::PropertyName::create_gpml("polePosition");

				GPlatesModel::FeatureHandle::iterator property = *current_top_level_propiter();

				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type reconstructed_point =
						d_reconstruction_rotation.get()->get_finite_rotation() * gml_point.point();

				if (current_top_level_propname() == site_name)
				{
					d_VGP_params.d_site_point = reconstructed_point;
					d_VGP_params.d_site_iterator = property;
				}
				else if (current_top_level_propname() == vgp_name)
				{
					d_VGP_params.d_vgp_point = reconstructed_point;
					d_VGP_params.d_vgp_iterator = property;
				}
			}

			void
			visit_xs_double(
					GPlatesPropertyValues::XsDouble &xs_double)
			{
				static const GPlatesModel::PropertyName a95_name = 
					GPlatesModel::PropertyName::create_gpml("poleA95");

				static const GPlatesModel::PropertyName dm_name = 
					GPlatesModel::PropertyName::create_gpml("poleDm");

				static const GPlatesModel::PropertyName dp_name = 
					GPlatesModel::PropertyName::create_gpml("poleDp");		

				static const GPlatesModel::PropertyName age_name = 
					GPlatesModel::PropertyName::create_gpml("averageAge");				

				if (current_top_level_propname() == a95_name)
				{
					d_VGP_params.d_a95 = xs_double.value();
				}
				else if (current_top_level_propname() == dm_name)
				{
					d_VGP_params.d_dm = xs_double.value();
				}
				else if (current_top_level_propname() == dp_name)
				{
					d_VGP_params.d_dp = xs_double.value();
				}
				else if (current_top_level_propname() == age_name)
				{
					d_VGP_params.d_age = xs_double.value();
				}
			}


			void
			visit_gpml_constant_value(
					GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}

		private:
			ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;
			const ReconstructParams &d_reconstruct_params;
			ReconstructionFeatureProperties d_reconstruction_params;
			ReconstructedVirtualGeomagneticPoleParams d_VGP_params;
			boost::optional<Transform::non_null_ptr_type> d_reconstruction_rotation;


			/**
			 * The @a ReconstructedFeatureGeometry objects generated during reconstruction.
			 */
			std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &d_reconstructed_feature_geometries;
		};
	}
}


bool
GPlatesAppLogic::ReconstructMethodVirtualGeomagneticPole::can_reconstruct_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref)
{
	CanReconstructFeature can_reconstruct_feature;
	can_reconstruct_feature.visit_feature(feature_weak_ref);

	return can_reconstruct_feature.can_reconstruct();
}


void
GPlatesAppLogic::ReconstructMethodVirtualGeomagneticPole::get_present_day_geometries(
		std::vector<Geometry> &present_day_geometries,
		const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref) const
{
	GetPresentDayGeometries visitor(present_day_geometries);

	visitor.visit_feature(feature_weak_ref);
}


void
GPlatesAppLogic::ReconstructMethodVirtualGeomagneticPole::reconstruct_feature(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref,
		const ReconstructHandle::type &/*reconstruct_handle*/,
		const ReconstructParams &reconstruct_params,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time)
{
	// Get the reconstruction tree for the reconstruction time.
	const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);

	ReconstructFeature visitor(reconstructed_feature_geometries, reconstruction_tree, reconstruct_params);

	visitor.visit_feature(feature_weak_ref);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructMethodVirtualGeomagneticPole::reconstruct_geometry(
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
