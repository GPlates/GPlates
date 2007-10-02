/* $Id$ */

/**
 * @file
 * Contains the definition of the namespace FloatingPointComparisons.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 James Boyden <jboy@jboy.id.au>
 *
 * This file is derived from the header file "FloatingPointComparisons.hh",
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

#ifndef GPLATES_UTILS_FLOATINGPOINTCOMPARISONS_H
#define GPLATES_UTILS_FLOATINGPOINTCOMPARISONS_H

namespace GPlatesUtils
{
	/**
	 * This namespace contains functions for floating-point comparisons.
	 */
	namespace FloatingPointComparisons
	{
		/**
		 * Determine whether the two geological times @a geo_time1 and @a geo_time2 are
		 * equal (within a small epsilon).
		 *
		 * For an explanation of why this function is necessary, read the article "What
		 * Every Computer Scientist Should Know About Floating-Point Arithmetic" by David
		 * Goldberg: http://docs.sun.com/source/806-3568/ncg_goldberg.html
		 *
		 * I realise that using an epsilon for equality comparison is a little questionable
		 * -- the Goldberg paper even states this: 
		 * 	Incidentally, some people think that the solution to such anomalies is
		 * 	never to compare floating-point numbers for equality, but instead to
		 * 	consider them equal if they are within some error bound E.  This is hardly
		 * 	a cure-all because it raises as many questions as it answers.  What should
		 * 	the value of E be?  If x < 0 and y > 0 are within E, should they really be
		 * 	considered to be equal, even though they have different signs?
		 *
		 * My justification for using an epsilon in this case is that I'm expecting
		 * geological times to be confined to the range [0.001, 10000.000] and I believe
		 * I've chosen an epsilon which correctly and usefully covers this range.
		 */
		bool
		geo_times_are_approx_equal(
				const double &geo_time1,
				const double &geo_time2);
	}
}

#endif  // GPLATES_UTILS_FLOATINGPOINTCOMPARISONS_H
