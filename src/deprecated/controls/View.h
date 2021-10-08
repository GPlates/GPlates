/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_CONTROLS_VIEW_H_
#define _GPLATES_CONTROLS_VIEW_H_

#include <string>

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
		std::string DocumentMetadata();
	}
}

#endif  /* _GPLATES_CONTROLS_VIEW_H_ */
