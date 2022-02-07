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
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "AssignPlateIds.h"
#include "GeometryCookieCutter.h"
#include "Reconstruction.h"
#include "ReconstructionGeometry.h"
#include "ReconstructMethodInterface.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/PropertyName.h"
#include "model/types.h"
#include "model/TopLevelProperty.h"

#include "property-values/GmlDataBlockCoordinateList.h"
#include "property-values/GmlTimePeriod.h"


namespace GPlatesAppLogic
{
	namespace PartitionFeatureUtils
	{
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_domain_type;
		typedef std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type> geometry_range_type;

		/**
		 * The results of partitioning the geometry properties of a feature.
		 */
		class PartitionedFeature
		{
		public:

			/**
			 * A partitioned geometry and optional associated partitioned scalar coverage.
			 */
			class PartitionedGeometry
			{
			public:
				explicit
				PartitionedGeometry(
						const geometry_domain_type &geometry_domain_,
						const boost::optional<geometry_range_type> &geometry_range_ = boost::none) :
					geometry_domain(geometry_domain_),
					geometry_range(geometry_range_)
				{  }

				geometry_domain_type geometry_domain;
				boost::optional<geometry_range_type> geometry_range; // Not all geometries (domains) have a range.
			};

			typedef std::vector<PartitionedGeometry> partitioned_geometry_seq_type;

			/**
			 * A partitioning polygon and the geometries (and optional scalar coverages) partitioned inside it.
			 */
			class Partition
			{
			public:
				explicit
				Partition(
						const ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry_) :
					reconstruction_geometry(reconstruction_geometry_)
				{ }

				ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry;
				partitioned_geometry_seq_type partitioned_geometries;
			};

			typedef std::list<Partition> partition_seq_type;

			/**
			 * Clone of a top-level geometry domain (and optional range) property.
			 */
			class GeometryPropertyClone
			{
			public:
				explicit
				GeometryPropertyClone(
						const GPlatesModel::TopLevelProperty::non_null_ptr_type &domain_,
						const boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> &range_ = boost::none) :
					domain(domain_),
					range(range_)
				{  }

				GPlatesModel::TopLevelProperty::non_null_ptr_type domain;
				boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> range;
			};

			typedef std::vector<GeometryPropertyClone> geometry_property_clone_seq_type;

			/**
			 * The results of partitioning a feature's geometry properties with a specific
			 * geometry *domain* property name.
			 *
			 * Note that the geometry *range* property name depends on the *domain* property name
			 * (there is a one-to-one mapping between them).
			 */
			class GeometryProperty
			{
			public:
				explicit
				GeometryProperty(
						const GPlatesModel::PropertyName &domain_property_name_,
						const boost::optional<GPlatesModel::PropertyName> &range_property_name_ = boost::none) :
					domain_property_name(domain_property_name_),
					range_property_name(range_property_name_)
				{  }

				GPlatesModel::PropertyName domain_property_name;
				boost::optional<GPlatesModel::PropertyName> range_property_name;

				geometry_property_clone_seq_type property_clones;

				partition_seq_type partitioned_inside_geometries;
				partitioned_geometry_seq_type partitioned_outside_geometries;
			};

			/**
			 * Mapping of geometry domain property names to partitioning results for geometry properties in the feature.
			 */
			typedef std::map<GPlatesModel::PropertyName, GeometryProperty> partitioned_geometry_property_map_type;

			/**
			 * Partitioning results for each geometry property in the feature.
			 */
			partitioned_geometry_property_map_type partitioned_geometry_properties;
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
		 *
		 * If @a partitioned_properties is specified then the partitioned geometry domain
		 * (and optional associated range) properties are returned. This enables the caller to
		 * subsequently remove those properties from the feature if it is to be re-used as
		 * one of the partitioned features.
		 */
		boost::shared_ptr<const PartitionedFeature>
		partition_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GeometryCookieCutter &geometry_cookie_cutter,
				bool respect_feature_time_period = true,
				boost::optional< std::vector<GPlatesModel::FeatureHandle::iterator> &> partitioned_properties = boost::none);


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
			 * Default property values, to use when there is no partitioning feature, are obtained from @a original_feature.
			 *
			 * If 'verify_information_model' is true then feature property types are only added if they don't not violate the GPGIM.
			 */
			GenericFeaturePropertyAssigner(
					const GPlatesModel::FeatureHandle::const_weak_ref &original_feature,
					const AssignPlateIds::feature_property_flags_type &feature_property_types_to_assign,
					bool verify_information_model);

			virtual
			void
			assign_property_values(
					const GPlatesModel::FeatureHandle::weak_ref &partitioned_feature,
					boost::optional<GPlatesModel::FeatureHandle::const_weak_ref> partitioning_feature);

		private:
			bool d_verify_information_model;

			boost::optional<GPlatesModel::integer_plate_id_type> d_default_reconstruction_plate_id;
			boost::optional<GPlatesModel::integer_plate_id_type> d_default_conjugate_plate_id;
			boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> d_default_valid_time;

			AssignPlateIds::feature_property_flags_type d_feature_property_types_to_assign;
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
					const GPlatesModel::PropertyName &geometry_domain_property_name,
					bool geometry_domain_has_associated_range,
					boost::optional<const ReconstructionGeometry *> partition = boost::none);

		private:

			//! Typedef for a mapping of domain property name to boolean indicating an associated range.
			typedef std::map<
					GPlatesModel::PropertyName/*geometry_domain_property_name*/,
					bool/*geometry_domain_has_associated_range*/> feature_contents_type;

