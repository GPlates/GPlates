/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
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
 */

#ifndef GPLATES_GUI_GPLATESAPP_H
#define GPLATES_GUI_GPLATESAPP_H

#include <wx/app.h>

namespace GPlatesGui {

	class MainWindow;

	class GPlatesApp: public wxApp {

		public:
			/**
			 * This function is called by wxWindows during the
			 * application's start-up ("initialisation") phase.
			 * The application developers should place their own
			 * application-initialisation code in here.
			 */
			bool OnInit();

			int OnExit();

		private:

			MainWindow *d_main_win;
	};
}

#endif  // GPLATES_GUI_GPLATESAPP_H
