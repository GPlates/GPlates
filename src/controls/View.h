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

#ifndef _GPLATES_CONTROLS_VIEW_H_
#define _GPLATES_CONTROLS_VIEW_H_

#include <string>
#include "FrameRedisplay.h"

namespace GPlatesControls
{
	namespace View
	{
		/**
		 * Return the title and meta information for the loaded
		 * data set.
		 *
		 * XXX: should emit some kind of error if there is no data
		 * set loaded from which to obtain the infomation.
		 */
		std::string
		DocumentMetadata();

		/**
		 * This is a function object.  Calling 
		 * GPlatesControls::View::Redisplay() will send a request for
		 * the repainting of the main frame.
		 */
		extern FrameRedisplay
		Redisplay;
	}
}

#endif  /* _GPLATES_CONTROLS_VIEW_H_ */
