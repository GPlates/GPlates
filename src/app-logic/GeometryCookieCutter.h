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

#ifndef GPLATES_APP_LOGIC_GEOMETRYCOOKIECUTTER_H
#define GPLATES_APP_LOGIC_GEOMETRYCOOKIECUTTER_H

#include <list>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "AppLogicFwd.h"
#include "ReconstructionGeometry.h"

#include "maths/PolygonIntersections.h"
#include "maths/PolygonOnSphere.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	/**
	 * Partitions geometry using dynamic resolved topological boundaries and/or
	 * static reconstructed feature polygons.
	 */
	class GeometryCookieCutter :
			private boost::noncopyable
	{
	public:
		/**
		 * Typedef for a sequence of geometries resulting from partitioning a single geometry.
		 */
		typedef GPlatesMaths::PolygonIntersections::partitioned_geometry_seq_type
				partitioned_geometry_seq_type;

		/**
		 * Typedef for a partitioning polygon and the geometries partitioned inside it.
		 */
		class Partition
		{
		public:
			Partition(
					const reconstruction_geometry_non_null_ptr_to_const_type &reconstruction_geometry_) :
				reconstruction_geometry(reconstruction_geometry_)
			{ }

			reconstruction_geometry_non_null_ptr_to_const_type reconstruction_geometry;
			partitioned_geometry_seq_type partitioned_geometries;
		};

		/**
		 * Typedef for a sequence of inside partitions.
		 */
		typedef std::list<Partition> partition_seq_type;


		/**
		 * Finds reconstructed polygon geometries to partition other geometry with.
		 *
		 * Topological networks (and their static interior polygons, if any) are used first when
		 * partitioning, then topological boundaries and then regular static polygons.
		 * The static interior polygons of any topological networks are higher priority than the
		 * networks themselves (it's required since the network boundaries contain the interior polygons).
		 *
		 * The partitioning polygons, in each group (ie, static, topological boundary, topological network),
		 * are sorted from highest plate id to lowest.
		 * This ensures that if there are any overlapping polygons in the (combined) set then
		 * those with the highest plate id will partition before those with lower plate ids.
		 *
		 * The reason for preferring plates further from the anchor is, I think,
		 * because it provides more detailed rotations since plates further down the plate
		 * circuit (or tree) are relative to plates higher up the tree and hence provide
		 * extra detail.
		 * And they are presumably smaller so they will partition their area leaving geometry
		 * partitioned outside their area a bigger overlapping plate polygon.
		 *
		 * Ideally the plate boundaries shouldn't overlap at all but it is possible to construct
		 * a set that do overlap.
		 *
		 * NOTE: We also include topological network here even though they are deforming
		 * and not rigid regions. This is because the current topological closed plate polygons
		 * do *not* cover the entire globe and leave holes where there are topological networks.
		 * So we assign plate ids using the topological networks with the understanding that
		 * these are to be treated as rigid regions as a first order approximation (although the
		 * plate ids don't exist in the rotation file so they'll need to be added - for example
		 * the Andes deforming region has plate id 29201 which should be mapped to 201 in
		 * the rotation file).
		 *
		 * Note that if the topological network contains interior static polygons (microblocks)
		 * then they do *not* need to be included in @a reconstructed_static_polygons - they are
		 * found directly through the networks in @a resolved_topological_networks.
		 *
		 * @a partition_point_speed_and_memory determines the speed versus memory trade-off of the
		 * point-in-polygon tests for partitioned *point* geometries.
		 */
		GeometryCookieCutter(
				const double &reconstruction_time,
				boost::optional<const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &> reconstructed_static_polygons,
				boost::optional<const std::vector<resolved_topological_boundary_non_null_ptr_type> &> resolved_topological_boundaries,
				boost::optional<const std::vector<resolved_topological_network_non_null_ptr_type> &> resolved_topological_networks,
				GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory = GPlatesMaths::PolygonOnSphere::ADAPTIVE);


		/**
		 * Returns true if we have partitioning polygons.
		 */
		bool
		has_partitioning_polygons() const;


		/**
		 * Partition @a geometry using the partitioning polygons found in the constructor.
		 *
		 * The partitioning polygons found in the constructor are assumed to not
		 * overlap each other - if they do then the overlapping boundary a geometry
		 * is partitioned into is valid but undefined.
		 *
		 * On returning @a partitioned_outside_geometries contains any partitioned
		 * geometries that are not inside any partitioning polygons.
		 * If there happen to be no partitioning polygons then @a geometry is added
		 * to @a partitioned_outside_geometries.
		 *
		 * Returns true if @a geometry is inside any partitioning polygons (even partially)
		 * in which case elements were appended to @a partitioned_inside_geometries.
		 */
		bool
		partition_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				partition_seq_type &partitioned_inside_geometries,
				partitioned_geometry_seq_type &partitioned_outside_geometries) const;


		/**
		 * Finds which partitioning polygon boundary contains @a point.
		 * Returns false if no containing boundaries are found.
		 *
		 * NOTE: If there are overlapping partitioning polygons and the specified point
		 * is inside more that one polygon then the partitioning polygon with the highest
		 * plate id is chosen. See the constructor comment for more details.
		 */
		boost::optional<const ReconstructionGeometry *>
		partition_point(
				const GPlatesMaths::PointOnSphere &point) const;


		/**
		 * Returns the reconstruction time of the reconstructed partitioning polygons
		 * used to partition geometry with.
		 */
		double
		get_reconstruction_time() const
		{
			return d_reconstruction_time;
		}

	private:
		/**
		 * Associates a @a ReconstructionGeometry with its polygon structure
		 * for geometry partitioning.
		 */
		class PartitioningGeometry
		{
		public:
			PartitioningGeometry(
					const reconstruction_geometry_non_null_ptr_type &reconstruction_geometry,
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon,
					GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory);


			reconstruction_geometry_non_null_ptr_to_const_type d_reconstruction_geometry;
			GPlatesMaths::PolygonIntersections::non_null_ptr_type d_polygon_intersections;

			//! Used to sort by plate id.
			struct SortPlateIdHighestToLowest
			{
				bool
				operator()(
						const PartitioningGeometry &lhs,
						const PartitioningGeometry &rhs) const;
			};
		};

		//! Typedef for a sequence of partitioning geometries.
		typedef std::vector<PartitioningGeometry> partitioning_geometry_seq_type;


		//! The partitioning geometries.
		partitioning_geometry_seq_type d_partitioning_geometries;

		double d_reconstruction_time;

		GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory d_partition_point_speed_and_memory;


		/**
		 * Adds all @a ResolvedTopologicalBoundary objects as partitioning geometries.
		 */
		void
		add_partitioning_resolved_topological_boundaries(
				const std::vector<resolved_topological_boundary_non_null_ptr_type> &resolved_topological_boundaries);

		/**
		 * Adds all @a ResolvedTopologicalNetwork objects as partitioning geometries.
		 */
		void
		add_partitioning_resolved_topological_networks(
				const std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks);

		/**
		 * Adds all @a ResolvedTopologicalNetwork interior polygons, if any, as partitioning geometries.
		 */
		void
		add_partitioning_resolved_topological_network_interior_polygons(
				const std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks);

		/**
		 * Adds all @a ReconstructedFeatureGeometry objects in @a reconstruction, that have
		 * polygon geometry, as partitioning geometries.
		 */
		void
		add_partitioning_reconstructed_feature_polygons(
				const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries);
	};
}

#endif // GPLATES_APP_LOGIC_GEOMETRYCOOKIECUTTER_H
