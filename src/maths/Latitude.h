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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_LATITUDE_H_
#define _GPLATES_MATHS_LATITUDE_H_

#include "Real.h"


namespace GPlatesMaths
{
	class Colatitude;

	/**
	 * A latitude is a real number in the range [-pi/2, pi/2].
	 */
	class Latitude
	{
		public:
			Latitude() : _rval(0.0) {  }

			explicit
			Latitude(const Real &r) : _rval(r) {

				AssertInvariant();
			}

			explicit
			Latitude(const Colatitude &colat);

			Real
			rval() const { return _rval; }

		private:
			Real _rval;

			void AssertInvariant();
	};


	inline bool
	operator==(const Latitude &lat1, const Latitude &lat2) {

		return (lat1.rval() == lat2.rval());
	}


	inline bool
	operator!=(const Latitude &lat1, const Latitude &lat2) {

		return (lat2.rval() != lat2.rval());
	}


	inline Latitude
	operator-(const Latitude &lat) {

		return Latitude(-lat.rval());
	}


	inline Real
	sin(const Latitude &lat) {

		return sin(lat.rval());
	}


	inline Real
	cos(const Latitude &lat) {

		return cos(lat.rval());
	}


	inline std::ostream &
	operator<<(std::ostream &os, const Latitude &lat) {

		os << lat.rval();
		return os;
	}
}

#endif  // _GPLATES_MATHS_LATITUDE_H_
