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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_TIMEWINDOW_H_
#define _GPLATES_GEO_TIMEWINDOW_H_

#include "GeneralisedData.h"
#include "global/types.h"  /* fpdata_t */

namespace GPlatesGeo
{
	using namespace GPlatesGlobal;

	/** 
	 * A window of time. 
	 * Units are Ma = millions of years ago.
	 * If \a _begin > \a _end, then the window is directed forward in time.
	 * If \a _begin < \a _end, then the window is directed backward in time.
	 * If \a _begin = \a _end, then the window is an instant in time.
	 *
	 * @todo Yet to be implemented.
	 */
	class TimeWindow : public GeneralisedData
	{
		public:
			TimeWindow()
				: GeneralisedData(), _begin(0.0), _end(0.0) { }

			TimeWindow(const fpdata_t& begin, const fpdata_t& end);
			
			virtual void
			ReadIn(std::istream& is) {

				is >> _begin >> _end;
			}

			virtual void
			PrintOut(std::ostream& os) const {
				
				os << _begin << " " << _end;
			}

		private:
			fpdata_t _begin, _end;
	};
}

#endif  // _GPLATES_GEO_TIMEWINDOW_H_
