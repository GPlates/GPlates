/* $Id$ */

/**
 * @file 
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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_CONTROLS_RECONSTRUCT_H_
#define _GPLATES_CONTROLS_RECONSTRUCT_H_

#include "global/types.h"

namespace GPlatesControls
{
	namespace Reconstruct
	{
		/**
		 * Reconstruct the positions of the data at time @a time
		 * using the loaded rotation file.
		 */
		void
		Time(const GPlatesGlobal::fpdata_t& time);

		/**
		 * Reset the construction back to the present day.
		 */
		void
		Present();

		/**
		 * Display an animation of the positions of the data as
		 * they move from time @a start_time to time @a end_time.
		 * @a start_time and @a end_time are measured in millions
		 * of years ago.
		 */
		void
		Animation(const GPlatesGlobal::fpdata_t& start_time,
		          const GPlatesGlobal::fpdata_t& end_time,
		          const GPlatesGlobal::fpdata_t& time_delta,
		          bool finish_on_end);
	}
}

#endif  /* _GPLATES_CONTROLS_RECONSTRUCT_H_ */
