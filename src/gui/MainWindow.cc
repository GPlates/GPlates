/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cmath>  /* for fabsf() */
#include <wx/wx.h>

#include "MainWindow.h"
#include "GLCanvas.h"
#include "EventIDs.h"
#include "AboutDialog.h"
#include "AnimationTimesDialog.h"
#include "OpenGL.h"
#include "ReconstructTimeDialog.h"
#include "controls/File.h"
#include "controls/GuiCalls.h"
#include "controls/Reconstruct.h"
#include "controls/AnimationTimer.h"
#include "global/types.h"

// Pixmaps for icons
#include "pixmaps/stock_zoom_in_24.xpm"
#include "pixmaps/stock_zoom_out_24.xpm"
#include "pixmaps/zoom_initial_24.xpm"
#include "pixmaps/mode_observation_24.xpm"
#include "pixmaps/mode_plate_manip_24.xpm"
#include "pixmaps/stock_stop_24.xpm"
#include "pixmaps/help_24.xpm"


/**
 * The menus.
 */
namespace Menus {

	using namespace GPlatesGui;


	/**
	 * The type of function used to create menu instances.
	 */
	typedef wxMenu *(*create_fn)();


	wxMenu *
	CreateFileMenu()
	{
		wxMenu *filemenu = new wxMenu;

		filemenu->Append(EventIDs::MENU_FILE_OPENDATA, 
		 _("Open &Data..."), 
		 _("Open a native GPlates data file."));
		filemenu->Append(EventIDs::MENU_FILE_LOADROTATION, 
		 _("Load &Rotation...\tCtrl-R"), 
		 _("Load a new rotation file."));
		filemenu->Append(EventIDs::MENU_FILE_IMPORT,
		 _("&Import External Data..."),
		 _("Import a non-native data file."));
		filemenu->Append(EventIDs::MENU_FILE_EXPORT,
		 _("&Export Snapshot..."),
		 _("Export a snapshot of the current state of the data."));
		filemenu->Append(EventIDs::MENU_FILE_SAVEALLDATA,
		 _("&Save All Data\tCtrl-S"),
		 _("Save all data to file."));
		filemenu->AppendSeparator();
		filemenu->Append(EventIDs::MENU_FILE_EXIT,
		 _("&Quit\tCtrl-Q"),
		 _("Exit GPlates."));

		return filemenu;
	}


#if 0
	wxMenu *
	CreateViewMenu()
	{
		wxMenu *viewmenu = new wxMenu;

		viewmenu->Append(EventIDs::MENU_VIEW_ZOOM_IN,
		 _("Zoom &In\tCtrl-I"),
		 _("Zoom in on the globe."));
		viewmenu->Append(EventIDs::MENU_VIEW_ZOOM_OUT,
		 _("Zoom &Out\tCtrl-O"),
		 _("Zoom out from the globe."));

		return viewmenu;
	}
#endif


	wxMenu *
	CreateReconstructMenu()
	{
		wxMenu *reconstructmenu = new wxMenu;

		reconstructmenu->Append(EventIDs::MENU_RECONSTRUCT_TIME,
		 _("Jump to &Time...\tCtrl-T"),
		 _("Reconstruct the data at a particular time."));
		reconstructmenu->Append(EventIDs::MENU_RECONSTRUCT_PRESENT,
		 _("Return to &Present\tCtrl-P"),
		 _("Reconstruct the data as it is in the present."));
		reconstructmenu->Append(EventIDs::MENU_RECONSTRUCT_ANIMATION,
		 _("&Animation...\tCtrl-A"),
		 _("Animate the reconstruction of the data between two times.")
		);

		return reconstructmenu;
	}


	wxMenu *
	CreateHelpMenu()
	{
		wxMenu *helpmenu = new wxMenu;

		helpmenu->Append(EventIDs::MENU_HELP_ABOUT,
		 _("&About GPlates...\tF1"),
		 _("Find out about GPlates."));

		return helpmenu;
	}


	struct MenuInstance
	{
		const char *title;
		create_fn   fn;
	};


