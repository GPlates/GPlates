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


using namespace GPlatesGui;


/**
 * Menu IDs, all wrapped up to avoid namespace pollution.
 */
namespace {

	enum {
		MENU_FILE_OPENDATA = wxID_HIGHEST + 1,  // To avoid ID clashes
		MENU_FILE_LOADROTATION,
		MENU_FILE_IMPORT,
		MENU_FILE_EXPORT,
		MENU_FILE_SAVEALLDATA,
		MENU_FILE_EXIT,

		MENU_VIEW_METADATA,
#if 0  // globe transparency currently disabled
		MENU_VIEW_GLOBE,
		MENU_VIEW_GLOBE_SOLID,
		MENU_VIEW_GLOBE_TRANSPARENT,
#endif

		MENU_RECONSTRUCT_TIME,
		MENU_RECONSTRUCT_PRESENT,
		MENU_RECONSTRUCT_ANIMATION,

		// Important for the ID to be this value, for the Mac port
		MENU_HELP_ABOUT = wxID_ABOUT
	};
}

const int
GPlatesGui::MainWindow::STATUSBAR_FIELDS[] = {

	-1,  /* variable width */
	100  /* 100 pixels wide */
};


MainWindow::MainWindow(wxFrame* parent, const wxString& title,
 const wxSize& size, const wxPoint& pos)
 : wxFrame(parent, -1 /* XXX: DEFAULT_WINDOWID */, title, pos, size)
{
	const int num_statusbar_fields =
	 static_cast< int >(sizeof(STATUSBAR_FIELDS) / sizeof(int));

	_status_bar = CreateStatusBar(num_statusbar_fields);
	if (!_status_bar) {
		std::cerr << "Failed to create status bar." << std::endl;
		exit(1);
	}
	SetStatusWidths(num_statusbar_fields, STATUSBAR_FIELDS);
	SetCurrentTime(0.0);

	_last_load_dir = "";
	_last_save_dir = "";

	CreateMenuBar();
	_canvas = new GLCanvas(this);
	_canvas->SetCurrent();

	GPlatesControls::GuiCalls::SetComponents(this, _canvas);

	Fit();
	CentreOnScreen();
}

void
MainWindow::OnMouseMove(wxMouseEvent& evt)
{
	using namespace GPlatesMaths;

	PointOnSphere* pos = 
		_canvas->GetSphereCoordFromScreen(evt.GetX(), evt.GetY());
	
	std::ostringstream oss;
	if (pos == NULL)
	{
		oss << "Current Earth coordinate of mouse: (off globe)";
	}
	else
	{
		LatLonPoint point = 
		 OperationsOnSphere::convertPointOnSphereToLatLonPoint(*pos);
		oss << "Current Earth coordinate of mouse: (" 
			<< point.latitude() << ", " << point.longitude() << ")";
		delete pos;
	}

	SetStatusText(wxString(oss.str().c_str(), *wxConvCurrent));
}

void
MainWindow::OnExit(wxCommandEvent&)
{
	Destroy();
	GPlatesControls::File::Quit(0);
}

void
MainWindow::OnOpenData(wxCommandEvent&)
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
MainWindow::OnLoadRotation(wxCommandEvent&)
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

void MainWindow::OnImport(wxCommandEvent&)
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
MainWindow::OnExport(wxCommandEvent&)
{
#if 0
	std::cout << GPlatesControls::File::Export()
		<< std::endl;
#endif
}

void
MainWindow::OnSaveAllData(wxCommandEvent&)
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
MainWindow::OnViewMetadata(wxCommandEvent&)
{
#if 0
	std::cout << GPlatesControls::View::DocumentMetadata()
		<< std::endl;
#endif
}

#if 0  // globe transparency currently disabled
void
MainWindow::OnViewGlobe(wxCommandEvent &event)
{
	Globe *gl = _canvas->GetGlobe();
	if (event.GetId() == MENU_VIEW_GLOBE_SOLID)
		gl->SetTransparency(false);
	else if (event.GetId() == MENU_VIEW_GLOBE_TRANSPARENT)
		gl->SetTransparency(true);

	gl->Paint();
}
#endif

void
MainWindow::OnReconstructTime(wxCommandEvent&)
{	
	ReconstructTimeDialog dialog(this);

	if (dialog.ShowModal() == wxID_OK)
		GPlatesControls::Reconstruct::Time(dialog.GetInput());
}

void
MainWindow::OnReconstructPresent(wxCommandEvent&)
{
	GPlatesControls::Reconstruct::Present();
}

void
MainWindow::OnReconstructAnimation(wxCommandEvent&)
{
	AnimationTimesDialog dialog(this);

	if (dialog.ShowModal() == wxID_OK) {
		GPlatesControls::Reconstruct::Animation(
			dialog.GetStartTime(), 
			dialog.GetEndTime(),
			dialog.GetNSteps());
	}
}

void
MainWindow::OnHelpAbout (wxCommandEvent&)
{	
	AboutDialog dialog (this);

	dialog.ShowModal ();
}

