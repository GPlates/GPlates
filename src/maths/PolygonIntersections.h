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
 
#ifndef GPLATES_MATHS_POLYGONINTERSECTIONS_H
#define GPLATES_MATHS_POLYGONINTERSECTIONS_H

#include <list>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolygonOrientation.h"
#include "PolylineOnSphere.h"


namespace GPlatesMaths
{
	namespace PolylineIntersections
	{
		class Graph;
	}

	class GreatCircleArc;


	/**
	 * Partitions @a GeometryOnSphere derived types using a @a PolygonOnSphere into
	 * geometries that are inside or outside or both (they are clipped if they cross
	 * the polygon boundary).
	 */
	class PolygonIntersections
	{
	public:
		/**
		 * Typedef for a shared pointer to @a PolygonIntersections.
		 */
		typedef boost::shared_ptr<PolygonIntersections> non_null_ptr_type;


		/**
		 * Create with the polygon that will do the partitioning.
		 */
		static
		non_null_ptr_type
		create(
				 const PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon)
		{
			return non_null_ptr_type(new PolygonIntersections(partitioning_polygon));
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


		//! Typedef for a sequence of partitioned geometries.
		typedef std::list<GeometryOnSphere::non_null_ptr_to_const_type>
				partitioned_geometry_seq_type;

		//! Typedef for a sequence of partitioned polylines.
		typedef std::list<PolylineOnSphere::non_null_ptr_to_const_type>
				partitioned_polyline_seq_type;


		/**
		 * Returns the partitioning polygon (passed into constructor).
		 */
		const PolygonOnSphere::non_null_ptr_to_const_type &
		get_partitioning_polygon() const
		{
			return d_partitioning_polygon;
		}


		/**
		 * Partition @a geometry_to_be_partitioned into geometries inside and outside
		 * the partitioning polygon.
		 */
		Result
		partition_geometry(
				const GeometryOnSphere::non_null_ptr_to_const_type &geometry_to_be_partitioned,
				partitioned_geometry_seq_type &partitioned_geometries_inside,
				partitioned_geometry_seq_type &partitioned_geometries_outside) const;


		/**
		 * Partition @a polyline_to_be_partitioned into polylines inside and outside
		 * the partitioning polygon.
		 */
		Result
		partition_polyline(
				const PolylineOnSphere::non_null_ptr_to_const_type &polyline_to_be_partitioned,
				partitioned_polyline_seq_type &partitioned_polylines_inside,
				partitioned_polyline_seq_type &partitioned_polylines_outside) const;


		/**
		 * Partition @a polygon_to_be_partitioned into either polylines inside and outside
		 * the partitioning polygon or neither if it was fully outside or inside.
		 *
		 * If no intersections occurred then @a GEOMETRY_INSIDE or @a GEOMETRY_OUTSIDE
		 * is returned and the lists @a partitioned_polylines_inside and
		 * @a partitioned_polylines_outside are not appended to - they can't be because
		 * they contain polylines whereas the geometry being partitioned is a polygon.
		 *
		 * If both @a partitioned_polylines_inside and @a partitioned_polylines_outside
		 * are empty on returning then the returned result is guaranteed to be one
		 * of @a GEOMETRY_INSIDE or @a GEOMETRY_OUTSIDE - in which case it is the caller's
		 * responsibility to add @a polygon_to_be_partitioned to their own list of
		 * inside/outside polygon's if they choose to do so.
		 */
		Result
		partition_polygon(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon_to_be_partitioned,
				partitioned_polyline_seq_type &partitioned_polylines_inside,
				partitioned_polyline_seq_type &partitioned_polylines_outside) const;


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
		 */
		Result
		partition_multipoint(
				const MultiPointOnSphere::non_null_ptr_to_const_type &multipoint_to_be_partitioned,
				boost::optional<MultiPointOnSphere::non_null_ptr_to_const_type> &partitioned_multipoint_inside,
				boost::optional<MultiPointOnSphere::non_null_ptr_to_const_type> &partitioned_multipoint_outside) const;

	private:
		PolygonOnSphere::non_null_ptr_to_const_type d_partitioning_polygon;
		PolygonOrientation::Orientation d_partitioning_polygon_orientation;


		/**
		 * Construct with the polygon that will do the partitioning.
		 */
		PolygonIntersections(
				 const PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon);

		Result
		partition_polyline_or_polygon_fully_inside_or_outside(
				const GPlatesMaths::PointOnSphere &arbitrary_point_on_geometry1,
				const GPlatesMaths::PointOnSphere &arbitrary_point_on_geometry2) const;

		/**
		 * Determines which partitioned polylines are inside/outside the
		 * partitioning polygon and appends to the appropriate partition list.
		 */
		void
		partition_intersecting_geometry(
				const GPlatesMaths::PolylineIntersections::Graph &partitioned_polylines_graph,
				partitioned_polyline_seq_type &partitioned_polylines_inside,
				partitioned_polyline_seq_type &partitioned_polylines_outside) const;

		bool
		is_gca_inside_partitioning_polygon(
				const GreatCircleArc &polyline_gca,
				const GreatCircleArc &polygon_prev_gca,
				const GreatCircleArc &polygon_next_gca,
				bool reverse_inside_outside) const;
	};
}

#endif // GPLATES_MATHS_POLYGONINTERSECTIONS_H
