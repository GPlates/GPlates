/* $Id$ */

/**
 * @file 
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

#ifndef _GPLATES_CONTROLS_GUICALLS_H_
#define _GPLATES_CONTROLS_GUICALLS_H_

#include "gui/MainWindow.h"
#include "gui/GLCanvas.h"
#include "global/types.h"  /* fpdata_t */

namespace GPlatesControls
{
	/**
	 * A collection of the calls which the GUI-controls must make
	 * back to the GUI.
	 *
	 * Note that none of these calls will have any effect unless
	 * the GUI components have been set.
	 */
	class GuiCalls
	{
		public:
			/**
			 * Repaint the GUI canvas.
			 */
			static void RepaintCanvas();

			/**
			 * Set the current geological time, as displayed in
			 * the main GUI window.
			 */
			static void SetCurrentTime(const
			 GPlatesGlobal::fpdata_t &t);

			/**
			 * Set the main GUI window and the GUI canvas.
			 */
			static void SetComponents(GPlatesGui::MainWindow
			 *window, GPlatesGui::GLCanvas *canvas);

			/**
			 * Set the current mode of operation to 'animation'.
			 */
			static void SetOpModeToAnimation();

			/**
			 * Return the current mode of operation to 'normal'.
			 */
			static void ReturnOpModeToNormal();

			/**
			 * Notify the main window that the animation has been
			 * stopped.
			 */
			static void StopAnimation(bool interrupted);

		private:
			static GPlatesGui::MainWindow *_window;
			static GPlatesGui::GLCanvas   *_canvas;
	};
}

#endif  /* _GPLATES_CONTROLS_GUICALLS_H_ */