void
GPlatesGui::MainWindow::SetCurrentTime(const GPlatesGlobal::fpdata_t &t) {

	std::ostringstream oss;
	oss
	/* wxWindows doesn't use a fixed-width font in the status bar,
	 * so this code doesn't achieve its aim of aligning all the times. */
#if 0
	 << std::setw(8)          // field width of 8 chars
	 << std::right            // right-align the text in the field
	 << std::setfill(' ')     // fill with a space
	 << std::fixed            // always use decimal notation
	 << std::setprecision(3)  // 3 decimal places
#endif
	 << t
	 << " Ma";
	SetStatusText(wxString(oss.str().c_str(), *wxConvCurrent), 1);
}

void
MainWindow::CreateMenuBar()
{
	wxMenu* filemenu = new wxMenu;
	filemenu->Append(MENU_FILE_OPENDATA, 
	 _("Open &Data..."), 
	 _("Open a native GPlates data file."));
	filemenu->Append(MENU_FILE_LOADROTATION, 
	 _("Load &Rotation...\tCtrl-R"), 
	 _("Load a new rotation file."));
	filemenu->Append (MENU_FILE_IMPORT,
	 _("&Import External Data..."),
	 _("Import a non-native data file."));
	filemenu->Append (MENU_FILE_EXPORT,
	 _("&Export Snapshot..."),
	 _("Export a snapshot of the current state of the data."));
	filemenu->Append(MENU_FILE_SAVEALLDATA,
	 _("&Save All Data\tCtrl-S"),
	 _("Save all data to file."));
	filemenu->AppendSeparator();
	filemenu->Append(MENU_FILE_EXIT,
	 _("&Quit\tCtrl-Q"),
	 _("Exit GPlates."));

	wxMenu* viewmenu = new wxMenu;
	viewmenu->Append(MENU_VIEW_METADATA,
	 _("&View Metadata..."),
	 _("View the document's metadata."));

#if 0  // globe transparency currently disabled
	viewmenu->AppendSeparator();
	wxMenu *viewGlobe_menu = new wxMenu;
	viewGlobe_menu->AppendRadioItem(MENU_VIEW_GLOBE_SOLID,
	 _("&Solid"));
	viewGlobe_menu->AppendRadioItem(MENU_VIEW_GLOBE_TRANSPARENT,
	 _("&Transparent"));
	viewmenu->Append(MENU_VIEW_GLOBE,
	 _("&Globe"),
	 viewGlobe_menu);
#endif

	wxMenu* reconstructmenu = new wxMenu;
	reconstructmenu->Append(MENU_RECONSTRUCT_TIME,
	 _("Jump to &Time...\tCtrl-T"),
	 _("Reconstruct the data at a particular time."));
	reconstructmenu->Append(MENU_RECONSTRUCT_PRESENT,
	 _("Return to &Present\tCtrl-P"),
	 _("Reconstruct the data as it is in the present."));
	reconstructmenu->Append(MENU_RECONSTRUCT_ANIMATION,
	 _("&Animation...\tCtrl-A"),
	 _("Animate the reconstruction of the data between two times."));
	
	wxMenu* helpmenu = new wxMenu;
	helpmenu->Append(MENU_HELP_ABOUT,
	 _("&About GPlates...\tF1"),
	 _("Find out about GPlates."));

	wxMenuBar* menubar = new wxMenuBar(wxMB_DOCKABLE);
	menubar->Append(filemenu, _("&File"));
	menubar->Append(viewmenu, _("&View"));
	menubar->Append(reconstructmenu, _("&Reconstruct"));
	menubar->Append(helpmenu, _("&Help"));

	SetMenuBar(menubar);
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_CLOSE(MainWindow::OnExit)
	EVT_MOTION(MainWindow::OnMouseMove)

	EVT_MENU(MENU_FILE_OPENDATA, MainWindow::OnOpenData)
	EVT_MENU(MENU_FILE_LOADROTATION, MainWindow::OnLoadRotation)
	EVT_MENU(MENU_FILE_IMPORT, MainWindow::OnImport)
	EVT_MENU(MENU_FILE_EXPORT, MainWindow::OnExport)
	EVT_MENU(MENU_FILE_SAVEALLDATA, MainWindow::OnSaveAllData)
	EVT_MENU(MENU_FILE_EXIT, MainWindow::OnExit)

	EVT_MENU(MENU_VIEW_METADATA, MainWindow::OnViewMetadata)
#if 0  // globe transparency currently disabled
	EVT_MENU(MENU_VIEW_GLOBE_SOLID, MainWindow::OnViewGlobe)
	EVT_MENU(MENU_VIEW_GLOBE_TRANSPARENT, MainWindow::OnViewGlobe)
#endif
		
	EVT_MENU(MENU_RECONSTRUCT_TIME, MainWindow::OnReconstructTime)
	EVT_MENU(MENU_RECONSTRUCT_PRESENT, MainWindow::OnReconstructPresent)
	EVT_MENU(MENU_RECONSTRUCT_ANIMATION, MainWindow::OnReconstructAnimation)

	EVT_MENU(MENU_HELP_ABOUT, MainWindow::OnHelpAbout)
END_EVENT_TABLE()
