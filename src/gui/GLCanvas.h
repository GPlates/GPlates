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

#ifndef _GPLATES_GUI_GLCANVAS_H_
#define _GPLATES_GUI_GLCANVAS_H_

#include <wx/glcanvas.h>
#include <wx/menu.h>
#include "MainWindow.h"
#include "Globe.h"
#include "Colour.h"
#include "ViewportZoom.h"
#include "geo/DataGroup.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGui
{
	class GLCanvas : public wxGLCanvas
	{
		public:
			explicit
			GLCanvas(MainWindow *parent, 
				 const wxSize &size = wxDefaultSize,
				 const wxPoint &position = wxDefaultPosition);

			/**
			 * Paint the picture.
			 */
			void OnPaint(wxPaintEvent&);

			/**
			 * Set the dimensions of our picture.
			 * Called on startup and when the user resizes the
			 * window.
			 */
			void OnSize(wxSizeEvent&);

			/**
			 * Handle all mouse events.
			 */
			void OnMouseEvent(wxMouseEvent&);

			/**
			 * According to the wxWindows docs, declaring this
			 * function to be empty eliminates flicker on some
			 * platforms (mainly win32).
			 */
			void
			OnEraseBackground(wxEraseEvent&) {  }

			/**
			 * Change the mode of interaction to 'spin globe' mode.
			 */
			void OnSpinGlobe(wxCommandEvent&);

			/**
			 * Zoom in.
			 */
			void ZoomIn();

			/**
			 * Zoom out.
			 */
			void ZoomOut();

			/**
			 * Reset zoom to initial value of 1.
			 */
			void ZoomReset();

#if 0
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

			Globe *GetGlobe() { return &_globe; }
#endif
			
		private:
			MainWindow *_parent;

			wxMenu *_popup_menu;

			Globe _globe;
			int _mouse_x;
			int _mouse_y;
			int _width;
			int _height;
			GLdouble _smaller_dim;
			GLdouble _larger_dim;
			int _wheel_rotation;
			bool _is_initialised;

			ViewportZoom m_viewport_zoom;

			void InitGL();
			void SetView();
			void HandleZoomChange();
			void GetDimensions();
			void ClearCanvas(const Colour &c = Colour::BLACK);

			GPlatesMaths::real_t getUniverseCoordY(int screen_x);
			GPlatesMaths::real_t getUniverseCoordZ(int screen_y);

			/**
			 * Create a new wxMenu (for the popup menu) and
			 * return it.
			 */
			wxMenu *CreatePopupMenu();

			enum mouse_event_type {

				MOUSE_EVENT_DRAG,
				MOUSE_EVENT_DOWN,
				MOUSE_EVENT_UP,
				MOUSE_EVENT_DCLICK
			};

			void HandleLeftMouseEvent(enum mouse_event_type type);

			void HandleWheelRotation(int delta);
			void HandleMouseMotion();

			DECLARE_EVENT_TABLE()
	};
}

#endif  /* _GPLATES_GUI_GLCANVAS_H_ */
