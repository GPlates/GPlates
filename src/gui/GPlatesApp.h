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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_GPLATESAPP_H_
#define _GPLATES_GUI_GPLATESAPP_H_

#include <wx/app.h>

namespace GPlatesGui
{
	class GPlatesApp: public wxApp
	{
		public:
			/**
			 * This function is called by wxWindows during the
			 * application's start-up ("initialisation") phase.
			 * The application developers should place their own
			 * application-initialisation code in here.
			 */
			bool OnInit();
	};
}

#endif  // _GPLATES_GUI_GPLATESAPP_H_
