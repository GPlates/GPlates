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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cmath>  /* for fabsf() */
#include <wx/wx.h>

#include "AboutDialog.h"
#include "AnimationTimesDialog.h"
#include "MainWindow.h"
#include "OpenGL.h"
#include "ReconstructTimeDialog.h"
#include "controls/File.h"
#include "controls/GuiCalls.h"
#include "controls/Reconstruct.h"
#include "global/types.h"
#include "maths/OperationsOnSphere.h"


/**
 * IDs for command and menu events.
 */
namespace EventIDs {

	enum {

		COMMAND_ESCAPE = wxID_HIGHEST + 1,  // To avoid ID clashes

		MENU_FILE_OPENDATA,
		MENU_FILE_LOADROTATION,
		MENU_FILE_IMPORT,
		MENU_FILE_EXPORT,
		MENU_FILE_SAVEALLDATA,
		MENU_FILE_EXIT,

		MENU_VIEW_METADATA,

		MENU_RECONSTRUCT_TIME,
		MENU_RECONSTRUCT_PRESENT,
		MENU_RECONSTRUCT_ANIMATION,

		// Important for the ID to be this value, for the Mac port
		MENU_HELP_ABOUT = wxID_ABOUT
	};
}


/**
 * The menus.
 */
namespace Menus {

	/**
	 * The type of function used to create menu instances.
	 */
	typedef wxMenu *(*create_fn)();


	wxMenu *
	CreateFileMenu() {

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


	wxMenu *
	CreateViewMenu() {

		wxMenu *viewmenu = new wxMenu;

		viewmenu->Append(EventIDs::MENU_VIEW_METADATA,
		 _("&View Metadata..."),
		 _("View the document's metadata."));

		return viewmenu;
	}


	wxMenu *
	CreateReconstructMenu() {

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
	CreateHelpMenu() {

		wxMenu *helpmenu = new wxMenu;

		helpmenu->Append(EventIDs::MENU_HELP_ABOUT,
		 _("&About GPlates...\tF1"),
		 _("Find out about GPlates."));

		return helpmenu;
	}


	struct MenuInstance {

		const char *title;
		create_fn   fn;
	};


	/**
	 * The menu instances.
	 */
	MenuInstance
	INSTANCES[] = {

		{ _("&File"),         CreateFileMenu },
		{ _("&View"),         CreateViewMenu },
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
		MENU_VIEW,
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
		100  /* 100 pixels wide */
	};


	/**
	 * IDs for the statusbar fields.
	 * These are intended to function as indices into the previous array.
	 * Be sure to keep them in sync with the ordering of the previous array.
	 */
	enum {

		INFO = 0,
		POSITION,
		TIME
	};
}


GPlatesGui::MainWindow::MainWindow(wxFrame* parent, const wxString& title,
 const wxSize& size, const wxPoint& pos) :

 wxFrame(parent, -1 /* XXX: DEFAULT_WINDOWID */, title, pos, size),
 _last_start_time(0.0),
 _last_end_time(0.0),
 _last_time_delta(1.0),
 _last_finish_on_end(true)
{
	const int num_statusbar_fields =
	 static_cast< int >(sizeof(StatusbarFields::WIDTHS) /
	                    sizeof(StatusbarFields::WIDTHS[0]));

	_status_bar = CreateStatusBar(num_statusbar_fields);
	if ( ! _status_bar) {

		std::cerr << "Failed to create status bar." << std::endl;
		exit(1);
	}
	SetStatusWidths(num_statusbar_fields, StatusbarFields::WIDTHS);
	SetCurrentTime(0.0);

	_last_load_dir = "";
	_last_save_dir = "";

	_menu_bar = CreateMenuBar();
	if ( ! _menu_bar) {

		std::cerr << "Failed to create menu bar." << std::endl;
		exit(1);
	}
	SetMenuBar(_menu_bar);

	_canvas = new GLCanvas(this);
	_canvas->SetCurrent();

	GPlatesControls::GuiCalls::SetComponents(this, _canvas);

	Fit();
	CentreOnScreen();
}


void
GPlatesGui::MainWindow::OnMouseMove(wxMouseEvent& evt)
{
	using namespace GPlatesMaths;

	PointOnSphere* pos = 
		_canvas->GetSphereCoordFromScreen(evt.GetX(), evt.GetY());
	
	std::ostringstream oss;
	if (pos == NULL)
	{
		oss << "(off globe)";
	}
	else
	{
		LatLonPoint point = 
		 OperationsOnSphere::convertPointOnSphereToLatLonPoint(*pos);
		oss << "(" << point.latitude() << ", "
		 << point.longitude() << ")";
		delete pos;
	}

	SetStatusText(wxString(oss.str().c_str(), *wxConvCurrent),
	              StatusbarFields::POSITION);
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
	  "NetCDF Grid files (*.grd)|*.grd|"
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
GPlatesGui::MainWindow::OnViewMetadata(wxCommandEvent&)
{
#if 0
	std::cout << GPlatesControls::View::DocumentMetadata()
		<< std::endl;
#endif
}


void
GPlatesGui::MainWindow::OnReconstructTime(wxCommandEvent&)
{	
	ReconstructTimeDialog dialog(this);

	if (dialog.ShowModal() == wxID_OK)
		GPlatesControls::Reconstruct::Time(dialog.GetTime());
}


void
GPlatesGui::MainWindow::OnReconstructPresent(wxCommandEvent&)
{
	GPlatesControls::Reconstruct::Present();
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

		GPlatesControls::Reconstruct::Animation(_last_start_time,
		 _last_end_time, _last_time_delta, _last_finish_on_end);
	}
}


void
GPlatesGui::MainWindow::OnHelpAbout (wxCommandEvent&)
{	
	AboutDialog dialog (this);

	dialog.ShowModal ();
}


void
GPlatesGui::MainWindow::SetCurrentTime(const GPlatesGlobal::fpdata_t &t) {

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


wxMenuBar *
GPlatesGui::MainWindow::CreateMenuBar()
{
	const size_t num_menu_instances = sizeof(Menus::INSTANCES) /
	                                  sizeof(Menus::INSTANCES[0]);

	wxMenuBar *menu_bar = new wxMenuBar(wxMB_DOCKABLE);

	for (size_t i = 0; i < num_menu_instances; i++) {

		const char *title = Menus::INSTANCES[i].title;
		Menus::create_fn fn = Menus::INSTANCES[i].fn;

		menu_bar->Append(fn(), title);
	}
	return menu_bar;
}


BEGIN_EVENT_TABLE(GPlatesGui::MainWindow, wxFrame)

	EVT_CLOSE(	GPlatesGui::MainWindow::OnExit)
	EVT_MOTION(	GPlatesGui::MainWindow::OnMouseMove)

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

	EVT_MENU(EventIDs::MENU_VIEW_METADATA,
			GPlatesGui::MainWindow::OnViewMetadata)

	EVT_MENU(EventIDs::MENU_RECONSTRUCT_TIME,
			GPlatesGui::MainWindow::OnReconstructTime)
	EVT_MENU(EventIDs::MENU_RECONSTRUCT_PRESENT,
			GPlatesGui::MainWindow::OnReconstructPresent)
	EVT_MENU(EventIDs::MENU_RECONSTRUCT_ANIMATION,
			GPlatesGui::MainWindow::OnReconstructAnimation)

	EVT_MENU(EventIDs::MENU_HELP_ABOUT,
			GPlatesGui::MainWindow::OnHelpAbout)

END_EVENT_TABLE()
