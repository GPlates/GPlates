/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The Geological Survey of Norway
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

#ifndef GPLATES_GUI_MAPPROJECTION_H
#define GPLATES_GUI_MAPPROJECTION_H

#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <QPointF>

#include "ProjectionException.h"

#include "file-io/Proj.h"

#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"

#include "utils/ReferenceCount.h"


namespace GPlatesGui
{
	class MapProjectionSettings;

	/**
	 * Projects latitude/longitude to/from various map projections.
	 */
	class MapProjection :
			public GPlatesUtils::ReferenceCount<MapProjection>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a MapProjection.
		typedef GPlatesUtils::non_null_intrusive_ptr<MapProjection> non_null_ptr_type;
		//! A convenience typedef for a shared pointer to a const @a MapProjection.
		typedef GPlatesUtils::non_null_intrusive_ptr<const MapProjection> non_null_ptr_to_const_type;


		enum Type
		{
			RECTANGULAR,
			MERCATOR,
			MOLLWEIDE,
			ROBINSON,

			NUM_PROJECTIONS
		};

		/**
		 * Return a suitable label naming the specified projection type.
		 */
		static
		const char *
		get_display_name(
				Type projection_type);

		/**
		 * Creates a @a MapProjection object with no map projection setting.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new MapProjection());
		}

		/**
		 * Creates a @a MapProjection object with the specified map projection and zero central meridian.
		 */
		static
		non_null_ptr_type
		create(
				Type projection_type)
		{
			return non_null_ptr_type(new MapProjection(projection_type));
		}

		/**
		 * Creates a @a MapProjection object with the specified map projection type and central meridian.
		 */
		static
		non_null_ptr_type
		create(
				const MapProjectionSettings &projection_settings)
		{
			return non_null_ptr_type(new MapProjection(projection_settings));
		}


		~MapProjection();

		/**
		 * Returns the projection settings of this map projection.
		 *
		 * The returned settings can be equality compared to determine if two map projections
		 * will generate the same projection results.
		 */
		MapProjectionSettings
		get_projection_settings() const;

		/**
		 * Change the projection to that referred to by projection_type.
		 */ 
		void
		set_projection_type(
				Type projection_type);

		/**
		 * Get the projection type.
		 */
		Type
		projection_type() const
		{
			return d_projection_type;
		}


		/**
		 * Set the central meridian.
		 */ 
		void
		set_central_meridian(
				const double &central_meridian_);

		/**
		 * Get the central meridian.
		 */
		double
		central_meridian() const
		{
			return d_central_meridian;
		}


		/**
		 * Transforms the point on sphere to cartesian coordinates (x,y) according to the 
		 * current state of the projection.
		 */
		QPointF
		forward_transform(
				const GPlatesMaths::PointOnSphere &point_on_sphere) const
		{
			return forward_transform(make_lat_lon_point(point_on_sphere));
		}

		/**
		 * Transforms the lat-lon point to cartesian coordinates (x,y) according to the 
		 * current state of the projection.
		 */
		QPointF
		forward_transform(
				const GPlatesMaths::LatLonPoint &lat_lon_point) const;

		/**
		 * Transform the longitude and latitude to cartesian coordinates according to the
		 * current state of the projection.
		 */ 
		void
		forward_transform(
				double &longitude,
				double &latitude) const;


		/**
		 * Transform cartesian (x,y) coordinates to a LatLonPoint according to the current
		 * state of the projection.
		 *
		 * Return type is boost::optional as there may not be a valid inverse transform for
		 * the provided (x,y) values - including if the specified map point is outside
		 * the map boundary (eg, outside the map rectangle in the Rectangular projection).
		 */
		boost::optional<GPlatesMaths::LatLonPoint>
		inverse_transform(
				const QPointF &map_point) const;

		/**
		 * Transform cartesian (x,y) coordinates to longitude and latitude according to the current
		 * state of the projection.
		 *
		 * Returns false if there is not a valid inverse transform for the provided (x,y) values -
		 * including if the specified (x, y) map position is outside the map boundary
		 * (eg, outside the map rectangle in the Rectangular projection).
		 */
		bool
		inverse_transform(
				double &x,
				double &y) const;


		/**
		 * Returns true if specified point is inside the map projection boundary.
		 */
		bool
		is_inside_map_boundary(
				const QPointF &map_point) const
		{
			return static_cast<bool>(inverse_transform(map_point));
		}

