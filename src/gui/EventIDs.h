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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_EVENTIDS_H_
#define _GPLATES_GUI_EVENTIDS_H_

#include <wx/event.h>

namespace GPlatesGui {

	/**
	 * IDs for command and menu events.
	 */
	namespace EventIDs {

		enum {

			// Use 'wxID_HIGHEST + 1' to avoid ID clashes
			COMMAND_ESCAPE = wxID_HIGHEST + 1,

			POPUP_SPIN_GLOBE,

			MENU_FILE_OPENDATA,
			MENU_FILE_LOADROTATION,
			MENU_FILE_IMPORT,
			MENU_FILE_EXPORT,
			MENU_FILE_SAVEALLDATA,
			MENU_FILE_EXIT,

			MENU_VIEW_ZOOM_IN,
			MENU_VIEW_ZOOM_OUT,

			MENU_RECONSTRUCT_TIME,
			MENU_RECONSTRUCT_PRESENT,
			MENU_RECONSTRUCT_ANIMATION,

			// Important for the ID to take this value,
			// for the Mac port.
			MENU_HELP_ABOUT = wxID_ABOUT
		};
	}
}

#endif  /* _GPLATES_GUI_EVENTIDS_H_ */
