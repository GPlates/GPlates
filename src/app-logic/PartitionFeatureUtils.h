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
 
#ifndef GPLATES_APP_LOGIC_PARTITIONFEATUREUTILS_H
#define GPLATES_APP_LOGIC_PARTITIONFEATUREUTILS_H

#include <list>
#include <map>
#include <utility>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "AssignPlateIds.h"
#include "GeometryCookieCutter.h"
#include "Reconstruction.h"
#include "ReconstructionTree.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/PropertyName.h"
#include "model/types.h"
#include "model/TopLevelProperty.h"

#include "property-values/GmlTimePeriod.h"


namespace GPlatesAppLogic
{
	namespace PartitionFeatureUtils
	{
		/**
		 * The results of partitioning the geometry properties of a feature.
		 */
		class PartitionedFeature
		{
		public:
			/**
			 * The results of partitioning a feature geometry property.
			 */
			class GeometryProperty
			{
			public:
				//! Sequence of partitioned geometries.
				typedef GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type
						partitioned_geometry_seq_type;

				//! Sequence of inside partitions.
				typedef GPlatesAppLogic::GeometryCookieCutter::partition_seq_type
						partition_seq_type;

				GeometryProperty(
						const GPlatesModel::PropertyName &geometry_property_name_,
						const GPlatesModel::TopLevelProperty::non_null_ptr_type &geometry_property_clone_) :
					geometry_property_name(geometry_property_name_),
					geometry_property_clone(geometry_property_clone_)
				{  }

				GPlatesModel::PropertyName geometry_property_name;
				GPlatesModel::TopLevelProperty::non_null_ptr_type geometry_property_clone;
				partition_seq_type partitioned_inside_geometries;
				partitioned_geometry_seq_type partitioned_outside_geometries;
			};

			//! Sequence of partitioning results for geometry properties in the feature.
			typedef std::list<GeometryProperty> partitioned_geometry_property_seq_type;

			/**
			 * Partitioning results for each geometry property in the feature.
			 */
			partitioned_geometry_property_seq_type partitioned_geometry_properties;
		};


		/**
		 * Partitions the geometries in the geometry properties of @a feature_ref
		 * using partitioning polygons in @a geometry_cookie_cutter.
		 *
		 * The results of the partitioning are returned or false if @a feature_ref
		 * doesn't exist at the reconstruction time of @a geometry_cookie_cutter.
		 *
		 * NOTE: This does not modify @a feature_ref.
		 *
		 * If @a respect_feature_time_period is true (the default) then the feature is only
		 * partitioned if the reconstruction time (stored in @a geometry_cookie_cutter) is within
		 * the time period over which the feature is defined.
		 */
		boost::shared_ptr<const PartitionedFeature>
		partition_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
				const GPlatesAppLogic::GeometryCookieCutter &geometry_cookie_cutter,
				bool respect_feature_time_period = true);


		/**
		 * Abstract base class for copying property values from a partitioning polygon
		 * feature to a partitioned feature.
		 *
		 * The number and type of property values copied is determined by the derived class.
		 *
		 * The partitioned feature may have an existing property with the same property name
		 * as the property being assigned so the property value assigner must always overwrite
		 * a property value if it exists (ie, remove it first and then add a new one).
		 */
		class PropertyValueAssigner
		{
		public:
			virtual
			~PropertyValueAssigner()
			{  }

			/**
			 * Copies property values from @a partitioning_feature to @a partitioned_feature.
			 *
			 * If @a partitioning_feature is false then it means @a partitioned_feature
			 * represents the feature containing geometries that were outside all
			 * partitioning polygons.
			 */
			virtual
			void
			assign_property_values(
					const GPlatesModel::FeatureHandle::weak_ref &partitioned_feature,
					boost::optional<GPlatesModel::FeatureHandle::const_weak_ref> partitioning_feature) = 0;
		};


		/**
		 * Optionally assigns various feature property types such as
		 * reconstruction plate ids and time periods.
		 */
		class GenericFeaturePropertyAssigner :
				public PropertyValueAssigner
		{
		public:
			/**
			 * Defaults property values to use when there is no partitioning feature.
			 * These are obtained from the feature before it is partitioned or modified
			 * in any way.
			 */
			GenericFeaturePropertyAssigner(
					const GPlatesModel::FeatureHandle::const_weak_ref &original_feature,
					const GPlatesAppLogic::AssignPlateIds::feature_property_flags_type &feature_property_types_to_assign);

			virtual
			void
			assign_property_values(
					const GPlatesModel::FeatureHandle::weak_ref &partitioned_feature,
					boost::optional<GPlatesModel::FeatureHandle::const_weak_ref> partitioning_feature);

		private:
			boost::optional<GPlatesModel::integer_plate_id_type> d_default_reconstruction_plate_id;
			boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> d_default_valid_time;

			GPlatesAppLogic::AssignPlateIds::feature_property_flags_type d_feature_property_types_to_assign;
		};


