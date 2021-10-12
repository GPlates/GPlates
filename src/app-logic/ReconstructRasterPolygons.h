/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTRASTERPOLYGONS_H
#define GPLATES_APP_LOGIC_RECONSTRUCTRASTERPOLYGONS_H

#include <vector>
#include <boost/optional.hpp>

#include "AppLogicUtils.h"
#include "ReconstructionTree.h"

#include "maths/PolygonOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Polygons used to reconstruct a raster.
	 *
	 * The polygon geometry, plate id and time periods will not change during the lifetime
	 * of an instance of this class.
	 * However, the rotation transforms of the polygons will change as the reconstruction time changes.
	 */
	class ReconstructRasterPolygons :
			public GPlatesUtils::ReferenceCount<ReconstructRasterPolygons>,
			private GPlatesModel::ConstFeatureVisitor
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ReconstructRasterPolygons.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructRasterPolygons> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a ReconstructRasterPolygons.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructRasterPolygons> non_null_ptr_to_const_type;


		/**
		 * Enough information to generate a closed region through which to view part of a raster.
		 */
		class ReconstructablePolygonRegion :
				public GPlatesUtils::ReferenceCount<ReconstructablePolygonRegion>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructablePolygonRegion> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructablePolygonRegion> non_null_ptr_to_const_type;

			//! Typedef for a sequence of interior polygons.
			typedef std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
					interior_polygon_seq_type;


			//! Creates a @a ReconstructablePolygonRegion object.
			static
			non_null_ptr_type
			create(
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &exterior_polygon_)
			{
				return non_null_ptr_type(new ReconstructablePolygonRegion(exterior_polygon_));
			}


			//! The sole exterior polygon.
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type exterior_polygon;

			//! Optional interior polygons that represents holes in the exterior polygon.
			interior_polygon_seq_type interior_polygons;

			// Can be used to control visibility of this polygon region based on reconstruction time.
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_appearance;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_disappearance;

		private:
			//! Constructor.
			explicit
			ReconstructablePolygonRegion(
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &exterior_polygon_) :
				exterior_polygon(exterior_polygon_)
			{  }
		};

		//! Typedef for a sequence of polygon regions.
		typedef std::vector<ReconstructablePolygonRegion::non_null_ptr_to_const_type> polygon_region_seq_type;


		/**
		 * Groups all polygon regions that have the same rotation (plate id) together.
		 */
		class RotationGroup :
				public GPlatesUtils::ReferenceCount<RotationGroup>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<RotationGroup> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const RotationGroup> non_null_ptr_to_const_type;


			//! Creates a @a RotationGroup object.
			static
			non_null_ptr_type
			create(
					const GPlatesMaths::UnitQuaternion3D &initial_rotation)
			{
				return non_null_ptr_type(new RotationGroup(initial_rotation));
			}


			/**
			 * Returns the finite rotation for the current reconstruction time as updated
			 * by @a update_rotations.
			 *
			 * It's a unit quaternion instead of a @a FiniteRotation to save memory
			 * since this is not going to be used to rotate any geometry, it's just going to
			 * used to convert to a matrix for OpenGL rendering.
			 */
			GPlatesMaths::UnitQuaternion3D current_rotation;

			/**
			 * The polygon regions in this rotation group.
			 *
			 * NOTE: These will remain unchanged for the lifetime of the parent
			 * @a ReconstructRasterPolygons object containing them.
			 */
			polygon_region_seq_type polygon_regions;

		private:
			//! Constructor.
			explicit
			RotationGroup(
					const GPlatesMaths::UnitQuaternion3D &initial_rotation) :
				current_rotation(initial_rotation)
			{  }
		};


		/**
		 * Creates a @a ReconstructRasterPolygons object.
		 *
		 * All static polygons (and their plate ids and age ranges) contained in
		 * the specified feature collections are extracted into the returned
		 * @a ReconstructRasterPolygons object.
		 */
		template< typename FeatureCollectionWeakRefIterator >
		static
		non_null_ptr_type
		create(
				FeatureCollectionWeakRefIterator polygon_feature_collections_begin,
				FeatureCollectionWeakRefIterator polygon_feature_collections_end,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree)
		{
			non_null_ptr_type reconstruct_raster_polygons(
					new ReconstructRasterPolygons(reconstruction_tree));

			AppLogicUtils::visit_feature_collections(
					polygon_feature_collections_begin, 
					polygon_feature_collections_end,
					*reconstruct_raster_polygons);

			return reconstruct_raster_polygons;
		}


		/**
		 * Return the current reconstruction time.
		 *
		 * This is updated whenever @a update_rotations is called.
		 */
		double
		get_current_reconstruction_time() const
		{
			return d_current_reconstruction_tree->get_reconstruction_time();
		}


		/**
		 * Updates the finite rotations of all polygons.
		 *
		 * Call this after a new reconstruction (such as when the reconstruction time changes).
		 */
		void
		update_rotations(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree);


		/**
		 * Returns the rotations grouped sorted by plate id (from lowest to highest).
		 */
		void
		get_rotation_groups_sorted_by_plate_id(
				std::vector<RotationGroup::non_null_ptr_to_const_type> &rotation_groups) const;

	private:
		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		finalise_post_feature_properties(
				feature_handle_type &feature_handle);

		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		void
		visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

	private:
		//! Typedef for a map of plate ids to rotation groups.
		typedef std::map<
				GPlatesModel::integer_plate_id_type,
				RotationGroup::non_null_ptr_type> rotation_group_map_type;


		/**
		 * The current reconstruction tree used to find rotations for the polygons.
		 */
		ReconstructionTree::non_null_ptr_to_const_type d_current_reconstruction_tree;

		/**
		 * Keeps track of the rotation groups mapped to plate ids.
		 */
		rotation_group_map_type d_rotation_groups;

		//! Special-case rotation group for polygons with no plate id.
		boost::optional<RotationGroup::non_null_ptr_type> d_no_plate_id_rotation_group;

		//
		// These are used when collecting information during feature visitation.
		// They are reset for each new feature visited.
		//
		struct FeatureInfoAccumulator
		{
			std::vector<ReconstructablePolygonRegion::non_null_ptr_type> polygon_regions;
			boost::optional<GPlatesModel::integer_plate_id_type> recon_plate_id;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_appearance;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_disappearance;
		};

		//! Used during visitation of a feature.
		FeatureInfoAccumulator d_feature_info_accumulator;


		//! Constructor.
		ReconstructRasterPolygons(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree) :
			d_current_reconstruction_tree(reconstruction_tree)
		{  }


		/**
		 * Returns the rotation group corresponding to the plate id of the
		 * just visited feature.
		 */
		RotationGroup &
		get_rotation_group();
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTRASTERPOLYGONS_H
