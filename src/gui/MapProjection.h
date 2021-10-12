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

#include <proj_api.h>

#include "boost/optional.hpp"

#include "gui/ProjectionException.h"
#include "maths/GreatCircle.h"
#include "maths/LatLonPointConversions.h"


namespace GPlatesGui
{

	// Make the first enum Orthographic (even though we don't implement that
	// as a map projection), so that we'll match up better with the combo-box indices, which will
	// use the zeroth entry for the 3D Orthographic (Globe) view. 
	enum ProjectionTypes{
		ORTHOGRAPHIC = 0,
		RECTANGULAR,
		MERCATOR,
		MOLLWEIDE,
		ROBINSON,
		LAMBERT_CONIC,
		NUM_PROJECTIONS
	};

	const int MIN_PROJECTION_INDEX = RECTANGULAR;

	class MapProjection{
		
	public:

		MapProjection();

		MapProjection(int projection_type);

		~MapProjection();

		/**
		 * Change the projection to that referred to by projection_type.
		 */ 
		void
		set_projection_type(
			int projection_type);

		/**
		 * Get the projection type.
		 */
		int projection_type() const
		{
			return d_projection_type;
		}

		/**
		 * Transform the latitude and longitude to cartesian coordinates according to the 
		 * current state of the projection. 
		 */ 
		void
		forward_transform(
			double &longitude,
			double &latitude) const;

		/**
		 * Transforms the point on sphere to cartesian coodinates according to the 
		 * current state of the projection.
		 */
		void
		forward_transform(
			const GPlatesMaths::PointOnSphere &point_on_sphere,
			double &x_coordinate, 
			double &y_coordinate) const;

		/**
		* Transform cartesian (x,y) coordinates to a LatLonPoint according to the current
		* state of the projection. 
		* 
		* Return type is boost::optional as there may not be a valid inverse transform
		* for the provided (x,y) values. 
		*/ 
		boost::optional<GPlatesMaths::LatLonPoint>
		inverse_transform(
			double &x,
			double &y) const;

		/**
		 * Set the central llp
		 */ 
		void
		set_central_llp(
			GPlatesMaths::LatLonPoint &llp);

		/**
		 * Get the central llp
		 */
		const
		GPlatesMaths::LatLonPoint &
		central_llp() const;

		/**
		 * Get the great circle which includes the great circle arc defining the boundary of the map. 
		 */
		const
		GPlatesMaths::GreatCircle &
		boundary_great_circle() const;

			
	private:

		// Make assignment operator private.
		MapProjection & 
		operator=(
			const MapProjection &);

		// Make copy constructor private.
		MapProjection(
			const MapProjection &other);

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

		/**
		 * The scale factor for the projection.
		 */ 
		double d_scale;

		/**
		 * An integer representing the current projection. 
		 */
		int d_projection_type;


		/**
		 * The central lat-lon point for the projection.
		 */
		GPlatesMaths::LatLonPoint d_central_llp;


		/**
		 * The great circle which includes the great circle arc defining the boundary of the map. 
		 */
		GPlatesMaths::GreatCircle d_boundary_great_circle;

	};
}

#endif // GPLATES_GUI_MAPPROJECTION_H
