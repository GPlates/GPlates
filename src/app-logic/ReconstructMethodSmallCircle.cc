/* $Id: ReconstructMethodVirtualGeomagneticPole.cc 11715 2011-06-07 09:30:12Z rwatson $ */

/**
 * \file 
 * $Revision: 11715 $
 * $Date: 2011-06-07 11:30:12 +0200 (ti, 07 jun 2011) $
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#include "ReconstructMethodSmallCircle.h"

#include "ReconstructedSmallCircle.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructUtils.h"
#include "SmallCircleGeometryPopulator.h"

#include "maths/PointOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GmlPoint.h"
#include "property-values/GpmlConstantValue.h"



namespace GPlatesAppLogic
{


	namespace
	{


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

				static const GPlatesModel::FeatureType small_circle_feature_type = 
						GPlatesModel::FeatureType::create_gpml("SmallCircle");

				if (feature_handle.feature_type() == small_circle_feature_type)
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


	}

}


bool
GPlatesAppLogic::ReconstructMethodSmallCircle::can_reconstruct_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref)
{
	CanReconstructFeature can_reconstruct_feature;
	can_reconstruct_feature.visit_feature(feature_weak_ref);

	return can_reconstruct_feature.can_reconstruct();
}


void
GPlatesAppLogic::ReconstructMethodSmallCircle::get_present_day_geometries(
		std::vector<Geometry> &present_day_geometries,
		const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref) const
{
	GetPresentDayGeometries visitor(present_day_geometries);

	visitor.visit_feature(feature_weak_ref);
}


void
GPlatesAppLogic::ReconstructMethodSmallCircle::reconstruct_feature(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref,
		const ReconstructHandle::type &reconstruct_handle,
		const ReconstructParams &reconstruct_params,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time)
{
	SmallCircleGeometryPopulator visitor(
		reconstructed_feature_geometries,
		reconstruction_tree_creator,
		reconstruction_time);

	visitor.visit_feature(feature_weak_ref);
}

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructMethodSmallCircle::reconstruct_geometry(
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
	const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
	const ReconstructionTreeCreator &reconstruction_tree_creator,
	const double &reconstruction_time,
	bool reverse_reconstruct)
{
#if 1
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
#endif

	// Otherwise just return the original geometry.
	return geometry;
}
