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

#ifndef _GPLATES_GUI_GLFRAME_H_
#define _GPLATES_GUI_GLFRAME_H_

#include <wx/wx.h>
#include "GLCanvas.h"
#include "Colour.h"

namespace GPlatesGui
{
	class GLFrame : public wxFrame
	{
		public:
			/*
			 * Note that '_(str)' is a gettext-style macro alias
			 * for 'wxGetTranslation(str)'.
			 */
			GLFrame(wxFrame* parent, 
					const wxString& title = _(""), 
					const wxSize& size = wxDefaultSize,
					const wxPoint& pos = wxDefaultPosition);

			void OnExit(wxCommandEvent&);

			// Mouse events
			void OnMouseMove(wxMouseEvent&);

			// Menubar events

			//   File events
			void OnOpenData(wxCommandEvent&);
			void OnOpenRotation(wxCommandEvent&);
			void OnSaveData(wxCommandEvent&);

			//   View events
			void OnViewMetadata(wxCommandEvent&);

			//   Reconstruct events
			void OnReconstructTime(wxCommandEvent&);
			void OnReconstructPresent(wxCommandEvent&);
			void OnReconstructAnimation(wxCommandEvent&);

		private:
			// XXX: DEFAULT_WINDOWID should be available to the entire GUI system.
//			static const wxWindowID DEFAULT_WINDOWID = -1;
			static const int STATUSBAR_NUM_FIELDS = 1;
		
			wxStatusBar* _status_bar;
			GLCanvas*	 _canvas;
			wxString _last_load_dir, _last_save_dir;

			DECLARE_EVENT_TABLE()
	};
}

#endif  /* _GPLATES_GUI_GLFRAME_H_ */