	/**
	 * The menu instances.
	 */
	MenuInstance
	INSTANCES[] = {

		{ _("&File"),         CreateFileMenu },
#if 0
		{ _("&View"),         CreateViewMenu },
#endif
		{ _("&Reconstruct"),  CreateReconstructMenu },
		{ _("&Help"),         CreateHelpMenu }
	};


	/**
	 * IDs for the menu instances.
	 * These are intended to function as indices into the array of
	 * menu instances.
	 * Be sure to keep them in sync with the ordering of the previous array.
	 */
	enum {

		MENU_FILE = 0,
#if 0
		MENU_VIEW,
#endif
		MENU_RECONSTRUCT,
		MENU_HELP
	};
}


/**
 * The statusbar fields.
 */
namespace StatusbarFields {

	/**
	 * The widths of the statusbar fields.
	 */
	const int
	WIDTHS[] = {

		-1,  /* variable width */
		150, /* 150 pixels wide */
		90,  /* 90 pixels wide */
		50   /* 50 pixels wide */
	};


	/**
	 * IDs for the statusbar fields.
	 * These are intended to function as indices into the previous array.
	 * Be sure to keep them in sync with the ordering of the previous array.
	 */
	enum {

		INFO = 0,
		POSITION,
		TIME,
		ZOOM
	};
}


/**
 * Event handlers used for keyboard "accelerators" (shortcuts).
 */
namespace {

	/**
	 * Extra event-handling functionality used during animations.
	 */
	class AnimEvtHandler : public wxEvtHandler
	{
		public:
			AnimEvtHandler(GPlatesGui::MainWindow *w) :
			 _main_window(w) {  }

			void
			OnEscape(wxCommandEvent&) {

				GPlatesControls::AnimationTimer::StopTimer();
			}

		private:
			GPlatesGui::MainWindow *_main_window;

			DECLARE_EVENT_TABLE()
	};
}


GPlatesGui::MainWindow::MainWindow(wxFrame* parent, const wxString& title,
 const wxSize& size, const wxPoint& pos) :

 wxFrame(parent, -1 /* XXX: DEFAULT_WINDOWID */, title, pos, size),
 _last_start_time(0.0),
 _last_end_time(0.0),
 _last_time_delta(1.0),
 _last_finish_on_end(true),
 _operation_mode(NORMAL_MODE)
{
	_menu_bar = CreateMenuBar();
	if ( ! _menu_bar) {

		std::cerr << "Failed to create menu bar." << std::endl;
		std::exit(1);
	}
	SetMenuBar(_menu_bar);

	_tool_bar = CreateToolBar_(wxTB_HORIZONTAL);
	if ( ! _tool_bar) {

		std::cerr << "Failed to create tool bar." << std::endl;
		std::exit(1);
	}
	SetToolBar(_tool_bar);
	SetAcceleratorTable(MainWindow::DefaultAccelTab());
	// Disable "stop" button until animations
	_tool_bar->EnableTool(EventIDs::TOOLBAR_STOP, false);

	const int num_statusbar_fields =
	 static_cast< int >(sizeof(StatusbarFields::WIDTHS) /
	                    sizeof(StatusbarFields::WIDTHS[0]));

	_status_bar = CreateStatusBar(num_statusbar_fields);
	if ( ! _status_bar) {

		std::cerr << "Failed to create status bar." << std::endl;
		std::exit(1);
	}
	SetStatusWidths(num_statusbar_fields, StatusbarFields::WIDTHS);
	SetCurrentTime(0.0);
	SetCurrentZoom(100);

	_last_load_dir = "";
	_last_save_dir = "";

	_canvas = new GLCanvas(this);
	_canvas->SetCurrent();

	/*
	 * NOTE: without this next function call, the keyboard shortcuts
	 * will not work until after the user has clicked inside the GLCanvas
	 * frame.
	 */
	_canvas->SetFocus();

	GPlatesControls::GuiCalls::SetComponents(this, _canvas);

	Fit();
	CentreOnScreen();
}


