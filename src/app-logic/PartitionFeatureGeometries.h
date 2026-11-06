/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_PARTITIONFEATUREGEOMETRIES_H
#define GPLATES_APP_LOGIC_PARTITIONFEATUREGEOMETRIES_H

#include <list>
#include <map>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GeometryCookieCutter.h"
#include "ReconstructionGeometry.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/PropertyName.h"
#include "model/TopLevelProperty.h"

#include "property-values/GmlDataBlockCoordinateList.h"


/**
 * Partitioning a feature's geometry properties, with their scalar coverages, by a cookie cutter
 * (phase4b-design.md sections 7.3 and 8.3): the part of PartitionFeatureUtils that pyGPlates'
 * 'partition_features' shares with GPlates' Assign Plate IDs, kept apart from PartitionFeatureUtils'
 * GPlates-only dependencies (Assign Plate IDs itself, reconstructions).
 */
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
		 *
		 * A time-dependent geometry property is partitioned only if it has a single time window
		 * containing the reconstruction time (then as a constant one, its property clone being
		 * a constant property); any other is not partitioned, and is not in
		 * @a partitioned_properties, so that the caller leaves it unchanged.
		 */
		boost::shared_ptr<const PartitionedFeature>
		partition_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GeometryCookieCutter &geometry_cookie_cutter,
				bool respect_feature_time_period = true,
				boost::optional< std::vector<GPlatesModel::FeatureHandle::iterator> &> partitioned_properties = boost::none);


		/**
		 * Returns true if @a feature_ref exists at time @a reconstruction_time.
		 */
		bool
		does_feature_exist_at_reconstruction_time(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
				const double &reconstruction_time);

	}
}

#endif // GPLATES_APP_LOGIC_PARTITIONFEATUREGEOMETRIES_H
