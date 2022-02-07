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

#ifndef GPLATES_MATHS_DATELINEWRAPPER_H
#define GPLATES_MATHS_DATELINEWRAPPER_H

#include <bitset>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/shared_ptr.hpp>

#include "AngularExtent.h"
#include "LatLonPoint.h"
#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "Rotation.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"
#include "utils/SmartNodeLinkedList.h"


namespace GPlatesMaths
{
	class BoundingSmallCircle;

	/**
	 * Clips polyline/polygon geometries to the dateline (at -180, or +180, degrees longitude) and
	 * wraps them to the opposite longitude so that they display correctly over a (-180,180)
	 * rectangular (lat/lon) projection.
	 *
	 * The motivation for this class is to remove horizontal lines across the display in ArcGIS
	 * for geometries that intersect the dateline.
	 *
	 * NOTE: This is a class instead of global functions in case certain things can be re-used
	 * such as vertex memory pools, etc.
	 *
	 * The algorithms and data structures used here are based on the following paper:
	 *
	 *   Greiner, G., Hormann, K., 1998.
	 *   Efficient clipping of arbitrary polygons.
	 *   Association for Computing Machinery—Transactions on Graphics 17 (2), 71–83.
	 *
	 * ...and, to a lesser extent,...
	 *
	 *   Liu, Y.K.,Wang,X.Q.,Bao,S.Z.,Gombosi, M.,Zalik, B.,2007.
	 *   An algorithm for polygon clipping, and for determining polygon intersections and unions.
	 *   Computers&Geosciences33,589–598.
	 */
	class DateLineWrapper :
			public GPlatesUtils::ReferenceCount<DateLineWrapper>
	{
	private:
		class LatLonLineGeometry;

	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<DateLineWrapper> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const DateLineWrapper> non_null_ptr_to_const_type;


		//! Typedef for a sequence of lat/lon points.
		typedef std::vector<LatLonPoint> lat_lon_points_seq_type;


		/**
		 * A wrapped lat/lon polygon.
		 */
		class LatLonPolygon
		{
		public:

			/**
			 * Each point in a ring of the wrapped polygon (excluding tessellated dateline points) can be
			 * considered to be an interpolation of two end points of an original polygon ring segment.
			 *
			 * This is useful when client needs to, for example, interpolate scalars values defined at
			 * the original points onto the new (wrapped and tessellated) points.
			 */
			struct InterpolateOriginalSegment
			{
				InterpolateOriginalSegment(
						const double &interpolate_ratio_,
						unsigned int original_segment_index_,
						unsigned int original_ring_index_) :
					interpolate_ratio(interpolate_ratio_),
					original_segment_index(original_segment_index_),
					original_ring_index(original_ring_index_)
				{  }

				/**
				 * Value in range [0, 1] where 0 represents the segment start point and 1 the segment
				 * end point - so to interpolate scalars use the formula:
				 *
				 *   scalar_lerp = scalar_at_start_point +
				 *			interpolate_ratio * (scalar_at_end_point - scalar_at_start_point)
				 */
				double interpolate_ratio;

				//! Index into the segments (great circle arcs) of a ring of the original polygon.
				unsigned int original_segment_index;

				/**
				 * Index into the rings of the original polygon.
				 *
				 * Index 0 is the exterior ring and indices 1, 2, ... are the interior rings.
				 */
				unsigned int original_ring_index;
			};

			/**
			 * Typedef for a sequence of optional @a InterpolateOriginalSegment.
			 *
			 * Entries that are 'boost::none' correspond to points in polygon rings that are not
			 * on any original ring segments (instead they are tessellated points along the dateline).
			 */
			typedef std::vector< boost::optional<InterpolateOriginalSegment> > interpolate_original_segment_seq_type;


			/**
			 * Each point has various properties.
			 *
			 * Note: A point can be tessellated along the dateline and not be on an original segment.
			 */
			enum PointType
			{
				ORIGINAL_POINT,      // Point exists in the original polyline.
				TESSELLATED_POINT,   // A new tessellated point (not an original point or dateline intersection point).
				ON_DATELINE,         // Point lies on the dateline.
				ON_ORIGINAL_SEGMENT, // Point lies on a segment of original polyline.

				NUM_POINT_TYPES // Must be the last enum.
			};

			/**
			 * A std::bitset for specifying which point properties to assign.
			 */
			typedef std::bitset<NUM_POINT_TYPES> point_flags_type;


			/**
			 * The points in the exterior ring of the dateline wrapped (and optionally tessellated) polygon.
			 *
			 * This is the original (unwrapped) points plus new points added due to intersection
			 * with the dateline and any new points due to tessellation.
			 *
			 * NOTE: The start and end points are generally *not* the same.
			 * So if you are rendering the polygon you may need to explicitly close the polygon by
			 * appending the start point.
			 * For example, a triangle has three points but it can be rendered as a line string
			 * consisting of four points (the last point being a duplicate of the first point).
			 */
			const lat_lon_points_seq_type &
			get_exterior_ring_points() const;

			/**
			 * Boolean flags to indicate various properties of each point in @a get_exterior_ring_points (at same index).
			 *
			 * Note: The indices into the returned array match indices into @a get_exterior_ring_points
			 * and not the original points in the exterior ring of the original polygon.
			 */
			void
			get_exterior_ring_point_flags(
					std::vector<point_flags_type> &exterior_ring_point_flags) const;

			/**
			 * Information on the interpolation of points in the wrapped exterior ring from the
			 * original segments in the (unwrapped and untessellated) polygon.
			 *
			 * Valid entries only apply to points with the @a ON_ORIGINAL_SEGMENT flag set.
			 * Entries that are 'boost::none' correspond to points in @a get_exterior_ring_points that are
			 * not on any original ring segments (instead they are tessellated points along the dateline).
			 *
			 * This is useful when client needs to, for example, interpolate scalars values defined at
			 * the original points onto the new (wrapped and tessellated) points.
			 *
			 * Note: The indices into the returned array match indices into @a get_exterior_ring_points
			 * and not the original points in the exterior ring of the original polygon.
			 */
			void
			get_exterior_ring_interpolate_original_segments(
					interpolate_original_segment_seq_type &exterior_ring_interpolate_original_segments) const;


			/**
			 * Returns the number of interior rings (if any).
			 */
			unsigned int
			get_num_interior_rings() const;

			/**
			 * Same as @a get_exterior_ring_points except refers to the points in the interior ring of the
			 * dateline wrapped (and optionally tessellated) polygon at the specified interior ring index.
			 */
			const lat_lon_points_seq_type &
			get_interior_ring_points(
					unsigned int interior_ring_index) const;

			/**
			 * Boolean flags to indicate various properties of each point in @a get_interior_ring_points
			 * (at the same interior ring index and same point index within that ring).
			 *
			 * Note: Points in @a get_interior_ring_points can be points from the original polygon's
			 * exterior and interior rings because a single clipped/dateline-wrapped polygon can
			 * follow part of an original exterior ring and part of an original interior ring.
			 *
			 * Note: The indices into the returned array match indices into @a get_interior_ring_points
			 * and not the original points in associated interior ring of the original polygon.
			 */
			void
			get_interior_ring_point_flags(
					std::vector<point_flags_type> &interior_ring_point_flags,
					unsigned int interior_ring_index) const;

			/**
			 * Information on the interpolation of points in the wrapped interior ring from the
			 * original segments in the (unwrapped and untessellated) polygon.
			 *
			 * Valid entries only apply to points with the @a ON_ORIGINAL_SEGMENT flag set.
			 * Entries that are 'boost::none' correspond to points in @a get_interior_ring_points that are
			 * not on any original ring segments (instead they are tessellated points along the dateline).
			 *
			 * This is useful when client needs to, for example, interpolate scalars values defined at
			 * the original points onto the new (wrapped and tessellated) points.
			 *
			 * Note: The indices into the returned array match indices into @a get_interior_ring_points
			 * and not the original points in the interior ring of the original polygon.
			 */
			void
			get_interior_ring_interpolate_original_segments(
					interpolate_original_segment_seq_type &interior_ring_interpolate_original_segments,
					unsigned int interior_ring_index) const;

		private:

			LatLonPolygon();

			boost::shared_ptr<LatLonLineGeometry> d_exterior_ring_line_geometry;
			std::vector< boost::shared_ptr<LatLonLineGeometry> > d_interior_ring_line_geometries;

			friend class DateLineWrapper;
		};


		/**
		 * A wrapped lat/lon polyline.
		 */
		class LatLonPolyline
		{
		public:

			/**
			 * Each point in the wrapped polyline can be considered to be an interpolation of
			 * two end points of an original polyline segment.
			 *
			 * This is useful when client needs to, for example, interpolate scalars values defined at
			 * the original points onto the new (wrapped and tessellated) points.
			 */
			struct InterpolateOriginalSegment
			{
				InterpolateOriginalSegment(
						const double &interpolate_ratio_,
						unsigned int original_segment_index_) :
					interpolate_ratio(interpolate_ratio_),
					original_segment_index(original_segment_index_)
				{  }

				/**
				 * Value in range [0, 1] where 0 represents the segment start point and 1 the segment
				 * end point - so to interpolate scalars use the formula:
				 *
				 *   scalar_lerp = scalar_at_start_point +
				 *			interpolate_ratio * (scalar_at_end_point - scalar_at_start_point)
				 */
				double interpolate_ratio;

				//! Index into the segments (great circle arcs) of the original polyline.
				unsigned int original_segment_index;
			};

			//! Typedef for a sequence of optional @a InterpolateOriginalSegment.
			typedef std::vector<InterpolateOriginalSegment> interpolate_original_segment_seq_type;


			/**
			 * Each point has various properties.
			 */
			enum PointType
			{
				ORIGINAL_POINT,    // Point exists in the original polyline.
				TESSELLATED_POINT, // A new tessellated point (not an original point or dateline intersection point).
				ON_DATELINE,       // Point lies on the dateline.

				NUM_POINT_TYPES // Must be the last enum.
			};

			/**
			 * A std::bitset for specifying which point properties to assign.
			 */
			typedef std::bitset<NUM_POINT_TYPES> point_flags_type;


			/**
			 * The dateline wrapped (and optionally tessellated) points.
			 *
			 * This is the original (unwrapped) points plus new points added due to intersection
			 * with the dateline and any new points due to tessellation.
			 */
			const lat_lon_points_seq_type &
			get_points() const;

			/**
			 * Boolean flags to indicate various properties of each point in @a get_points (at same index).
			 */
			void
			get_point_flags(
					std::vector<point_flags_type> &point_flags) const;

			/**
			 * Information on the interpolation of points in the wrapped polyline from the
			 * original segments in the (unwrapped and untessellated) polyline.
			 *
			 * This is useful when client needs to, for example, interpolate scalars values defined at
			 * the original points onto the new (wrapped and tessellated) points.
			 *
			 * Note: The indices into the returned array match indices into @a get_points
			 * and not the original points in the original polyline.
			 */
			void
			get_interpolate_original_segments(
					interpolate_original_segment_seq_type &interpolate_original_segments) const;

		private:

			LatLonPolyline();

			boost::shared_ptr<LatLonLineGeometry> d_line_geometry;

			friend class DateLineWrapper;
		};


		/**
		 * A wrapped lat/lon multi-point.
		 */
		class LatLonMultiPoint
		{
		public:

			/**
			 * Wraps points in the range [-180 + central_meridian, central_meridian + 180].
			 */
			const lat_lon_points_seq_type &
			get_points() const
			{
				return *d_points;
			}

		private:

			LatLonMultiPoint() :
				d_points(new lat_lon_points_seq_type())
			{  }

			boost::shared_ptr<lat_lon_points_seq_type> d_points;

			friend class DateLineWrapper;
		};


		/**
		 * Creates a @a DateLineWrapper object.
		 *
		 * If @a central_meridian is non-zero then the 'dateline' is shifted such that the
		 * range of output longitudes is [-180 + central_meridian, central_meridian + 180].
		 * If @a central_meridian is zero then the output range becomes [-180, 180].
		 *
		 * NOTE: If @a central_meridian is outside the range [-180, 180] then it will be wrapped
		 * to be within that range. This ensures that the range of longitudes of output points
		 * [-180 + central_meridian, central_meridian + 180] will always be in the range [-360, 360]
		 * which is the valid range for @a LatLonPoint.
		 */
		static
		non_null_ptr_type
		create(
				const double &central_meridian = 0.0)
		{
			return non_null_ptr_type(new DateLineWrapper(central_meridian));
		}


		//
		// The following methods wrap a geometry-on-sphere to the dateline while converting from
		// cartesian (xyz) coordinates to latitude/longitude coordinates.
		//
		// The dateline is the great circle arc from north pole to south pole at longitude -180 degrees.
		// The dateline splits the entire globe into the longitude range (-180,180).
		// So longitudes greater than 180 wrap to -180 and longitudes less than -180 wrap to 180.
		//
		// However, if the 'central_meridian' passed into @a create is non-zero then the dateline is shifted
		// such that the range of output longitudes is [-180 + central_meridian, central_meridian + 180].
		//
		// Only polylines and polygons are clipped/intersected to the dateline.
		// The only effect on multi-points and points is to wrap the longitudes into the range
		// [-180 + central_meridian, central_meridian + 180].
		//
		// NOTE: If a polyline/polygon does *not* intersect the dateline then it is not clipped and
		// hence only one wrapped geometry is output (it is just the unclipped geometry converted
		// to lat/lon coordinates).
		//
		// NOTE: In some rare cases it's possible that the entire input polyline/polygon got swallowed
		// by the dateline. This can happen if it is entirely *on* the dateline which is considered
		// to be *outside* the dateline polygon (which covers the entire globe and 'effectively'
		// excludes a very thin area of size epsilon around the dateline arc).
		// In this situation the input polyline/polygon is passed directly to the output.
		//


		/**
		 * Clips the specified *polygon* to the dateline.
		 *
		 * This also wraps to the range [-180 + central_meridian, central_meridian + 180].
		 *
		 * If @a tessellate_threshold is specified then the arcs of the wrapped polygon are tessellated
		 * such that they do not exceed the threshold in arc length.
		 *
		 * If @a group_interior_with_exterior_rings is true and part of the input polygon intersects
		 * the dateline then each (non-intersecting) ring of the polygon (if any) is added as:
		 *  1) an exterior ring of a new output polygon:
		 *     - if it's not inside (or intersecting) the polygons output so far,
		 *  2) an interior ring of an existing output polygon:
		 *     - if it's inside (or intersecting) any polygon output so far.
		 * However this can slow down wrapping if there are lots of interior (non-intersecting) rings
		 * (and where some rings do intersect).
		 * Some clients do not need this grouping - in other words it doesn't matter if all the
		 * interior rings end up in separate polygons as exterior rings. An example is filled polygons
		 * in the 2D map views because it just takes all the rings regardless of whether they are
		 * exterior or interior rings and renders them the same way (inverting pixel's stencil buffer
		 * value each time pixel drawn takes care of correctly masking out polygon holes and intersections).
		 */
		void
		wrap_polygon(
				const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
				std::vector<LatLonPolygon> &wrapped_polygons,
				boost::optional<AngularExtent> tessellate_threshold = boost::none,
				bool group_interior_with_exterior_rings = true) const;

		/**
		 * Clips the specified *polyline* to the dateline.
		 *
		 * This also wraps to the range [-180 + central_meridian, central_meridian + 180].
		 *
		 * If @a tessellate_threshold is specified then the arcs of the wrapped polygon are tessellated
		 * such that they do not exceed the threshold in arc length.
		 */
		void
		wrap_polyline(
				const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline,
				std::vector<LatLonPolyline> &wrapped_polylines,
				boost::optional<AngularExtent> tessellate_threshold = boost::none) const;

		/**
		 * Wraps points in the specified *multi-point* to the range [-180 + central_meridian, central_meridian + 180].
		 */
		LatLonMultiPoint
		wrap_multi_point(
				const MultiPointOnSphere::non_null_ptr_to_const_type &input_multipoint) const;

		/**
		 * Wraps the specified *point* to the range [-180 + central_meridian, central_meridian + 180].
		 */
		LatLonPoint
		wrap_point(
				const PointOnSphere &input_point) const;


		//
		// The following early reject methods are not needed and only provided to support optimisations...
		//


		/**
		 * Returns true if the specified polyline can possibly intersect the (central meridian shifted) dateline arc.
		 *
		 * This can be used to avoid wrapping polylines that cannot possibly intersect the dateline.
		 *
		 * For example, this is used by the 2D map renderer to avoid converting to lat/lon (in dateline wrapper)
		 * then converting to x/y/z (to tessellate polyline segments) and then converting back to
		 * lat/lon prior to projection - instead unwrapped polylines can just be tessellated and then
		 * converted to lat/lon, saving expensive x/y/z <-> lat/lon conversions.
		 *
		 * NOTE: A return value of true does *not* necessarily mean the polyline intersects the dateline.
		 * However false always means the polyline does *not* intersect the dateline.
		 */
		bool
		possibly_wraps(
				const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline) const;

		/**
		 * Returns true if the specified polygon can possibly intersect the (central meridian shifted) dateline arc.
		 *
		 * See the other overload of @a possibly_wraps for more details.
		 */
		bool
		possibly_wraps(
				const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon) const;

	private:

		/**
		 * For non-zero central meridians we need to rotate geometries into a dateline reference
		 * frame for clipping/wrapping and then reverse the rotation when outputting wrapped geometries.
		 */
		struct CentralMeridian
		{
			explicit
			CentralMeridian(
					const double &longitude_);

			/**
			 * The central meridian longitude.
			 */
			double longitude;

			/**
			 * Rotation *to* central meridian (of map projection) with non-zero longitude to place geometries
			 * in a reference frame centred at longitude zero (where the dateline wrapper can be used).
			 */
			Rotation rotate_to_dateline_frame;

			/**
			 * Rotation *from* central meridian (of map projection) with non-zero longitude.
			 *
			 * This is the reverse of @a rotate_to_dateline_frame.
			 */
			Rotation rotate_from_dateline_frame;
		};


		/**
		 * A possibly tessellated sequence of points (for a polyline/polygon).
		 */
		class LatLonLineGeometry
		{
		public:

			/**
			 * See LatLonPolyline::InterpolateOriginalSegment and LatLonPolygon::InterpolateOriginalSegment
			 * for more details.
			 */
			struct InterpolateOriginalSegment
			{
				InterpolateOriginalSegment(
						const double &interpolate_ratio_,
						unsigned int original_segment_index_,
						unsigned int original_geometry_part_index_) :
					interpolate_ratio(interpolate_ratio_),
					original_segment_index(original_segment_index_),
					original_geometry_part_index(original_geometry_part_index_)
				{  }

				double interpolate_ratio;
				unsigned int original_segment_index;
				unsigned int original_geometry_part_index; //!< Only applies to polygons (and is the ring index).
			};

			/**
			 * Typedef for a sequence of optional @a InterpolateOriginalSegment.
			 *
			 * Entries that are 'boost::none' correspond to points in polygon rings that are not
			 * on any original ring segments (instead they are tessellated points along the dateline).
			 *
			 * For polylines all entries are valid.
			 */
			typedef std::vector< boost::optional<InterpolateOriginalSegment> > interpolate_original_segment_seq_type;


			/**
			 * Each point has various properties.
			 *
			 * Note: For polygon rings, a point can be tessellated along the dateline and not be on an original segment.
			 */
			enum PointType
			{
				ORIGINAL_POINT,      // Point exists in the original polyline/polygon.
				TESSELLATED_POINT,   // A new tessellated point (not an original point or dateline intersection point).
				ON_DATELINE,         // Point lies on the dateline.
				ON_ORIGINAL_SEGMENT, // Point lies on a segment of original polygon (note: always true for polylines).

				NUM_POINT_TYPES // Must be the last enum.
			};

			/**
			 * A std::bitset for specifying which point properties to assign.
			 */
			typedef std::bitset<NUM_POINT_TYPES> point_flags_type;


			/**
			 * Add a point to this line geometry (polyline/polygon).
			 *
			 * It will also tessellate arcs if requested.
			 */
			void
			add_point(
					const LatLonPoint &lat_lon_point,
					const PointOnSphere &point,
					const double &central_meridian_longitude,
					const boost::optional<AngularExtent> &tessellate_threshold,
					bool is_unwrapped_point,
					bool on_dateline,
					boost::optional<InterpolateOriginalSegment> interpolate_original_segment,
					bool is_end_of_dateline_segment = false);

			/**
			 * Call once finished adding points, if this line geometry is a *polygon ring* and it's being tessellated.
			 *
			 * This is only necessary if tessellation is required.
			 *
			 * The last arc of a polygon ring is between it's last and first points.
			 * If this last arc is tessellated then new tessellated points may get added after
			 * the last point added via @a add_point.
			 */
			void
			finish_tessellating_polygon_ring(
					const double &central_meridian_longitude,
					const AngularExtent &tessellate_threshold);

			/**
			 * The dateline wrapped (and tessellated) points.
			 */
			const lat_lon_points_seq_type &
			get_points() const
			{
				return d_lat_lon_points;
			}

			/**
			 * Boolean flags to indicate various properties of each point in @a get_points (at same index).
			 */
			const std::vector<point_flags_type> &
			get_point_flags() const
			{
				return d_point_flags;
			}

			/**
			 * Information on the interpolation of points in @a get_points (at same index) from the
			 * original segments in the original (unwrapped and untessellated) line geometry.
			 */
			const interpolate_original_segment_seq_type &
			get_points_interpolate_original_segments() const
			{
				return d_points_interpolate_original_segments;
			}

			/**
			 * Return the original, untessellated points.
			 *
			 * Currently only used to determine if polygon rings should be interiors of other rings.
			 */
			const std::vector<PointOnSphere> &
			get_untessellated_points_on_sphere() const
			{
				return d_untessellated_points;
			}

		private:

			struct UntessellatedPointInfo
			{
				UntessellatedPointInfo(
						const LatLonPoint &lat_lon_point_,
						const PointOnSphere &point_,
						bool is_unwrapped_point_,
						bool on_dateline_,
						boost::optional<InterpolateOriginalSegment> interpolate_original_segment_,
						bool is_end_of_dateline_segment_) :
					lat_lon_point(lat_lon_point_),
					point(point_),
					is_unwrapped_point(is_unwrapped_point_),
					on_dateline(on_dateline_),
					interpolate_original_segment(interpolate_original_segment_),
					is_end_of_dateline_segment(is_end_of_dateline_segment_)
				{  }

				LatLonPoint lat_lon_point;
				PointOnSphere point;
				bool is_unwrapped_point;
				bool on_dateline;
				boost::optional<InterpolateOriginalSegment> interpolate_original_segment;
				bool is_end_of_dateline_segment;
			};


			/**
			 * All the lat/lon points (tessellated and untessellated).
			 */
			lat_lon_points_seq_type d_lat_lon_points;
			std::vector<point_flags_type> d_point_flags;
			interpolate_original_segment_seq_type d_points_interpolate_original_segments;

			/**
			 * We also keep track of the original, untessellated points.
			 *
			 * Currently only used to determine if polygon rings should be interiors of other rings.
			 */
			std::vector<PointOnSphere> d_untessellated_points;

			// Previous untessellated point information (could be a wrapped or unwrapped point).
			boost::optional<UntessellatedPointInfo> d_previous_untessellated_point_info;

			// Start (untessellated) point information of line geometry (could be a wrapped or unwrapped point).
			boost::optional<UntessellatedPointInfo> d_start_point_info;


			void
			add_untessellated_point(
					const UntessellatedPointInfo &untessellated_point_info);

			void
			add_tessellated_points(
					const UntessellatedPointInfo &untessellated_point_info,
					const double &central_meridian_longitude,
					const AngularExtent &tessellate_threshold);

			void
			add_tessellated_point(
					const LatLonPoint &lat_lon_point,
					bool on_dateline,
					const boost::optional<InterpolateOriginalSegment> &interpolate_original_segment);
		};


		/**
		 * Classification of a vertex based on its position relative to the dateline.
		 */
		enum VertexClassification
		{
			// In front of the plane containing the dateline arc.
			CLASSIFY_FRONT,

			// Behind the plane containing the dateline arc.
			CLASSIFY_BACK,

			// On the 'thick' (epsilon) plane containing the dateline arc, but off the dateline
			// arc itself (ie, roughly longitude of zero degrees).
			CLASSIFY_OFF_DATELINE_ARC_ON_PLANE,

			// On the 'thick' (epsilon) plane containing the dateline arc and on the dateline
			// arc itself (ie, roughly longitude of -/+ 180 degrees).
			CLASSIFY_ON_DATELINE_ARC,

			// Within the 'thick' point (epsilon circle) at the north pole.
			// Basically within epsilon from the north pole.
			CLASSIFY_ON_NORTH_POLE,

			// Within the 'thick' point (epsilon circle) at the south pole.
			// Basically within epsilon from the south pole.
			CLASSIFY_ON_SOUTH_POLE
		};


		/**
		 * The type of intersection when intersecting a line segment of a geometry with the dateline.
		 */
		enum IntersectionType
		{
			INTERSECTED_DATELINE,
			INTERSECTED_NORTH_POLE,
			INTERSECTED_SOUTH_POLE
		};


		/**
		 * A vertex in the graph.
		 */
		struct Vertex
		{
			explicit
			Vertex(
					const LatLonPoint &lat_lon_point_,
					// Specifying 'none' just requests the point be created from 'lat_lon_point_'...
					boost::optional<const PointOnSphere &> point_ = boost::none,
					bool is_unwrapped_point_ = false,
					boost::optional<LatLonLineGeometry::InterpolateOriginalSegment> interpolate_original_segment_ = boost::none,
					bool is_intersection_ = false,
					bool exits_other_polygon_ = false) :
				lat_lon_point(lat_lon_point_),
				point(point_ ? point_.get() : make_point_on_sphere(lat_lon_point_)),
				intersection_neighbour(NULL),
				intersection_neighbour_list(NULL),
				is_unwrapped_point(is_unwrapped_point_),
				interpolate_original_segment(interpolate_original_segment_),
				is_intersection(is_intersection_),
				exits_other_polygon(exits_other_polygon_),
				used_to_output_polygon(false)
			{  }


			LatLonPoint lat_lon_point;
			PointOnSphere point;

			/**
			 * References the other polygon if this is an intersection vertex, otherwise NULL.
			 *
			 * Since the dateline is treated as a polygon it can reference the geometry (polygon) and vice versa.
			 *
			 * Note that for polylines this is always NULL since traversal of the dateline is not needed.
			 *
			 * NOTE: Due to cyclic dependencies this is a 'void *' instead of a 'vertex_list_type::Node *'.
			 */
			void/*vertex_list_type::Node*/ *intersection_neighbour;
			void/*vertex_list_type*/ *intersection_neighbour_list;

			/**
			 * Is true if this vertex represents a point in the original geometry before it was wrapped.
			 */
			bool is_unwrapped_point;

			/*
			 * This vertex can be considered an interpolation of a segment's (GreatCircleArc) start and
			 * end points by an interpolation ratio.
			 *
			 * Is none if vertex is not on a segment of the original geometry.
			 */
			boost::optional<LatLonLineGeometry::InterpolateOriginalSegment> interpolate_original_segment;

			/**
			 * Is true if this vertex represents an intersection of geometry with the dateline.
			 *
			 * NOTE: This can be true when @a intersection_neighbour is NULL if geometry is a *polyline*.
			 * This is because polylines don't use, and hence don't initialise, @a intersection_neighbour.
			 */
			bool is_intersection;

			/**
			 * Is true if this vertex is an intersection that exits the other polygon.
			 *
			 * For a polygon geometry the other polygon is the dateline polygon.
			 * And for the dateline polygon the other polygon is the geometry polygon.
			 * Note that this is not used when the geometry is a polyline.
			 */
			bool exits_other_polygon;

			/**
			 * Is true if this vertex has been used to output a polygon.
			 *
			 * This is used to track which output polygons have been output.
			 */
			bool used_to_output_polygon;
		};

		//! Typedef for a double-linked list of vertices.
		typedef GPlatesUtils::SmartNodeLinkedList<Vertex> vertex_list_type;

		//! Typedef for a pool allocator of vertex list nodes.
		typedef boost::object_pool<vertex_list_type::Node> vertex_node_pool_type;


		/**
		 * A graph of a geometry potentially intersecting the dateline.
		 *
		 * The dateline is treated as an infinitesimally thin polygon so that traditional
		 * polygon-polygon clipping can be performed (when the geometry is a polygon and not a polyline).
		 */
		class IntersectionGraph :
				boost::noncopyable
		{
		public:

			enum IntersectionResult
			{
				INTERSECTS_DATELINE,
				ENTIRELY_OFF_DATELINE,
				ENTIRELY_ON_DATELINE
			};

			explicit
			IntersectionGraph(
					// Is this graph used to clip a polygon ?
					bool is_polygon_graph_);

			/**
			 * Adds a line geometry (polyline or ring from a polygon).
			 *
			 * NOTE: The line geometry *must* be in the dateline reference frame where points at the
			 * central meridian have a longitude of zero.
			 */
			template <typename LineSegmentForwardIter>
			void
			add_line_geometry(
					LineSegmentForwardIter const dateline_frame_line_segments_begin,
					LineSegmentForwardIter const dateline_frame_line_segments_end,
					bool is_polygon_ring);

			/**
			 * Traverses geometry vertices (and intersection vertices) in graph and outputs polylines
			 * that either intersect the dateline are are entirely *off* the dateline.
			 *
			 * Note: Polylines that are entirely *on* the dateline are not output.
			 */
			void
			output_polylines(
					std::vector<LatLonPolyline> &lat_lon_polylines,
					const boost::optional<CentralMeridian> &central_meridian,
					const boost::optional<AngularExtent> &tessellate_threshold);

			/**
			 * Traverses geometry/dateline vertices (and intersection vertices) in graph and
			 * outputs polygons that intersect the dateline.
			 *
			 * Note: Polygon rings that don't intersect the dateline are not output.
			 * Which means polygons rings that are entirely *off* or *on* the dateline are not output.
			 */
			void
			output_intersecting_polygons(
					std::vector<LatLonPolygon> &lat_lon_polygons,
					const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
					const boost::optional<CentralMeridian> &central_meridian,
					const boost::optional<AngularExtent> &tessellate_threshold);

			/**
			 * Get the intersection results for each line geometry.
			 *
			 * Indices into the results are based on the order in which @a add_line_geometry was called.
			 */
			void
			get_line_geometry_intersection_results(
					std::vector<IntersectionResult> &intersection_results) const;

			/**
			 * Output the line geometry of the non-intersecting polygon ring at the specified ring geometry index.
			 *
			 * @a ring_geometry_index is based on the order in which @a add_line_geometry was called.
			 *
			 * NOTE: Must have already called @a add_line_geometry for each ring in the polygon and the
			 * intersection result at index @a ring_geometry_index returned by
			 * @a get_line_geometry_intersection_results should not be 'INTERSECTS_DATELINE'.
			 */
			void
			output_non_intersecting_line_polygon_ring(
					LatLonLineGeometry &ring_line_geometry,
					unsigned int ring_geometry_index,
					const boost::optional<CentralMeridian> &central_meridian,
					const boost::optional<AngularExtent> &tessellate_threshold) const;

		private:

			//! The sentinel node in the vertex lists constructs an unused dummy vertex - the value doesn't matter.
			static const Vertex LISTS_SENTINEL;


			/**
			 * A line geometry containing vertices (and whether intersected dateline or not).
			 *
			 * A polyline has a single line geometry whereas a polygon has a line geometry for its
			 * exterior ring and one for each interior ring (if any).
			 */
			struct LineGeometry
			{
				LineGeometry() :
					vertices(new vertex_list_type(LISTS_SENTINEL)),
					intersects_dateline(false)
				{  }

				boost::shared_ptr<vertex_list_type> vertices; //!< Using shared_ptr since list is not copyable.
				bool intersects_dateline;
			};


			//! Allocates vertex list nodes - all nodes are freed when 'this' graph is destroyed.
			vertex_node_pool_type d_vertex_node_pool;

			/**
			 * The line geometries containing vertices (and intersection point copies).
			 *
			 * A polyline has a single line geometry whereas a polygon has a line geometry for its
			 * exterior ring and one for each interior ring (if any).
			 */
			std::vector<LineGeometry> d_line_geometries;

			/**
			 * Contains the four lat/lon corner dateline vertices (and any intersection point copies).
			 *
			 * NOTE: This list is empty if this graph is *not* a polygon graph (ie, if it's a polyline graph).
			 */
			vertex_list_type d_dateline_vertices;

			/*
			 * The four corner vertices represent the corners of the rectangular projection:
			 *   (-90,  180)
			 *   ( 90,  180)
			 *   ( 90, -180)
			 *   (-90, -180)
			 *
			 * The order is a clockwise order when looking at the dateline arc from above the globe.
			 *
			 *   NF -> NB
			 *   /\    |
			 *   |     \/
			 *   SF <- SB
			 *
			 *            dateline
			 *               |
			 *   <---   +180 | -180   --->
			 *               |
			 *
			 * NOTE: These are NULL if this graph is *not* a polygon graph (ie, if it's a polyline graph).
			 */
			vertex_list_type::Node *d_dateline_corner_south_front;
			vertex_list_type::Node *d_dateline_corner_north_front;
			vertex_list_type::Node *d_dateline_corner_north_back;
			vertex_list_type::Node *d_dateline_corner_south_back;

			//! If generating a graph for a polygon then @a dateline_vertices will be created at intersections.
			bool d_is_polygon_graph;

			//! Is true if geometry intersected or touched (within epsilon) the north pole.
			bool d_geometry_intersected_north_pole;
			//! Is true if geometry intersected or touched (within epsilon) the south pole.
			bool d_geometry_intersected_south_pole;


			//! Adds a line segment based on its vertex classifications.
			void
			add_line_segment(
					const GreatCircleArc &line_segment,
					unsigned int line_segment_index,
					unsigned int line_geometry_index,
					VertexClassification line_segment_start_vertex_classification,
					VertexClassification line_segment_end_vertex_classification);

			/**
			 * Returns intersection of line segment (line_segment_start_point, line_segment_end_point) with dateline, if any.
			 *
			 * If an intersection was found then type of intersection is returned in @a intersection_type and
			 * the interpolation ratio of intersection point between the segment's start and end points
			 * is returned in @a segment_interpolation_ratio.
			 *
			 * NOTE: The line segment endpoints must be on opposite sides of the 'thick' dateline plane,
			 * otherwise the result is not numerically robust.
			 */
			boost::optional<PointOnSphere>
			intersect_line_segment(
					IntersectionType &intersection_type,
					double &segment_interpolation_ratio,
					const GreatCircleArc &line_segment,
					bool line_segment_start_point_in_dateline_front_half_space);

			/**
			 * Calculates intersection point of the specified line segment and the dateline.
			 *
			 * A precondition is the line segment must not be zero length and that its end points
			 * must be on opposite sides of the 'thick' dateline plane (dateline great circle).
			 */
			boost::optional<PointOnSphere>
			calculate_intersection(
					const GreatCircleArc &line_segment,
					double &segment_interpolation_ratio) const;

			//! Classifies the specified point relative to the dateline.
			VertexClassification
			classify_vertex(
					const UnitVector3D &vertex);

			//! Adds a regular vertex that is *not* the result of intersecting or touching the dateline.
			void
			add_vertex(
					const PointOnSphere &point,
					const double &segment_interpolation_ratio,
					unsigned int segment_index,
					unsigned int geometry_part_index);

			//! Adds an intersection of geometry with the dateline on the front side of dateline.
			void
			add_intersection_vertex_on_front_dateline(
					const PointOnSphere &point,
					bool is_unwrapped_point,
					const double &segment_interpolation_ratio,
					unsigned int segment_index,
					unsigned int geometry_part_index,
					bool exiting_dateline_polygon);

			//! Adds an intersection of geometry with the dateline on the back side of dateline.
			void
			add_intersection_vertex_on_back_dateline(
					const PointOnSphere &point,
					bool is_unwrapped_point,
					const double &segment_interpolation_ratio,
					unsigned int segment_index,
					unsigned int geometry_part_index,
					bool exiting_dateline_polygon);

			/**
			 * Adds an intersection of geometry with the north pole.
			 */
			void
			add_intersection_vertex_on_north_pole(
					const PointOnSphere &point,
					bool is_unwrapped_point,
					const double &segment_interpolation_ratio,
					unsigned int segment_index,
					unsigned int geometry_part_index,
					bool exiting_dateline_polygon);

			/**
			 * Adds an intersection of geometry with the south pole.
			 */
			void
			add_intersection_vertex_on_south_pole(
					const PointOnSphere &point,
					bool is_unwrapped_point,
					const double &segment_interpolation_ratio,
					unsigned int segment_index,
					unsigned int geometry_part_index,
					bool exiting_dateline_polygon);

			/**
			 * Record the fact that the geometry intersected the north pole.
			 */
			void
			intersected_north_pole()
			{
				d_geometry_intersected_north_pole = true;
			}

			/**
			 * Record the fact that the geometry intersected the south pole.
			 */
			void
			intersected_south_pole()
			{
				d_geometry_intersected_south_pole = true;
			}

			void
			link_intersection_vertices(
						vertex_list_type &intersection_vertex_list1,
						vertex_list_type::Node *intersection_vertex_node1,
						vertex_list_type &intersection_vertex_list2,
						vertex_list_type::Node *intersection_vertex_node2)
			{
				Vertex &vertex1 = intersection_vertex_node1->element();
				vertex1.intersection_neighbour_list = &intersection_vertex_list2;
				vertex1.intersection_neighbour = intersection_vertex_node2;

				Vertex &vertex2 = intersection_vertex_node2->element();
				vertex2.intersection_neighbour_list = &intersection_vertex_list1;
				vertex2.intersection_neighbour = intersection_vertex_node1;
			}

			void
			generate_entry_exit_flags_for_dateline_polygon(
					const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon);

			void
			generate_entry_exit_flags_for_dateline_polygon(
					const vertex_list_type::iterator initial_dateline_vertex_iter,
					const bool initial_dateline_vertex_is_inside_geometry_polygon);

			void
			output_intersecting_polygons(
					std::vector<LatLonPolygon> &lat_lon_polygons,
					const boost::optional<CentralMeridian> &central_meridian,
					const boost::optional<AngularExtent> &tessellate_threshold);
		};


		/**
		 * Used to transform input geometries to the dateline reference frame (for wrapping) and back again.
		 *
		 * Is none when central meridian is zero.
		 */
		boost::optional<CentralMeridian> d_central_meridian;


		//! Constructor.
		explicit
		DateLineWrapper(
				double central_meridian);


		/**
		 * Output the input polyline (it is entirely off the dateline).
		 */
		void
		output_input_polyline(
				std::vector<LatLonPolyline> &wrapped_polylines,
				const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline,
				const boost::optional<AngularExtent> &tessellate_threshold) const;

		/**
		 * Output the input polygon (it is entirely off the dateline).
		 */
		void
		output_input_polygon(
				std::vector<LatLonPolygon> &wrapped_polygons,
				const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
				const boost::optional<AngularExtent> &tessellate_threshold) const;


		/**
		 * Output the input polyline if it is entirely *on* the dateline.
		 */
		void
		output_polyline_if_entirely_on_dateline(
				std::vector<LatLonPolyline> &wrapped_polylines,
				const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline,
				const IntersectionGraph &graph,
				const boost::optional<AngularExtent> &tessellate_threshold_degrees) const;

		/**
		 * Output the rings of the input polygon that do not intersect the dateline (if any).
		 *
		 * If @a group_interior_with_exterior_rings is true (and a ring is entirely off the
		 * dateline) then add a ring as:
		 *  1) an exterior ring of a new output polygon:
		 *     - if it's not inside (or intersecting) the polygons output so far,
		 *  2) an interior ring of an existing output polygon:
		 *     - if it's inside (or intersecting) any polygon output so far.
		 */
		void
		output_non_intersecting_polygon_rings(
				std::vector<LatLonPolygon> &wrapped_polygons,
				const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
				const IntersectionGraph &graph,
				const boost::optional<AngularExtent> &tessellate_threshold_degrees,
				bool group_interior_with_exterior_rings) const;


		//! Output the non-intersecting input vertices of a polyline, or polygon ring.
		template <class VertexIteratorType>
		void
		output_input_line_geometry(
				const VertexIteratorType vertex_begin,
				const VertexIteratorType vertex_end,
				LatLonLineGeometry &output_line_geometry,
				boost::optional<unsigned int> polygon_ring_index,
				bool on_dateline_arc,
				const boost::optional<AngularExtent> &tessellate_threshold_degrees) const;

		/**
		 * Returns true if the specified bounding small circle intersects the dateline arc.
		 *
		 * This is used as a quick test to see if a geometry (bounded by the small circle)
		 * can *possibly* intersect the dateline.
		 *
		 * The bounding small circle is converted to dateline reference frame if necessary.
		 *
		 * NOTE: A return value of true does *not* necessarily mean a geometry bounded by the
		 * small circle intersects the dateline.
		 * However a return value of false does mean a bounded geometry does *not* intersect the dateline.
		 */
		bool
		intersects_dateline(
				BoundingSmallCircle geometry_bounding_small_circle) const;
	};
}

