/* $Id$ */


/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_POLYGONPARTITIONER_H
#define GPLATES_MATHS_POLYGONPARTITIONER_H

#include <list>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GeometryOnSphere.h"
#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolygonOverlay.h"
#include "PolylineOnSphere.h"


namespace GPlatesMaths
{
	/**
	 * Partitions @a GeometryOnSphere derived types using a @a PolygonOnSphere into
	 * geometries that are inside or outside or both (they are clipped if they cross
	 * the polygon boundary).
	 *
	 * A polygon or polyline is cut by the overlay (PolygonOverlay::Overlay): a polygon into
	 * polygons with holes, inside (the intersection) and outside (the difference), with both
	 * polygons' interior rings taking part; a polyline into runs inside and outside. A geometry
	 * whose pieces would all be on one side (one that only touches the partitioning polygon,
	 * from either side, runs along its boundary, or is identical to it) is not cut: the
	 * geometry itself is returned, inside or outside (phase4b-design.md section 5.3). A
	 * polyline piece along the boundary is inside.
	 */
	class PolygonPartitioner
	{
	public:
		/**
		 * Typedef for a shared pointer to @a PolygonPartitioner.
		 */
		typedef boost::shared_ptr<PolygonPartitioner> non_null_ptr_type;


		/**
		 * Create with the polygon that will do the partitioning.
		 *
		 * @a partition_point_speed_and_memory determines the speed versus memory trade-off of the
		 * point-in-polygon tests against the partitioning polygon: those of @a partition_point,
		 * and the overlay's tests of a cut geometry's pieces.
		 *
		 * @a sliver_min_width, if set, turns on the overlay's sliver filter for a cut polygon's
		 * pieces (PolygonOverlay::Options::sliver_min_width, phase4b-design.md section 4.5): a ring
		 * whose mean width is below it, in radians, is dropped. The result is then decided after the
		 * filter: a side it empties leaves the polygon whole on the other side (on the side that lost
		 * the larger area if it empties both).
		 */
		static
		non_null_ptr_type
		create(
				 const PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon,
				 PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory = PolygonOnSphere::ADAPTIVE,
				 boost::optional<double> sliver_min_width = boost::none)
		{
			return non_null_ptr_type(
					new PolygonPartitioner(partitioning_polygon, partition_point_speed_and_memory, sliver_min_width));
		}


		/**
		 * The result of partitioning a geometry against the partitioning polygon.
		 */
		enum Result
		{
			//! Geometry is fully inside the partitioning polygon.
			GEOMETRY_INSIDE,

			//! Geometry is fully outside the partitioning polygon.
			GEOMETRY_OUTSIDE,

			//! Geometry intersects with the boundary of the partitioning polygon.
			GEOMETRY_INTERSECTING
		};


		//! Where each point of a geometry came from (phase4b-design.md section 7).
		typedef PolygonOverlay::provenance_seq_type provenance_seq_type;


		/**
		 * A partitioned geometry, and where each of its points came from when that was asked for
		 * (phase4b-design.md section 8.2).
		 */
		class PartitionedGeometry
		{
		public:
			explicit
			PartitionedGeometry(
					const GeometryOnSphere::non_null_ptr_to_const_type &geometry_,
					const boost::shared_ptr<const provenance_seq_type> &provenance_ =
							boost::shared_ptr<const provenance_seq_type>()) :
				geometry(geometry_),
				provenance(provenance_)
			{  }

			GeometryOnSphere::non_null_ptr_to_const_type geometry;

			/**
			 * One per point of @a geometry (GeometryUtils::get_geometry_points order), relative to
			 * the geometry partitioning started from (through a cascade, section 7.2); null unless
			 * asked for. A subject location is a vertex index (ring 0) for a point or multi-point.
			 */
			boost::shared_ptr<const provenance_seq_type> provenance;
		};

		//! Typedef for a sequence of partitioned geometries.
		typedef std::list<PartitionedGeometry> partitioned_geometry_seq_type;

		//! Typedef for a sequence of partitioned polygons.
		typedef std::list<PolygonOnSphere::non_null_ptr_to_const_type> partitioned_polygon_seq_type;

		//! Typedef for a sequence of partitioned polylines.
		typedef std::list<PolylineOnSphere::non_null_ptr_to_const_type> partitioned_polyline_seq_type;

		//! Typedef for a sequence of partitioned points.
		typedef std::vector<PointOnSphere> partitioned_point_seq_type;


		/**
		 * What the provenance of a cascade of partitioners needs (phase4b-design.md section 7.2),
		 * beyond the input's own table.
		 */
		class CascadeProvenance
		{
		public:
			/**
			 * The geometry the cascade started from, to which the input's table refers (none if
			 * the input has no table: it is then the original itself). It must outlive the call.
			 */
			const GeometryOnSphere *original_geometry = nullptr;

			/**
			 * The cascade's partitioning polygons by index (this one at @a clip_index), or null.
			 * They let an edge along an earlier cut be told from a lost location
			 * (PolygonOverlay::Options::cascade_clips); they must outlive the call.
			 */
			const std::vector<const PolygonOnSphere *> *partitioning_polygons = nullptr;

			//! This partitioner's index in the cascade, recorded in each clip location it makes.
			unsigned int clip_index = 0;
		};


		/**
		 * Returns the partitioning polygon (passed into constructor).
		 */
		const PolygonOnSphere::non_null_ptr_to_const_type &
		get_partitioning_polygon() const
		{
			return d_partitioning_polygon;
		}