			struct FeatureInfo
			{
				explicit
				FeatureInfo(
						const GPlatesModel::FeatureHandle::weak_ref &feature_) :
					feature(feature_)
				{  }

				GPlatesModel::FeatureHandle::weak_ref feature;
				feature_contents_type contents;
			};

			typedef std::list<FeatureInfo> feature_info_seq_type;

			/**
			 * Typedef for mapping partitions to features.
			 */
			typedef std::map<
					boost::optional<const ReconstructionGeometry *>,
					feature_info_seq_type>
							partition_to_feature_map_type;

			/**
			 * The original feature.
			 *
			 * This is the first feature to be returned by @a get_feature_for_partition.
			 * This is done so we can avoid destroying the original feature (which relies on
			 * feature delete which currently isn't fully supported yet).
			 */
			GPlatesModel::FeatureHandle::weak_ref d_original_feature;

			//! Whether the original feature is being used by an inside or outside feature.
			bool d_has_original_feature_been_claimed;

			/**
			 * A cloned version of the original feature.
			 *
			 * Since the original feature can be returned to the user (and subsequently modified)
			 * we need to clone it before this happens. Then we can make clones of this feature to
			 * return to the user without worrying about those modifications to the original feature.
			 */
			GPlatesModel::FeatureHandle::non_null_ptr_to_const_type d_feature_to_clone_from;

			/**
			 * The feature collection containing the original feature and any cloned features.
			 */
			GPlatesModel::FeatureCollectionHandle::weak_ref d_feature_collection;

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
		 * Adds partitioned inside geometries to the partitioned features associated with the partitioned polygons.
		 *
		 * Partitioned outside geometries are added to the special feature associated with no partition.
		 *
		 * All partitioned geometries are reverse reconstructed using the plate id of their partitioning polygon
		 * (if has a plate id) and/or deformed if @a reconstruct_method_context contains deformation.
		 */
		void
		add_partitioned_geometry_to_feature(
				const PartitionedFeature::GeometryProperty &geometry_property,
				PartitionedFeatureManager &partitioned_feature_manager,
				const ReconstructMethodInterface::Context &reconstruct_method_context,
				const double &reconstruction_time);


		/**
		 * Adds the reconstructed geometry @a geometry_domain_property to the partitioned feature
		 * associated with @a partition and reverse reconstructs the geometry to present day
		 * (if @a partition has a plate id and the reconstruction time is not present day -
		 * also deformation may be involved if @a reconstruct_method_context contains deformation).
		 *
		 * Also adds the optional geometry range property (scalar coverage).
		 *
		 * If @a partition is none then adds to the special feature associated with no partition.
		 */
		void
		add_unpartitioned_geometry_to_feature(
				const PartitionedFeature::GeometryProperty &geometry_property,
				PartitionedFeatureManager &partitioned_feature_manager,
				const ReconstructMethodInterface::Context &reconstruct_method_context,
				const double &reconstruction_time,
				boost::optional<const ReconstructionGeometry *> partition = boost::none);


		/**
		 * Finds the partitioning polygon that contains the most partitioned geometries
		 * of @a partitioned_feature.
		 *
		 * This is based on arc distance of the partitioned geometries if they are
		 * line geometries (polyline, polygon) or number of points if point geometries.
		 *
		 * Returns the none if @a partitioned_feature has no partitioned inside geometries.
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
		 * Returns the reverse reconstructed geometry from @a reconstructed_geometry to present day using
		 * the intrinsic state (properties) of @a feature and extrinsic state of @a reconstruct_method_context.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reverse_reconstruct(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &reconstructed_geometry,
				const GPlatesModel::FeatureHandle::weak_ref &feature,
				const ReconstructMethodInterface::Context &reconstruct_method_context,
				const double &reconstruction_time);


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
		 *
		 * If 'verify_information_model' is true then property is only added if it doesn't not violate the GPGIM.
		 */
		void
		assign_reconstruction_plate_id_to_feature(
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				bool verify_information_model);


		/**
		 * Returns the 'gpml:conjugatePlateId' plate id if one exists.
		 */
		boost::optional<GPlatesModel::integer_plate_id_type>
		get_conjugate_plate_id_from_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref);


		/**
		 * Assigns a 'gpml:conjugatePlateId' property value to @a feature_ref.
		 * Removes any properties with this name that might already exist in @a feature_ref.
		 *
		 * If @a conjugate_plate_id is false then only conjugate plate id properties
		 * are removed and none added.
		 */
		void
		assign_conjugate_plate_id_to_feature(
				boost::optional<GPlatesModel::integer_plate_id_type> conjugate_plate_id,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				bool verify_information_model);


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
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				bool verify_information_model);


		/**
		 * Creates a property value suitable for @a geometry_domain and appends it
		 * to @a feature_ref with the property name @a geometry_domain_property_name.
		 *
		 * It doesn't attempt to remove any existing properties named @a geometry_domain_property_name.
		 */
		void
		append_geometry_domain_to_feature(
				const geometry_domain_type &geometry_domain,
				const GPlatesModel::PropertyName &geometry_domain_property_name,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref);


		/**
		 * Creates a property value suitable for @a geometry_range and appends it
		 * to @a feature_ref with the property name @a geometry_range_property_name.
		 *
		 * It doesn't attempt to remove any existing properties named @a geometry_range_property_name.
		 */
		void
		append_geometry_range_to_feature(
				const geometry_range_type &geometry_range,
				const GPlatesModel::PropertyName &geometry_range_property_name,
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
