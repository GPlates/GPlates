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

#ifndef _GPLATES_GUI_MAINWINDOW_H_
#define _GPLATES_GUI_MAINWINDOW_H_

#include <wx/wx.h>
#include "Colour.h"
#include "global/types.h"  /* fpdata_t */


namespace GPlatesGui
{
	class GLCanvas;

	class MainWindow : public wxFrame
	{
		public:
			explicit
			MainWindow(wxFrame* parent, 
			           const wxString& title = "", 
			           const wxSize& size = wxDefaultSize,
			           const wxPoint& pos = wxDefaultPosition);

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

			// Reconstruct events
			void OnReconstructTime(wxCommandEvent&);
			void OnReconstructPresent(wxCommandEvent&);
			void OnReconstructAnimation(wxCommandEvent&);

			// Help events
			void OnHelpAbout(wxCommandEvent&);

			/*
			 * Toolbar events
			 */
			void OnZoomIn(wxCommandEvent&);
			void OnZoomOut(wxCommandEvent&);
			void OnZoomReset(wxCommandEvent&);

			/**
			 * Set the current geological time (as displayed in
			 * the status bar) to @a t.
			 */
			void SetCurrentTime(const GPlatesGlobal::fpdata_t &t);

			/**
			 * Set the current zoom (as displayed in the status
			 * bar) to @a z percent.
			 */
			void SetCurrentZoom(unsigned z);

			/**
			 * Set the current position on the globe (as displayed
			 * in the status bar) to "(off globe)".
			 */
			void SetCurrentGlobePosOffGlobe();

			/**
			 * Set the current position on the globe (as displayed
			 * in the status bar) to (@a lat, @a lon).
			 */
			void SetCurrentGlobePos(const GPlatesGlobal::fpdata_t
			 &lat, const GPlatesGlobal::fpdata_t &lon);

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
			wxToolBar   *_tool_bar;
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
			 * Create a new wxMenuBar and return it.
			 */
			wxMenuBar *CreateMenuBar(long style = 0);

			/**
			 * Create a new wxToolBar and return it.
			 *
			 * 2006-08-30: Appended an underscore to the end of the
			 * function name so that it doesn't hide the virtual
			 * function of the same name in the base class.
			 *
			 * Note that this class is going to be re-written, so
			 * there's no need to bother inventing a new naming
			 * scheme.
			 */
			wxToolBar *CreateToolBar_(long style = 0);

			static wxAcceleratorTable DefaultAccelTab();

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
