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

#include "maths/PolygonIntersections.h"
#include "maths/PolygonOnSphere.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
	class ReconstructionGeometry;
	class ReconstructionGeometryCollection;
	class ResolvedTopologicalBoundary;

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
					const ReconstructionGeometry *reconstruction_geometry_) :
				reconstruction_geometry(reconstruction_geometry_)
			{ }

			const ReconstructionGeometry *reconstruction_geometry;
			partitioned_geometry_seq_type partitioned_geometries;
		};

		/**
		 * Typedef for a sequence of inside partitions.
		 */
		typedef std::list<Partition> partition_seq_type;


		/**
		 * Finds reconstructed polygon geometries to partition other geometry with.
		 *
		 * If @a partition_using_topological_plate_polygons is true then all
		 * @a ResolvedTopologicalBoundary objects found are used to partition geometry.
		 *
		 * If @a partition_using_static_polygons is true then all @a ReconstructedFeatureGeometry
		 * objects found, that contain polygon geometry, are used to partition geometry.
		 *
		 * By default only topological plate polygons are used to partition geometry.
		 * Both flags can be true (but probably not useful).
		 */
		GeometryCookieCutter(
				const ReconstructionGeometryCollection &reconstruction_geometry_collection,
				bool partition_using_topological_plate_polygons = true,
				bool partition_using_static_polygons = false);


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
					const ResolvedTopologicalBoundary *resolved_topological_boundary);

			PartitioningGeometry(
					const ReconstructedFeatureGeometry *reconstructed_feature_geometry,
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon);


			const ReconstructionGeometry *d_reconstruction_geometry;
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


		/**
		 * Adds all @a ResolvedTopologicalBoundary objects in @a reconstruction
		 * as partitioning geometries.
		 */
		void
		add_partitioning_resolved_topological_boundaries(
				const ReconstructionGeometryCollection &reconstruction_geometry_collection);

		/**
		 * Adds all @a ReconstructedFeatureGeometry objects in @a reconstruction, that have
		 * polygon geometry, as partitioning geometries.
		 */
		void
		add_partitioning_reconstructed_feature_polygons(
				const ReconstructionGeometryCollection &reconstruction_geometry_collection);

		/**
		 * Sorts the sequence of @a ReconstructionGeometry objects added with
		 * @a add_partitioning_resolved_topological_boundaries and
		 * @a add_partitioning_reconstructed_feature_polygons by plate id.
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
		 */
		void
		sort_partitioning_reconstructed_feature_polygons_by_plate_id();
	};
}

#endif // GPLATES_APP_LOGIC_GEOMETRYCOOKIECUTTER_H