void
GPlatesGui::MainWindow::OnOpenData(wxCommandEvent&)
{
	wxFileDialog filedlg(this,
	 _("Select a data file..."),
	 _last_load_dir, "",  // no default file
	 _("GPlates Data files (*.gpml)|*.gpml|"
	  "All files (*)|*"),  // wildcard
	 wxOPEN | wxFILE_MUST_EXIST);  // An 'Open' dialog box

	if (filedlg.ShowModal() == wxID_OK) {

		std::string selected_file;
		_last_load_dir = filedlg.GetDirectory();
		selected_file = filedlg.GetPath().mb_str();
		GPlatesControls::File::OpenData(selected_file);
	}
}


void
GPlatesGui::MainWindow::OnLoadRotation(wxCommandEvent&)
{
	wxFileDialog filedlg(this, 
	 _("Select a rotation file..."),
	 _last_load_dir, "",  // no default file
	 _("PLATES Rotation files (*.rot)|*.rot|"
	  "All files (*)|*"),  // wildcard
	 wxOPEN | wxFILE_MUST_EXIST);  // An 'Open' dialog box

	if (filedlg.ShowModal() == wxID_OK) {

		std::string selected_file;
		_last_load_dir = filedlg.GetDirectory();
		selected_file = filedlg.GetPath().mb_str();
		GPlatesControls::File::LoadRotation(selected_file);
	}
}


void
GPlatesGui::MainWindow::OnImport(wxCommandEvent&)
{
	wxFileDialog filedlg (this,
	 _("Select a data file to import..."),
	 _last_load_dir, "",  // no default file
	 _("PLATES Data files (*.dat)|*.dat|"
//	  "NetCDF Grid files (*.grd)|*.grd|"
	  "All files (*)|*"),  // wildcard
	 wxOPEN | wxFILE_MUST_EXIST);  // An 'Open' dialog box

	if (filedlg.ShowModal () == wxID_OK) {

		std::string selected_file;
		_last_load_dir = filedlg.GetDirectory ();
		selected_file = filedlg.GetPath ().mb_str ();
		GPlatesControls::File::ImportData(selected_file);
	}
}


void
GPlatesGui::MainWindow::OnExport(wxCommandEvent&)
{
#if 0
	std::cout << GPlatesControls::File::Export()
		<< std::endl;
#endif
}


void
GPlatesGui::MainWindow::OnSaveAllData(wxCommandEvent&)
{
	wxFileDialog filedlg(this, 
	 _("Designate a file name..."),
	 _last_save_dir, "",  // no default file
	 _("GPlates Data files (*.gpml)|*.gpml|"
	  "All files (*)|*"),  // wildcard
	 wxSAVE | wxOVERWRITE_PROMPT);  // A 'Save' dialog box

	if (filedlg.ShowModal() == wxID_OK) {

		std::string selected_file;
		_last_save_dir = filedlg.GetDirectory();
		selected_file = filedlg.GetPath().mb_str();
		GPlatesControls::File::SaveData(selected_file);
	}

}


void
GPlatesGui::MainWindow::OnExit(wxCommandEvent&)
{
	Destroy();
	GPlatesControls::File::Quit(0);
}


void
GPlatesGui::MainWindow::OnZoomIn(wxCommandEvent&)
{
	_canvas->ZoomIn();
}


void
GPlatesGui::MainWindow::OnZoomOut(wxCommandEvent&)
{
	_canvas->ZoomOut();
}


void
GPlatesGui::MainWindow::OnZoomReset(wxCommandEvent&)
{
	_canvas->ZoomReset();
}


void
GPlatesGui::MainWindow::OnReconstructTime(wxCommandEvent&)
{	
	ReconstructTimeDialog dialog(this);

	if (dialog.ShowModal() == wxID_OK)
	{
#if 0  // FIXME:  This is just if-0-ed-out for now, so I don't have to gut the whole program.
	// Ultimately, it should *all* go.
		GPlatesControls::Reconstruct::Time(dialog.GetTime());
#endif
	}
}


void
GPlatesGui::MainWindow::OnReconstructPresent(wxCommandEvent&)
{
#if 0  // FIXME:  This is just if-0-ed-out for now, so I don't have to gut the whole program.
	// Ultimately, it should *all* go.
	GPlatesControls::Reconstruct::Present();
#endif
}


