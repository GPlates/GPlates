/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_CONTROLS_LIFETIME_H_
#define _GPLATES_CONTROLS_LIFETIME_H_

#include <string>
#include "gui/MainWindow.h"


namespace GPlatesControls
{
	/**
	 * This class is used to control the lifetime of the program.
	 *
	 * To be more precise, it is used to control the *termination* of
	 * the lifetime of the program.  It contains all the information
	 * needed to correctly terminate the program (a reference to the
	 * wxWidgets "top window", for example) and it needs to be
	 * initialised with this information before an instance can be
	 * created.
	 *
	 * Exceptions will be thrown if:
	 *  - the class is initialised twice.
	 *  - the class is initialised with a NULL pointer.
	 *  - an attempt is made to create an instance before the class
	 *     has been initialised.
	 *
	 * This class is a Singleton.
	 */
	class Lifetime
	{
		public:
			static void init(GPlatesGui::MainWindow *main_win);
			static Lifetime *instance();

			void terminate(const std::string &reason);

		protected:
			Lifetime() {  }

		private:
			static Lifetime *_instance;
			static bool _is_initialised;
			static GPlatesGui::MainWindow *_main_win;
	};
}

#endif  // _GPLATES_CONTROLS_LIFETIME_H_
