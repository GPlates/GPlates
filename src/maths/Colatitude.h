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

#ifndef _GPLATES_MATHS_COLATITUDE_H_
#define _GPLATES_MATHS_COLATITUDE_H_

#include "Real.h"


namespace GPlatesMaths
{
	class Latitude;

	/**
	 * A colatitude is a real number in the range [0, pi].
	 */
	class Colatitude
	{
		public:
			Colatitude() : _rval(0.0) {  }

			/**
			 * Create a Colatitude using a real number.
			 *
			 * @throws ViolatedClassInvariantException if @a r is 
			 *   outside the range [0, pi].
			 */
			explicit
			Colatitude(const Real &r) : _rval(r) {

				AssertInvariant();
			}

			explicit
			Colatitude(const Latitude &lat);

			Real
			rval() const { return _rval; }

		private:
			Real _rval;

			/**
			 * @throws ViolatedClassInvariantException if the class 
			 *   invariant is false (i.e. @a _rval is outside the 
			 *   range [0, pi]).
			 */
			void AssertInvariant();
	};


	inline bool
	operator==(const Colatitude &colat1, const Colatitude &colat2) {

		return (colat1.rval() == colat2.rval());
	}


	inline bool
	operator!=(const Colatitude &colat1, const Colatitude &colat2) {

		return (colat2.rval() != colat2.rval());
	}


	inline Real
	sin(const Colatitude &colat) {

		return sin(colat.rval());
	}


	inline Real
	cos(const Colatitude &colat) {

		return cos(colat.rval());
	}


	inline std::ostream &
	operator<<(std::ostream &os, const Colatitude &colat) {

		os << colat.rval();
		return os;
	}
}

#endif  // _GPLATES_MATHS_COLATITUDE_H_