void
GPlatesGui::MainWindow::OnReconstructAnimation(wxCommandEvent&)
{
	AnimationTimesDialog dialog(this, _last_start_time, _last_end_time,
	                             _last_time_delta, _last_finish_on_end);

	if (dialog.ShowModal() == wxID_OK) {

		_last_start_time = dialog.GetStartTime();
		_last_end_time = dialog.GetEndTime();
		_last_time_delta = dialog.GetTimeDelta();
		_last_finish_on_end = dialog.GetFinishOnEnd();

#if 0  // FIXME:  This is just if-0-ed-out for now, so I don't have to gut the whole program.
	// Ultimately, it should *all* go.
		GPlatesControls::Reconstruct::Animation(_last_start_time,
		 _last_end_time, _last_time_delta, _last_finish_on_end);
#endif
	}
}


void
GPlatesGui::MainWindow::OnHelpAbout (wxCommandEvent&)
{	
	AboutDialog dialog (this);

	dialog.ShowModal ();
}


void
GPlatesGui::MainWindow::SetCurrentTime(const GPlatesGlobal::fpdata_t &t)
{
	std::ostringstream oss;
	oss
#if 0  /* wxWindows doesn't use a fixed-width font in the status bar,
        * so this code doesn't achieve its aim of aligning all the times. */
	 << std::setw(8)          // field width of 8 chars
	 << std::right            // right-align the text in the field
	 << std::setfill(' ')     // fill with a space
	 << std::fixed            // always use decimal notation
	 << std::setprecision(3)  // 3 decimal places
#endif
	 << t
	 << " Ma";
	SetStatusText(wxString(oss.str().c_str(), *wxConvCurrent),
	              StatusbarFields::TIME);
}


void
GPlatesGui::MainWindow::SetCurrentZoom(unsigned z)
{
	std::ostringstream oss;
	oss << z << "%";
	SetStatusText(wxString(oss.str().c_str(), *wxConvCurrent),
	              StatusbarFields::ZOOM);
}


void
GPlatesGui::MainWindow::SetCurrentGlobePosOffGlobe()
{
	SetStatusText(wxString("(off globe)", *wxConvCurrent),
	              StatusbarFields::POSITION);
}


void
GPlatesGui::MainWindow::SetCurrentGlobePos(const GPlatesGlobal::fpdata_t &lat,
	const GPlatesGlobal::fpdata_t &lon)
{
	std::ostringstream oss;
	oss << "("
	 << std::fixed << std::setprecision(4) << lat << ", "
	 << std::fixed << std::setprecision(4) << lon << ")";
	SetStatusText(wxString(oss.str().c_str(), *wxConvCurrent),
	              StatusbarFields::POSITION);
}


void
GPlatesGui::MainWindow::SetOpModeToAnimation()
{
	if (_operation_mode != NORMAL_MODE) {

		// FIXME: should we complain?  For now, do nothing.
		return;
	}

	// A new event handler pushed onto handler stack
	PushEventHandler(new AnimEvtHandler(this));

	// A new set of keyboard "accelerators" (ie, shortcuts)
	// FIXME: do this more elegantly... (ie, without the duplication)
	wxAcceleratorEntry accels[4];
	accels[0].Set(wxACCEL_SHIFT, '=', EventIDs::COMMAND_PLUS);
	accels[1].Set(wxACCEL_NORMAL, '-', EventIDs::COMMAND_MINUS);
	accels[2].Set(wxACCEL_NORMAL, '1', EventIDs::COMMAND_1);
	accels[3].Set(wxACCEL_NORMAL, WXK_ESCAPE, EventIDs::COMMAND_ESCAPE);
	size_t num_accels = sizeof(accels) / sizeof(accels[0]);
	wxAcceleratorTable accel_tab(num_accels, accels);
	SetAcceleratorTable(accel_tab);

	// Disable all menus
	size_t num_menus = sizeof(Menus::INSTANCES) /
	                   sizeof(Menus::INSTANCES[0]);

	for (size_t i = 0; i < num_menus; i++) {

		// Disable menu 'i'
		_menu_bar->EnableTop(i, false);
	}

	// Enable "stop" button
	_tool_bar->EnableTool(EventIDs::TOOLBAR_STOP, true);

	SetStatusText(_("Press Esc to interrupt animation."),
	              StatusbarFields::INFO);

	// Operation mode has been changed
	_operation_mode = ANIMATION_MODE;
}