		//! The speed of point-in-polygon tests against the partitioning polygon (see @a create).
		PolygonOnSphere::PointInPolygonSpeedAndMemory
		get_partition_point_speed_and_memory() const
		{
			return d_partition_point_speed_and_memory;
		}


		//! The sliver filter's width, if it is on (see @a create).
		const boost::optional<double> &
		get_sliver_min_width() const
		{
			return d_sliver_min_width;
		}


		/**
		 * Partition @a geometry_to_be_partitioned into geometries inside and outside
		 * the partitioning polygon.
		 *
		 * A polygon cut by the partitioning polygon comes back as polygons (with holes), a polyline
		 * as polylines, a multi-point as one multi-point inside and one outside; a geometry that
		 * is wholly inside or outside (section 5.3) comes back itself.
		 *
		 * The result is the same with or without lists (phase4b-design.md section 5.3): with
		 * neither (the result-only path, section 10.2), the cut is made the same way and only
		 * its output geometries are not built.
		 *
		 * @throws PolygonOverlay::InconsistentOverlayException if the overlay's classes are
		 * inconsistent (phase4b-design.md section 9.2), with or without lists.
		 */
		Result
		partition_geometry(
				const GeometryOnSphere::non_null_ptr_to_const_type &geometry_to_be_partitioned,
				boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_inside = boost::none,
				boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_outside = boost::none) const;


		/**
		 * As @a partition_geometry, with each output geometry's provenance (a table per output
		 * geometry), composed with @a geometry_to_be_partitioned's own table when it has one (a
		 * piece of an earlier partitioner in a cascade, @a cascade_provenance).
		 *
		 * @throws PolygonOverlay::InconsistentOverlayException as @a partition_geometry does.
		 */
		Result
		partition_geometry(
				const PartitionedGeometry &geometry_to_be_partitioned,
				boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_inside,
				boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_outside,
				const CascadeProvenance &cascade_provenance) const;


		/**
		 * Partition @a polyline_to_be_partitioned into polylines inside and outside
		 * the partitioning polygon (the polyline itself if it is wholly inside or outside).
		 *
		 * @throws PolygonOverlay::InconsistentOverlayException as @a partition_geometry does.
		 */
		Result
		partition_polyline(
				const PolylineOnSphere::non_null_ptr_to_const_type &polyline_to_be_partitioned,
				boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_inside = boost::none,
				boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_outside = boost::none) const;


		/**
		 * Partition @a polygon_to_be_partitioned into polygons (with holes) inside and outside the
		 * partitioning polygon (the polygon itself if it is wholly inside or outside).
		 *
		 * The result is the same with or without lists (see @a partition_geometry).
		 *
		 * @throws PolygonOverlay::InconsistentOverlayException as @a partition_geometry does.
		 */
		Result
		partition_polygon(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon_to_be_partitioned,
				boost::optional<partitioned_polygon_seq_type &> partitioned_polygons_inside = boost::none,
				boost::optional<partitioned_polygon_seq_type &> partitioned_polygons_outside = boost::none) const;


		/**
		 * Returns whether @a point_to_be_partitioned is inside, outside or
		 * on the boundary of the partitioning polygon.
		 */
		Result
		partition_point(
				const PointOnSphere &point_to_be_partitioned) const;


		/**
		 * Partition @a multipoint_to_be_partitioned into an optional multipoint inside and
		 * an optional multipoint outside the partitioning polygon.
		 *
		 * @a GEOMETRY_INTERSECTING is returned if any points were on the boundary
		 * of the partitioning polygon or if points were partitioned both inside and outside.
		 *
		 * Example usage:
		 *   partitioned_point_seq_type partitioned_points_inside;
		 *   partitioned_point_seq_type partitioned_points_outside;
		 *   PolygonPartitioner::Result result = polygon_partitioner.partition_multipoint(
		 *	      multipoint,
		 *	      partitioned_points_inside,
		 *	      partitioned_points_outside);
		 *	 if (!partitioned_points_inside.empty())
		 *	 {
		 *	      // Some of 'multipoint's points are inside the polygon.
		 *	 }
		 *	 if (!partitioned_points_outside.empty())
		 *	 {
		 *	      // Some of 'multipoint's points are outside the polygon.
		 *	 }
		 */
		Result
		partition_multipoint(
				const MultiPointOnSphere::non_null_ptr_to_const_type &multipoint_to_be_partitioned,
				boost::optional<partitioned_point_seq_type &> partitioned_points_inside = boost::none,
				boost::optional<partitioned_point_seq_type &> partitioned_points_outside = boost::none) const;


		/**
		 * The provenance of a geometry relative to itself: each point its own vertex (ring 0 for
		 * a point, multi-point or polyline; a polygon's interior ring 'i' is ring 'i + 1').
		 */
		static
		boost::shared_ptr<const provenance_seq_type>
		create_identity_provenance(
				const GeometryOnSphere &geometry);

	private:
		PolygonOnSphere::non_null_ptr_to_const_type d_partitioning_polygon;
		PolygonOnSphere::PointInPolygonSpeedAndMemory d_partition_point_speed_and_memory;
		boost::optional<double> d_sliver_min_width;


		/**
		 * Construct with the polygon that will do the partitioning.
		 */
		PolygonPartitioner(
				 const PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon,
				 PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory,
				 boost::optional<double> sliver_min_width);
	};
}

#endif // GPLATES_MATHS_POLYGONPARTITIONER_H
