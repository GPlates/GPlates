/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_POINTONSPHERE_H_
#define _GPLATES_MATHS_POINTONSPHERE_H_

#include "global/types.h"  /* real_t */
#include "global/Assert.h"
#include "ViolatedCoordinateInvariantException.h"

namespace GPlatesMaths
{
	using namespace GPlatesGlobal;

	/** 
	 * Point on a sphere. 
	 * Represents a coordinate on the surface of a sphere, i.e. in
	 * terms of latitude and longitude.
	 * @invariant 
	 *   - -90 <= latitude <= 90
	 *   - -180 <= longitude <= 180
	 */
	class PointOnSphere
	{
		public:
			/**
			 * Create a point with specified latitude and longitude.
			 * @param lat The latitude.
			 * @param lon The longitude.
			 */
			explicit 
			PointOnSphere(const real_t& lat = 0.0,
			              const real_t& lon = 0.0)
				: _lat(lat), _lon(lon) {
				
				AssertInvariantHolds();
			}

		protected:
			/** 
			 * Assert the class invariant.
			 * @throw ViolatedCoordinateInvariantException
			 *   if the invariant has been violated.
			 */
			void
			AssertInvariantHolds() const;

		private:
			real_t _lat,  /**< Latitude coordinate. */
			       _lon;  /**< Longitude coordinate. */
	};
}

#endif  // _GPLATES_MATHS_POINTONSPHERE_H_