void
GPlatesGui::MainWindow::ReturnOpModeToNormal()
{
	if (_operation_mode == NORMAL_MODE) {

		// FIXME: should we complain?  For now, do nothing.
		return;
	}

	// Pop animation event handler from handler stack (and delete it)
	PopEventHandler(true);

	// Remove animation keyboard accelerators
	SetAcceleratorTable(MainWindow::DefaultAccelTab());

	// Re-enable all menus
	size_t num_menus = sizeof(Menus::INSTANCES) /
	                   sizeof(Menus::INSTANCES[0]);

	for (size_t i = 0; i < num_menus; i++) {

		// Re-enable menu 'i'
		_menu_bar->EnableTop(i, true);
	}

	// Disable "stop" button again
	_tool_bar->EnableTool(EventIDs::TOOLBAR_STOP, false);

	// Operation mode has been returned to normal
	_operation_mode = NORMAL_MODE;
}


void
GPlatesGui::MainWindow::StopAnimation(bool interrupted)
{
	if (interrupted) {

		// The animation was prematurely interrupted
		SetStatusText(_("Animation interrupted."),
		              StatusbarFields::INFO);

	} else {

		SetStatusText(_("Animation finished."),
		              StatusbarFields::INFO);
	}
}


wxMenuBar *
GPlatesGui::MainWindow::CreateMenuBar(long style)
{
	size_t num_menus = sizeof(Menus::INSTANCES) /
	                   sizeof(Menus::INSTANCES[0]);

	wxMenuBar *menu_bar = new wxMenuBar(style);

	for (size_t i = 0; i < num_menus; i++) {

		const char *title = Menus::INSTANCES[i].title;
		Menus::create_fn fn = Menus::INSTANCES[i].fn;

		menu_bar->Append(fn(), title);
	}
	return menu_bar;
}


wxToolBar *
GPlatesGui::MainWindow::CreateToolBar_(long style)
{
	wxToolBar *tool_bar = wxFrame::CreateToolBar(style);

	tool_bar->SetMargins(2, 2);

	wxBitmap *mode_observation_bitmap =
	 new wxBitmap(mode_observation_24_xpm);
	tool_bar->AddTool(EventIDs::TOOLBAR_MODE_OBSERVATION,
	                   "Enter Observation Mode", *mode_observation_bitmap,
	                   "Enter Observation Mode    F3", wxITEM_NORMAL);
	wxBitmap *mode_plate_manip_bitmap =
	 new wxBitmap(mode_plate_manip_24_xpm);
	tool_bar->AddTool(EventIDs::TOOLBAR_MODE_PLATE_MANIP,
	                   "Enter Plate Manipulation Mode",
	                   *mode_plate_manip_bitmap,
	                   "Enter Plate Manipulation Mode    F4",
	                   wxITEM_NORMAL);

	tool_bar->AddSeparator();

	wxBitmap *zoom_in_bitmap = new wxBitmap(stock_zoom_in_24_xpm);
	tool_bar->AddTool(EventIDs::TOOLBAR_ZOOM_IN, "Zoom In",
	                   *zoom_in_bitmap, "Zoom In    +", wxITEM_NORMAL);
	wxBitmap *zoom_out_bitmap = new wxBitmap(stock_zoom_out_24_xpm);
	tool_bar->AddTool(EventIDs::TOOLBAR_ZOOM_OUT, "Zoom Out",
	                   *zoom_out_bitmap, "Zoom Out    -", wxITEM_NORMAL);
	wxBitmap *zoom_initial_bitmap = new wxBitmap(zoom_initial_24_xpm);
	tool_bar->AddTool(EventIDs::TOOLBAR_ZOOM_RESET, "Reset Zoom",
	                   *zoom_initial_bitmap, "Reset Zoom    1",
	                   wxITEM_NORMAL);

	tool_bar->AddSeparator();

	wxBitmap *stop_bitmap = new wxBitmap(stock_stop_24_xpm);
	tool_bar->AddTool(EventIDs::TOOLBAR_STOP, "Stop Animation",
	                   *stop_bitmap, "Stop Animation    Esc",
	                   wxITEM_NORMAL);

	tool_bar->AddSeparator();

	wxBitmap *help_bitmap = new wxBitmap(help_24_xpm);
	tool_bar->AddTool(EventIDs::TOOLBAR_HELP, "Open Help Browser",
	                   *help_bitmap, "Open Help Browser    Shift+F1",
	                   wxITEM_NORMAL);

	return tool_bar;
}


