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
#include "Globe.h"
#include <iostream>

namespace GPlatesGui
{
	class GLCanvas : public wxGLCanvas
	{
		public:
			GLCanvas(wxWindow* parent, const wxSize& size = wxDefaultSize,
					 const wxPoint& position = wxDefaultPosition)

				: wxGLCanvas(parent, -1, position, size) {  }

			void InitGL();

			void OnPaint(wxPaintEvent&);
			void OnSize(wxSizeEvent&);
			void OnEraseBackground(wxEraseEvent&) {
				std::cerr << "In \'" << __PRETTY_FUNCTION__ << "\'" << std::endl;
			}

		private:
			Globe _globe;

			DECLARE_EVENT_TABLE()
	};
}

#endif  /* _GPLATES_GUI_GLCANVAS_H_ */
