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
#include <sstream>
#include <cstdlib>
#include <cmath>  /* for fabsf() */
#include <wx/wx.h>
#include "OpenGL.h"
#include "MainWindow.h"
#include "ReconstructTimeDialog.h"
#include "AnimationTimesDialog.h"
#include "controls/File.h"
#include "controls/View.h"
#include "controls/Reconstruct.h"
#include "global/types.h"
#include "maths/OperationsOnSphere.h"


using namespace GPlatesGui;


/**
 * Menu IDs, all wrapped up to avoid namespace pollution.
 */
namespace {

	enum {
		MENU_FILE_OPENDATA,
		MENU_FILE_OPENROTATION,
		MENU_FILE_SAVEDATA,
		MENU_FILE_EXIT,

		MENU_VIEW_METADATA,
		MENU_VIEW_GLOBE,
		MENU_VIEW_GLOBE_SOLID,
		MENU_VIEW_GLOBE_TRANSPARENT,

		MENU_RECONSTRUCT_TIME,
		MENU_RECONSTRUCT_PRESENT,
		MENU_RECONSTRUCT_ANIMATION
	};
}


MainWindow::MainWindow(wxFrame* parent, const wxString& title,
 const wxSize& size, const wxPoint& pos)
 : wxFrame(parent, -1 /* XXX: DEFAULT_WINDOWID */, title, pos, size)
{
	_status_bar = CreateStatusBar(STATUSBAR_NUM_FIELDS);
	if (!_status_bar) {
		std::cerr << "Failed to create status bar." << std::endl;
		exit(1);
	}

	_last_load_dir = "";
	_last_save_dir = "";

	CreateMenuBar();
	_canvas = new GLCanvas(this);
	_canvas->SetCurrent();

	GPlatesControls::View::Redisplay.SetFrame(_canvas);
		

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
	  "PLATES Data files (*.dat)|*.dat|"
	  "All files (*.*)|*.*"),  // wildcard
	 wxOPEN | wxFILE_MUST_EXIST);  // An 'Open' dialog box

	if (filedlg.ShowModal() == wxID_OK) {
		std::string selected_file;
		_last_load_dir = filedlg.GetDirectory();
		selected_file = filedlg.GetPath().mb_str();
		GPlatesControls::File::OpenData(selected_file);
	}
}


void
MainWindow::OnSaveData(wxCommandEvent&)
{
	wxFileDialog filedlg(this, 
	 _("Designate a file name..."),
	 _last_save_dir, "",  // no default file
	 _("GPlates Data files (*.gpml)|*.gpml|"
	  "All files (*.*)|*.*"),  // wildcard
	 wxSAVE | wxOVERWRITE_PROMPT);  // A 'Save' dialog box

	if (filedlg.ShowModal() == wxID_OK) {
		std::string selected_file;
		_last_save_dir = filedlg.GetDirectory();
		selected_file = filedlg.GetPath().mb_str();
		GPlatesControls::File::SaveData(selected_file);
	}

}


void
MainWindow::OnOpenRotation(wxCommandEvent&)
{
	wxFileDialog filedlg(this, 
	 _("Select a rotation file..."),
	 _last_load_dir, "",  // no default file
	 _("PLATES Rotation files (*.rot)|*.rot|"
	  "All files (*.*)|*.*"),  // wildcard
	 wxOPEN | wxFILE_MUST_EXIST);  // An 'Open' dialog box

	if (filedlg.ShowModal() == wxID_OK) {
		std::string selected_file;
		_last_load_dir = filedlg.GetDirectory();
		selected_file = filedlg.GetPath().mb_str();
		GPlatesControls::File::OpenRotation(selected_file);
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
MainWindow::CreateMenuBar()
{
	wxMenu* filemenu = new wxMenu;
	filemenu->Append(MENU_FILE_OPENDATA, 
	 _("Open &Data...\tCtrl-O"), 
	 _("Open a data file"));
	filemenu->Append(MENU_FILE_OPENROTATION, 
	 _("Open &Rotation...\tCtrl-R"), 
	 _("Open a rotation file"));
	filemenu->Append(MENU_FILE_SAVEDATA,
	 _("&Save Data...\tCtrl-S"),
	 _("Save current data to file"));
	filemenu->AppendSeparator();
	filemenu->Append(MENU_FILE_EXIT,
	 _("&Quit\tCtrl-Q"),
	 _("Exit GPlates"));

	wxMenu* viewmenu = new wxMenu;
	viewmenu->Append(MENU_VIEW_METADATA,
	 _("&View Metadata...\tCtrl-V"),
	 _("View the document's metadata"));

	viewmenu->AppendSeparator();
	wxMenu *viewGlobe_menu = new wxMenu;
	viewGlobe_menu->AppendRadioItem(MENU_VIEW_GLOBE_SOLID,
	 _("&Solid"));
	viewGlobe_menu->AppendRadioItem(MENU_VIEW_GLOBE_TRANSPARENT,
	 _("&Transparent"));
	viewmenu->Append(MENU_VIEW_GLOBE,
	 _("&Globe"),
	 viewGlobe_menu);

	wxMenu* reconstructmenu = new wxMenu;
	reconstructmenu->Append(MENU_RECONSTRUCT_TIME,
	 _("Particular &Time...\tCtrl-T"),
	 _("Reconstruct the data at a particular time"));
	reconstructmenu->Append(MENU_RECONSTRUCT_PRESENT,
	 _("Return to &Present\tCtrl-P"),
	 _("Reconstruct the data at the present"));
	reconstructmenu->Append(MENU_RECONSTRUCT_ANIMATION,
	 _("&Animation...\tCtrl-A"),
	 _("Animate the reconstruction of the data between two times."));
	
	wxMenuBar* menubar = new wxMenuBar(wxMB_DOCKABLE);
	menubar->Append(filemenu, _("&File"));
	menubar->Append(viewmenu, _("&View"));
	menubar->Append(reconstructmenu, _("&Reconstruct"));

	SetMenuBar(menubar);
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_CLOSE(MainWindow::OnExit)
	EVT_MOTION(MainWindow::OnMouseMove)

	EVT_MENU(MENU_FILE_OPENDATA, MainWindow::OnOpenData)
	EVT_MENU(MENU_FILE_OPENROTATION, MainWindow::OnOpenRotation)
	EVT_MENU(MENU_FILE_SAVEDATA, MainWindow::OnSaveData)
	EVT_MENU(MENU_FILE_EXIT, MainWindow::OnExit)

	EVT_MENU(MENU_VIEW_METADATA, MainWindow::OnViewMetadata)
	EVT_MENU(MENU_VIEW_GLOBE_SOLID, MainWindow::OnViewGlobe)
	EVT_MENU(MENU_VIEW_GLOBE_TRANSPARENT, MainWindow::OnViewGlobe)
		
	EVT_MENU(MENU_RECONSTRUCT_TIME, MainWindow::OnReconstructTime)
	EVT_MENU(MENU_RECONSTRUCT_PRESENT, MainWindow::OnReconstructPresent)
	EVT_MENU(MENU_RECONSTRUCT_ANIMATION, MainWindow::OnReconstructAnimation)
END_EVENT_TABLE()
