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

#ifndef _GPLATES_GUI_GLCANVAS_H_
#define _GPLATES_GUI_GLCANVAS_H_

#include <wx/glcanvas.h>
#include <iostream>
#include "Globe.h"
#include "geo/DataGroup.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGui
{
	class GLCanvas : public wxGLCanvas
	{
		public:
			GLCanvas(wxWindow* parent, 
					 const wxSize& size = wxDefaultSize,
					 const wxPoint& position = wxDefaultPosition)
				: wxGLCanvas(parent, -1, position, size), 
				  _zoom_factor(1.0), // Zoom factor == 1.0 => no zoom
				  _is_initialised(false) { parent->Show(TRUE); SetCurrent(); }

			/**
			 * Paint the picture.
			 */
			void
			OnPaint(wxPaintEvent&);

			/**
			 * Set the dimensions of our picture.
			 * Called on startup and when the user resizes the window.
			 */
			void 
			OnSize(wxSizeEvent&);

			/**
			 * According to the wxWindows docs, declaring this function to
			 * be empty eliminates flicker on some platforms (mainly win32).
			 */
			void 
			OnEraseBackground(wxEraseEvent&) {  }

			/**
			 * Double clicking the left mouse button repositions the view
			 * to centre on the point clicked.
			 */
			void
			OnReposition(wxMouseEvent&);
			
			/**
			 * Right mouse button is responsible for Z axis spins.
			 */
			void
			OnSpin(wxMouseEvent&);

			/**
			 * Return the PointOnSphere corresponding to the given screen
			 * coordinate, or else return a NULL pointer.
			 * @pre 0 <= screenx < SCREEN_WIDTH, 
			 *      0 <= screeny < SCREEN_HEIGHT.
			 * @warning The @b client is responsible for the deletion of
			 *   the memory pointed to by the return value.
			 */
			GPlatesMaths::PointOnSphere*
			GetSphereCoordFromScreen(int screenx, int screeny);
			
		private:
			Globe _globe;
			GLfloat _zoom_factor;
			bool _is_initialised;

			void InitGL();
			void SetView();

			DECLARE_EVENT_TABLE()
	};
}

#endif  /* _GPLATES_GUI_GLCANVAS_H_ */
