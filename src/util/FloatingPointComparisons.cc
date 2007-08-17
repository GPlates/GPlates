/* $Id$ */

/**
 * @file
 * Contains the definitions of the functions of the namespace FloatingPointComparisons.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 James Boyden <jboy@jboy.id.au>
 *
 * This file is derived from the source file "FloatingPointComparisons.cc",
 * which is part of the ReconTreeViewer software:
 *  http://sourceforge.net/projects/recontreeviewer/
 *
 * ReconTreeViewer is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2, as published
 * by the Free Software Foundation.
 *
 * ReconTreeViewer is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cmath>  // std::fabs
#include "FloatingPointComparisons.h"


namespace
{
	// Allow a small epsilon of about 1.0e-09, in case of accumulated floating-point error.
	//
	// For what it's worth, this represents a precision of about eight-and-three-quarter hours,
	// which is not too bad for geological time.
	static const double Epsilon = 1.0e-9;
}


bool
GPlatesUtil::FloatingPointComparisons::geo_times_are_approx_equal(
		const double &geo_time1,
		const double &geo_time2)
{
	return (std::fabs(geo_time1 - geo_time2) < ::Epsilon);
}