wxAcceleratorTable
GPlatesGui::MainWindow::DefaultAccelTab()
{
	wxAcceleratorEntry accels[3];
	accels[0].Set(wxACCEL_SHIFT, '=', EventIDs::COMMAND_PLUS);
	accels[1].Set(wxACCEL_NORMAL, '-', EventIDs::COMMAND_MINUS);
	accels[2].Set(wxACCEL_NORMAL, '1', EventIDs::COMMAND_1);
	size_t num_accels = sizeof(accels) / sizeof(accels[0]);
	wxAcceleratorTable accel_tab(num_accels, accels);

	return accel_tab;
}


BEGIN_EVENT_TABLE(GPlatesGui::MainWindow, wxFrame)

	EVT_CLOSE(	GPlatesGui::MainWindow::OnExit)

	EVT_MENU(EventIDs::MENU_FILE_OPENDATA,
			GPlatesGui::MainWindow::OnOpenData)
	EVT_MENU(EventIDs::MENU_FILE_LOADROTATION,
			GPlatesGui::MainWindow::OnLoadRotation)
	EVT_MENU(EventIDs::MENU_FILE_IMPORT,
			GPlatesGui::MainWindow::OnImport)
	EVT_MENU(EventIDs::MENU_FILE_EXPORT,
			GPlatesGui::MainWindow::OnExport)
	EVT_MENU(EventIDs::MENU_FILE_SAVEALLDATA,
			GPlatesGui::MainWindow::OnSaveAllData)
	EVT_MENU(EventIDs::MENU_FILE_EXIT,
			GPlatesGui::MainWindow::OnExit)

#if 0
	EVT_MENU(EventIDs::MENU_VIEW_ZOOM_IN,
			GPlatesGui::MainWindow::OnViewZoomIn)
	EVT_MENU(EventIDs::MENU_VIEW_ZOOM_OUT,
			GPlatesGui::MainWindow::OnViewZoomOut)
#endif

	EVT_MENU(EventIDs::MENU_RECONSTRUCT_TIME,
			GPlatesGui::MainWindow::OnReconstructTime)
	EVT_MENU(EventIDs::MENU_RECONSTRUCT_PRESENT,
			GPlatesGui::MainWindow::OnReconstructPresent)
	EVT_MENU(EventIDs::MENU_RECONSTRUCT_ANIMATION,
			GPlatesGui::MainWindow::OnReconstructAnimation)

	EVT_MENU(EventIDs::MENU_HELP_ABOUT,
			GPlatesGui::MainWindow::OnHelpAbout)

	EVT_MENU(GPlatesGui::EventIDs::COMMAND_PLUS,
			GPlatesGui::MainWindow::OnZoomIn)
	EVT_MENU(GPlatesGui::EventIDs::COMMAND_MINUS,
			GPlatesGui::MainWindow::OnZoomOut)
	EVT_MENU(GPlatesGui::EventIDs::COMMAND_1,
			GPlatesGui::MainWindow::OnZoomReset)

	EVT_TOOL(EventIDs::TOOLBAR_ZOOM_IN,
			GPlatesGui::MainWindow::OnZoomIn)
	EVT_TOOL(EventIDs::TOOLBAR_ZOOM_OUT,
			GPlatesGui::MainWindow::OnZoomOut)
	EVT_TOOL(EventIDs::TOOLBAR_ZOOM_RESET,
			GPlatesGui::MainWindow::OnZoomReset)

END_EVENT_TABLE()


BEGIN_EVENT_TABLE(AnimEvtHandler, wxEvtHandler)

	EVT_MENU(GPlatesGui::EventIDs::COMMAND_ESCAPE,
			AnimEvtHandler::OnEscape)

	EVT_TOOL(GPlatesGui::EventIDs::TOOLBAR_STOP,
			AnimEvtHandler::OnEscape)

END_EVENT_TABLE()