		/**
		 * Return the map position near (but just inside to within a small tolerance) the map boundary
		 * given two map positions (one inside and one outside map boundary).
		 *
		 * The returned position is essentially the intersection of the 2D line segment joining the specified
		 * inside and outside points with the map boundary, obtained using bisection iteration that terminates
		 * once converged to within @a bisection_iteration_threshold_ratio times the bounding radius.
		 *
		 * Throws @a PreconditionViolationError if the specified inside point is not inside, or
		 * the specified outside point is not outside, the map boundary.
		 *
		 * Note: The returned map position is guaranteed to have a valid inverse transform.
		 *
		 * Note: The lat-lon point (0, central_meridian) maps to the origin in map projection space.
		 *
		 * Note: The line segment (joining inside and outside points) only crosses map boundary once
		 *       since shape of map boundary is convex.
		 */
		QPointF
		get_map_boundary_position(
				const QPointF &map_point_inside_boundary,
				const QPointF &map_point_outside_boundary,
				double bisection_iteration_threshold_ratio = 1e-6/*equivalent to roughly 1 arc second on map*/) const;

		/**
		 * Return the radius of the circle/sphere that bounds the map (including a very small numerical tolerance).
		 *
		 * Note: The lat-lon point (0, central_meridian) maps to the origin in map projection space.
		 *       So the bounding circle/sphere is centred at the origin (in map projection space).
		 */
		double
		get_map_bounding_radius() const;
			
	private:

#if defined(GPLATES_USING_PROJ4)

		/**
		 * The proj4 projection.
		 */ 
		projPJ d_projection;

		/**
		 * A proj4 latlon projection.
		 *
		 * This is used in the pw_transform function.
		 */
		projPJ d_latlon_projection;

#else // using proj5+...

		/**
		 * The proj5+ transformation between a configurable projection and lat/lon. 
		 */ 
		PJ *d_transformation;

		/**
		 * Information about the current instance of PROJ.
		 */
		PJ_INFO d_proj_info;

#endif

		/**
		 * The scale factor for the projection.
		 */ 
		double d_scale;

		/**
		 * An integer representing the current projection. 
		 */
		Type d_projection_type;


		/**
		 * The central meridian for the projection.
		 */
		double d_central_meridian;

		/**
		 * Radius of the circle/sphere that bounds the map (including a very small numerical tolerance).
		 *
		 * This is calculated and cached when bounding radius is requested (and set to none when the map projection changes).
		 */
		mutable boost::optional<double> d_cached_bounding_radius;


		MapProjection();

		MapProjection(
				Type projection_type);

		MapProjection(
				const MapProjectionSettings &projection_settings);

		/**
		 * Ask the Proj library to forward transform from (longitude, latitude) in degrees to map projection space.
		 */
		void
		forward_proj_transform(
				double longitude,
				double latitude,
				double &x,
				double &y) const;

		/**
		 * Ask the Proj library to inverse transform from map projection space (x, y) back to (longitude, latitude) in degrees.
		 *
		 * Returns false is there's not a valid inverse transform for the provided (x, y) values.
		 */
		bool
		inverse_proj_transform(
				double x,
				double y,
				double &longitude,
				double &latitude) const;

		/**
		 * Check that the inverted (x, y), which are (longitude, latitude) coordinates, forward transform
		 * to the specified (x, y) within a numerical tolerance.
		 */
		bool
		check_forward_transform(
				const double &inverted_x,
				const double &inverted_y,
				const double &x,
				const double &y) const;
	};


	/**
	 * Projection settings used to determine if two map projections will generate the same projection results.
	 *
	 * NOTE: This is equality comparable.
	 *
	 * Note that this was a nested class inside @a MapProjection but was causing compile issues on some systems -
	 * possibly due to 'friend bool operator==' injecting into the outer class instead of a namespace.
	 */
	class MapProjectionSettings :
			public boost::equality_comparable<MapProjectionSettings>
	{
	public:
		MapProjectionSettings(
				MapProjection::Type projection_type_,
				const double &central_meridian_);

		MapProjection::Type
		get_projection_type() const
		{
			return d_projection_type;
		}

		void
		set_projection_type(
				MapProjection::Type projection_type_)
		{
			d_projection_type = projection_type_;
		}

		double
		get_central_meridian() const
		{
			return d_central_meridian;
		}

		void
		set_central_meridian(
				const double &central_meridian_)
		{
			d_central_meridian = central_meridian_;
		}

	private:
		//! The projection type.
		MapProjection::Type d_projection_type;

		//! The central meridian for the projection.
		double d_central_meridian;

		friend
		bool
		operator==(
				const MapProjectionSettings &lhs,
				const MapProjectionSettings &rhs)
		{
			return lhs.d_projection_type == rhs.d_projection_type &&
				GPlatesMaths::are_almost_exactly_equal(lhs.d_central_meridian, rhs.d_central_meridian);
		}
	};
}

#endif // GPLATES_GUI_MAPPROJECTION_H
