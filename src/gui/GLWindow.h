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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_GLWINDOW_H_
#define _GPLATES_GUI_GLWINDOW_H_

#include "Globe.h"
#include "Colour.h"

namespace GPlatesGui
{
	/**
	 * GLWindow conforms to the Singleton pattern
	 */
	class GLWindow
	{
		public:
			static GLWindow*
			GetWindow(int* argc = NULL, char** argv = NULL);
				
		private:
			GLWindow(int* argc, char** argv);

			void
			Clear(const Colour& c = Colour::BLACK);
			
			/**
			 * Callbacks.
			 */
			static void
			Display();

			static void
			Reshape(int width, int height);

			static void
			Keyboard(unsigned char key, int x, int y);

			static void
			Special(int key, int x, int y);

			/**
			 * The one and only window that can exist.
			 */
			static GLWindow* _window;

			/**
			 * The globe to display on this window.
			 */
			Globe _globe;
	};
}

#endif  /* _GPLATES_GUI_GLWINDOW_H_ */
