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

#ifndef _GPLATES_GUI_GLFRAME_H_
#define _GPLATES_GUI_GLFRAME_H_

#include <wx/frame.h>
#include "GLCanvas.h"
#include "Colour.h"

namespace GPlatesGui
{
	/**
	 * GLFrame conforms to the Singleton pattern
	 */
	class GLFrame : public wxFrame
	{
		public:
			GLFrame(wxFrame* parent, 
					const GPlatesGeo::DataGroup* data,
					const wxString& title = "", 
					const wxSize& size = wxDefaultSize,
					const wxPoint& pos = wxDefaultPosition);

			void 
			OnExit(wxCommandEvent&) { Destroy(); }
			
			void
			OnMouseMove(wxMouseEvent&);

		private:
			// XXX: DEFAULT_WINDOWID should be available to the entire GUI system.
//			static const wxWindowID DEFAULT_WINDOWID = -1;
			static const int STATUSBAR_NUM_FIELDS = 1;
		
			wxStatusBar* _status_bar;
			GLCanvas*	 _canvas;

			DECLARE_EVENT_TABLE()
	};
}

#endif  /* _GPLATES_GUI_GLFRAME_H_ */
