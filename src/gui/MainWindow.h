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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_MAINWINDOW_H_
#define _GPLATES_GUI_MAINWINDOW_H_

#include <wx/wx.h>
#include "GLCanvas.h"
#include "Colour.h"
#include "global/types.h"  /* fpdata_t */


namespace GPlatesGui
{
	class MainWindow : public wxFrame
	{
		public:
			MainWindow(wxFrame* parent, 
			           const wxString& title = "", 
			           const wxSize& size = wxDefaultSize,
			           const wxPoint& pos = wxDefaultPosition);

			// Mouse events
			void OnMouseMove(wxMouseEvent&);

			/*
			 * Menubar events
			 */

			//File events
			void OnOpenData(wxCommandEvent&);
			void OnLoadRotation(wxCommandEvent&);
			void OnImport(wxCommandEvent&);
			void OnExport(wxCommandEvent&);
			void OnSaveAllData(wxCommandEvent&);
			void OnExit(wxCommandEvent&);

			// View events
			void OnViewMetadata(wxCommandEvent&);
			void OnViewGlobe(wxCommandEvent&);

			// Reconstruct events
			void OnReconstructTime(wxCommandEvent&);
			void OnReconstructPresent(wxCommandEvent&);
			void OnReconstructAnimation(wxCommandEvent&);

			// Help events
			void OnHelpAbout(wxCommandEvent&);

			/**
			 * Set the current geological time in the status bar
			 * to @a t.
			 */
			void SetCurrentTime(const GPlatesGlobal::fpdata_t &t);

			/**
			 * Set the current mode of operation to 'animation'.
			 */
			void SetOpModeToAnimation();

			/**
			 * Return the current mode of operation to 'normal'.
			 */
			void ReturnOpModeToNormal();

			/**
			 * Notify this main window that the animation has been
			 * stopped.
			 */
			void StopAnimation(bool interrupted);

		private:
#if 0
			/*
			 * XXX: DEFAULT_WINDOWID should be available
			 * to the entire GUI system.
			 */
			static const wxWindowID DEFAULT_WINDOWID = -1;
#endif

			/*
			 * Gui components contained within this window.
			 */

			wxMenuBar   *_menu_bar;
			wxStatusBar *_status_bar;
			GLCanvas    *_canvas;

			/*
			 * Cached stuff.
			 */

			// For opening and saving files
			wxString _last_load_dir, _last_save_dir;

			// For animations
			GPlatesGlobal::fpdata_t _last_start_time;
			GPlatesGlobal::fpdata_t _last_end_time;
			GPlatesGlobal::fpdata_t _last_time_delta;
			bool _last_finish_on_end;

			/**
			 * The current geological time.
			 */
			GPlatesGlobal::fpdata_t _current_time;

			/**
			 * Create a new wxMenuBar and return it.
			 */
			wxMenuBar *CreateMenuBar();

			enum operation_modes {

				NORMAL_MODE,
				ANIMATION_MODE
			};

			/**
			 * The current mode of operation.
			 */
			enum operation_modes _operation_mode;

			/**
			 * Declare a wxWindows event table.
			 */
			DECLARE_EVENT_TABLE()
	};
}

#endif  /* _GPLATES_GUI_MAINWINDOW_H_ */