		/**
		 * Manages creation/cloning of features for partitioned geometries.
		 */
		class PartitionedFeatureManager
		{
		public:
			PartitionedFeatureManager(
					const GPlatesModel::FeatureHandle::weak_ref &original_feature,
					const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
					const boost::shared_ptr<PropertyValueAssigner> &property_value_assigner);


			/**
			 * Returns the feature mapped to @a partition (allocates new feature if necessary).
			 *
			 * If this is the first time the feature for @a partition has been requested then
			 * it allocates a feature (either using the original feature passed into the constructor,
			 * if it hasn't been used yet, or by cloning it).
			 * At the same time property values are copied from the partitioning feature
			 * associated with @a partition to the newly allocated feature using the
			 * property value assigner passed into the constructor.
			 *
			 * Use boost::none for @a partition if you want the feature that will
			 * contain geometries partitioned outside all partitioning polygons.
			 */
			GPlatesModel::FeatureHandle::weak_ref
			get_feature_for_partition(
					boost::optional<const ReconstructionGeometry *> partition = boost::none);

		private:
			/**
			 * Typedef for mapping partitions to features.
			 */
			typedef std::map<
					boost::optional<const ReconstructionGeometry *>,
					GPlatesModel::FeatureHandle::weak_ref> partition_to_feature_map_type;

			/**
			 * The original feature can be used to contain partitioned geometry.
			 *
			 * This is done instead of cloning the original feature and then destroying
			 * the original feature (which relies on feature delete which currently isn't
			 * fully supported yet).
			 */
			GPlatesModel::FeatureHandle::weak_ref d_original_feature;

			/**
			 * The feature collection containing the original feature and any cloned features.
			 */
			GPlatesModel::FeatureCollectionHandle::weak_ref d_feature_collection;

			//! Whether the original feature is being used by an inside or outside feature.
			bool d_has_original_feature_been_claimed;

			/**
			 * Used to copy requested property values from partitioning polygon feature to
			 * partitioned features.
			 */
			boost::shared_ptr<PropertyValueAssigner> d_property_value_assigner;

			/**
			 * The currently assigned features for the various partitions (including the
			 * the feature representing no partition).
			 */
			partition_to_feature_map_type d_partitioned_features;


			/**
			 * Return the original feature is it hasn't been claimed yet or
			 * return a clone of it (without geometry properties or plate id).
			 */
			GPlatesModel::FeatureHandle::weak_ref
			create_feature();

			/**
			 * Assigns property values when a feature is first referenced.
			 */
			void
			assign_property_values(
					const GPlatesModel::FeatureHandle::weak_ref &partitioned_feature,
					boost::optional<const ReconstructionGeometry *> partition);
		};


		/**
		 * Adds partitioned geometries to the partitioned feature associated with
		 * @a partition.
		 *
		 * If @a partition is false then adds to the special feature associated with no partition.
		 *
		 * All partitioned geometries are reverse reconstructed using the plate id of their
		 * partitioning polygon (if has a plate id).
		 */
		void
		add_partitioned_geometry_to_feature(
				const GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type &
						partitioned_geometries,
				const GPlatesModel::PropertyName &geometry_property_name,
				PartitionedFeatureManager &partitioned_feature_manager,
				const ReconstructionTree &reconstruction_tree,
				boost::optional<const ReconstructionGeometry *> partition = boost::none);


		/**
		 * Adds partitioned geometries to the partitioned features associated with
		 * the partitioned polygons.
		 *
		 * All partitioned geometries are reverse reconstructed using the plate id of their
		 * partitioning polygon (if has a plate id).
		 */
		void
		add_partitioned_geometry_to_feature(
				const GPlatesAppLogic::GeometryCookieCutter::partition_seq_type &partitions,
				const GPlatesModel::PropertyName &geometry_property_name,
				PartitionedFeatureManager &partitioned_feature_manager,
				const ReconstructionTree &reconstruction_tree);


