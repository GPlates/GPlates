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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_TIMEWINDOW_H_
#define _GPLATES_GEO_TIMEWINDOW_H_

#include "global/types.h"  /* fpdata_t */

namespace GPlatesGeo
{
	using namespace GPlatesGlobal;

	/** 
	 * A window of time. 
	 * Units are Ma = millions of years ago.
	 * In the interests of a canonical representation, the time of
	 * appearance must always be before or at the same time as the
	 * time of disappearance, so the value of \a _begin must always
	 * be greater-than or equal-to \a end.
	 */
	class TimeWindow
	{
		public:
			TimeWindow() : _begin(0.0), _end(0.0), _inf(true) {  }

			TimeWindow(const fpdata_t& begin, const fpdata_t& end);

			fpdata_t
			GetBeginning() const { return _begin; }

			fpdata_t
			GetEnd() const { return _end; }

			bool
			IsInfinite() const { return _inf; }

			bool
			ContainsTime(const fpdata_t &t) const {

				if (_inf) return true;
				return ((_begin >= t) && (t >= _end));
			}

		private:
			fpdata_t _begin, _end;
			bool _inf;
	};


	inline bool
	operator==(TimeWindow tw1, TimeWindow tw2) {

		if (tw1.IsInfinite() && tw2.IsInfinite()) return true;
		return ((tw1.GetBeginning() == tw2.GetBeginning()) &&
		        (tw1.GetEnd() == tw2.GetEnd()));
	}
}

#endif  // _GPLATES_GEO_TIMEWINDOW_H_
