/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_FASTMATHS_H
#define GPLATES_MATHS_FASTMATHS_H

#include "MathsUtils.h"


namespace GPlatesMaths
{
	/**
	 * Faster versions of some standard library math functions.
	 *
	 * NOTE: Only use these functions if CPU profiling has determined that they are needed and
	 * the loss of accuracy is not important.
	 * See 'utils/Profile.h' for details on how to profile.
	 *
	 * NOTE: These are less accurate than the standard library math functions.
	 * So you should always use the standard library functions in general.
	 */
	namespace FastMaths
	{
		/**
		 * Calculates the arc-sine.
		 *
		 * Accurate to within 1e-4.
		 *
		 * On an Intel CPU (circa 2011) it's about twice as fast as std::asin, but probably
		 * faster on older architectures.
		 * On an i7-950 3GHz, profiling showed an average of 34 cycles per call over the input
		 * range [0,1] whereas std::asin averaged 78 cycles over the same input range.
		 */
		double
		asin(
				double sine/*is not a reference because used as a temporary*/);


		/**
		 * Calculates the arc-cosine.
		 *
		 * Accurate to within 1e-4.
		 *
		 * On an Intel CPU (circa 2011) it's about twice as fast as std::acos, but probably
		 * faster on older architectures.
		 * On an i7-950 3GHz, profiling showed an average of 38 cycles per call over the input
		 * range [0,1] whereas std::acos averaged 80 cycles over the same input range.
		 */
		inline
		double
		acos(
				const double &cosine)
		{
			return HALF_PI - FastMaths::asin(cosine);
		}
	}
}

#endif // GPLATES_MATHS_FASTMATHS_H