		/**
		 * Adds the reconstructed geometry @a geometry_property to the partitioned feature
		 * associated with @a partition and reverse reconstructs the geometry to present day
		 * (if @a partition has a plate id and the reconstruction time is not present day).
		 *
		 * If @a partition is false then adds to the special feature associated with no partition.
		 */
		void
		add_partitioned_geometry_to_feature(
				const GPlatesModel::TopLevelProperty::non_null_ptr_type &geometry_property,
				PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager,
				const ReconstructionTree &reconstruction_tree,
				boost::optional<const ReconstructionGeometry *> partition = boost::none);


		/**
		 * Finds the partitioning polygon that contains the most partitioned geometries
		 * of @a geometry_property.
		 *
		 * This is based on arc distance of the partitioned geometries if they are
		 * line geometries (polyline, polygon) or number of points if point geometries.
		 *
		 * Returns the false if @a geometry_property has no partitioned inside geometries.
		 */
		boost::optional<const ReconstructionGeometry *>
		find_partition_containing_most_geometry(
				const PartitionedFeature::GeometryProperty &geometry_property);


		/**
		 * Finds the partitioning polygon that contains the most partitioned geometries
		 * of @a partitioned_feature.
		 *
		 * This is based on arc distance of the partitioned geometries if they are
		 * line geometries (polyline, polygon) or number of points if point geometries.
		 *
		 * Returns the false if @a partitioned_feature has no partitioned inside geometries.
		 */
		boost::optional<const ReconstructionGeometry *>
		find_partition_containing_most_geometry(
				const PartitionedFeature &partitioned_feature);


		/**
		 * Returns true if @a feature_ref exists at time @a reconstruction_time.
		 */
		bool
		does_feature_exist_at_reconstruction_time(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
				const double &reconstruction_time);


		/**
		 * Returns the reverse rotation (to present day) using the plate id of the
		 * partitioning polygon @a partition (if it's not false).
		 */
		boost::optional<GPlatesMaths::FiniteRotation>
		get_reverse_reconstruction(
				boost::optional<const ReconstructionGeometry *> partition,
				const ReconstructionTree &reconstruction_tree);


		/**
		 * Returns the 'gpml:reconstructionPlateId' plate id if one exists.
		 */
		boost::optional<GPlatesModel::integer_plate_id_type>
		get_reconstruction_plate_id_from_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref);


		/**
		 * Assigns a 'gpml:reconstructionPlateId' property value to @a feature_ref.
		 * Removes any properties with this name that might already exist in @a feature_ref.
		 *
		 * If @a reconstruction_plate_id is false then only reconstruction plate id properties
		 * are removed and none added.
		 */
		void
		assign_reconstruction_plate_id_to_feature(
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref);


		/**
		 * Returns the 'gml:validTime' time period if one exists.
		 */
		boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type>
		get_valid_time_from_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref);


		/**
		 * Assigns a 'gml:validTime' property value to @a feature_ref.
		 * Removes any properties with this name that might already exist in @a feature_ref.
		 *
		 * If @a valid_time is false then only valid time properties are removed and none added.
		 */
		void
		assign_valid_time_to_feature(
				boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> valid_time,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref);


		/**
		 * Creates a property value suitable for @a geometry and appends it
		 * to @a feature_ref with the property name @a geometry_property_name.
		 *
		 * It doesn't attempt to remove any existing properties named @a geometry_property_name.
		 */
		void
		append_geometry_to_feature(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const GPlatesModel::PropertyName &geometry_property_name,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref);


		/**
		 * Visits a @a GeometryOnSphere and accumulates a size metric for it;
		 * for points/multipoints this is number of points and for polylines/polygons
		 * this is arc distance.
		 */
		class GeometrySizeMetric :
				public boost::less_than_comparable<GeometrySizeMetric>
		{
		public:
			GeometrySizeMetric();


			/**
			 * For points and multipoints adds number of points to current total
			 * number of points; for polylines and polygons adds the
			 * arc distance (unit sphere) to the current total arc distance.
			 */
			void
			accumulate(
					const GPlatesMaths::GeometryOnSphere &geometry);


			/**
			 * Adds metric @a geometry_size_metric to this object.
			 */
			void
			accumulate(
					const GeometrySizeMetric &geometry_size_metric);


			/**
			 * Less than operator.
			 * Greater than operator provided by base class boost::less_than_comparable.
			 */
			bool
			operator<(
					const GeometrySizeMetric &rhs) const;

		private:
			unsigned int d_num_points;
			GPlatesMaths::real_t d_arc_distance;
			bool d_using_arc_distance;
		};
	}
}

#endif // GPLATES_APP_LOGIC_PARTITIONFEATUREUTILS_H
