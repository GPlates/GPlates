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
#include <proj_api.h>

#include "gui/ProjectionException.h"

#include "maths/GreatCircle.h"
#include "maths/LatLonPoint.h"

#include "utils/ReferenceCount.h"


namespace GPlatesGui
{
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


		// Make the first enum Orthographic (even though we don't implement that
		// as a map projection), so that we'll match up better with the combo-box indices, which will
		// use the zeroth entry for the 3D Orthographic (Globe) view. 
		enum Type
		{
			ORTHOGRAPHIC = 0,
			RECTANGULAR,
			MERCATOR,
			MOLLWEIDE,
			ROBINSON,
			LAMBERT_CONIC,

			NUM_PROJECTIONS
		};

		/**
		 * Projection settings used to determine if two map projections will generate the same projection results.
		 *
		 * NOTE: This is equality comparable.
		 */
		class Settings :
				public boost::equality_comparable<Settings>
		{
		public:
			Settings(
					Type projection_type,
					const GPlatesMaths::LatLonPoint &central_llp) :
				d_projection_type(projection_type),
				d_central_llp(central_llp)
			{  }

			Type
			get_projection_type() const
			{
				return d_projection_type;
			}

			void
			set_projection_type(
					Type projection_type)
			{
				d_projection_type = projection_type;
			}

			const GPlatesMaths::LatLonPoint &
			get_central_llp() const
			{
				return d_central_llp;
			}

			void
			set_central_llp(
					const GPlatesMaths::LatLonPoint &central_llp)
			{
				d_central_llp = central_llp;
			}

		private:
			//! The projection type.
			Type d_projection_type;

			//! The central lat-lon point for the projection.
			GPlatesMaths::LatLonPoint d_central_llp;

			friend
			bool
			operator==(
					const Settings &lhs,
					const Settings &rhs)
			{
				return lhs.d_projection_type == rhs.d_projection_type &&
					make_point_on_sphere(lhs.d_central_llp) == make_point_on_sphere(rhs.d_central_llp);
			}
		};


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
				const Settings &projection_settings)
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
		Settings
		get_projection_settings() const
		{
			return Settings(d_projection_type, d_central_llp);
		}

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
		 * Transforms the point on sphere to cartesian coodinates according to the 
		 * current state of the projection.
		 */
		void
		forward_transform(
			const GPlatesMaths::PointOnSphere &point_on_sphere,
			double &x_coordinate, 
			double &y_coordinate) const;

		/**
		 * Transform the latitude and longitude to cartesian coordinates according to the 
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
		const GPlatesMaths::LatLonPoint &
		central_llp() const
		{
			return d_central_llp;
		}

		/**
		 * Get the great circle which includes the great circle arc defining the boundary of the map. 
		 */
		const GPlatesMaths::GreatCircle &
		boundary_great_circle() const
		{
			return d_boundary_great_circle;
		}

			
	private:

		static const int MIN_PROJECTION_INDEX = RECTANGULAR;

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
		Type d_projection_type;


		/**
		 * The central lat-lon point for the projection.
		 */
		GPlatesMaths::LatLonPoint d_central_llp;


		/**
		 * The great circle which includes the great circle arc defining the boundary of the map. 
		 */
		GPlatesMaths::GreatCircle d_boundary_great_circle;


		MapProjection();

		MapProjection(
				Type projection_type);

		MapProjection(
				const Settings &projection_settings);

		/**
		 * Updates the boundary great circle - should be called if central llp or projection type changed.
		 */
		void
		update_boundary_great_circle();
	};
}

#endif // GPLATES_GUI_MAPPROJECTION_H