//
// Implementation.
//

namespace GPlatesMaths
{
	inline
	const DateLineWrapper::lat_lon_points_seq_type &
	DateLineWrapper::LatLonPolygon::get_exterior_ring_points() const
	{
		return d_exterior_ring_line_geometry->get_points();
	}

	inline
	unsigned int
	DateLineWrapper::LatLonPolygon::get_num_interior_rings() const
	{
		return d_interior_ring_line_geometries.size();
	}

	inline
	const DateLineWrapper::lat_lon_points_seq_type &
	DateLineWrapper::LatLonPolygon::get_interior_ring_points(
			unsigned int interior_ring_index) const
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				interior_ring_index < d_interior_ring_line_geometries.size(),
				GPLATES_ASSERTION_SOURCE);

		return d_interior_ring_line_geometries[interior_ring_index]->get_points();
	}

	inline
	DateLineWrapper::LatLonPolygon::LatLonPolygon() :
		d_exterior_ring_line_geometry(new LatLonLineGeometry())
	{  }


	inline
	const DateLineWrapper::lat_lon_points_seq_type &
	DateLineWrapper::LatLonPolyline::get_points() const
	{
		return d_line_geometry->get_points();
	}

	inline
	DateLineWrapper::LatLonPolyline::LatLonPolyline() :
		d_line_geometry(new LatLonLineGeometry())
	{  }
}

#endif // GPLATES_MATHS_DATELINEWRAPPER_H
