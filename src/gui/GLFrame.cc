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

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cmath>  /* for fabsf() */
#include <wx/wx.h>
#include "OpenGL.h"
#include "GLFrame.h"
#include "ReconstructTimeDialog.h"
#include "AnimationTimesDialog.h"
#include "controls/File.h"
#include "controls/View.h"
#include "controls/Reconstruct.h"


using namespace GPlatesGui;


/**
 * Menu IDs.
 */
enum {
	MENU_FILE_OPENDATA,
	MENU_FILE_OPENROTATION,
	MENU_FILE_EXIT,

	MENU_VIEW_METADATA,
	
	MENU_RECONSTRUCT_TIME,
	MENU_RECONSTRUCT_PRESENT,
	MENU_RECONSTRUCT_ANIMATION
};


static void
CreateMenuBar(GLFrame* frame)
{
	wxMenu* filemenu = new wxMenu;
	filemenu->Append(MENU_FILE_OPENDATA, 
					 _("Open &Data...\tCtrl-O"), 
					 _("Open a data file"));
	filemenu->Append(MENU_FILE_OPENROTATION, 
					 _("Open &Rotation...\tCtrl-R"), 
					 _("Open a rotation file"));
	filemenu->AppendSeparator();
	filemenu->Append(MENU_FILE_EXIT, _("&Quit\tCtrl-Q"), _("Exit GPlates"));

	wxMenu* viewmenu = new wxMenu;
	viewmenu->Append(MENU_VIEW_METADATA,
					 _("&View Metadata...\tCtrl-V"),
					 _("View the document's metadata"));

	wxMenu* reconstructmenu = new wxMenu;
	reconstructmenu->Append(MENU_RECONSTRUCT_TIME,
							_("Particular &Time...\tCtrl-T"),
							_("Reconstruct the data at a particular time"));
	reconstructmenu->Append(MENU_RECONSTRUCT_PRESENT,
							_("Return to &Present\tCtrl-P"),
							_("Reconstruct the data at the present"));
	reconstructmenu->Append(MENU_RECONSTRUCT_ANIMATION,
							_("&Animation...\tCtrl-A"),
							_("Animate the reconstruction of the data between "
							"two times."));
	
	wxMenuBar* menubar = new wxMenuBar(wxMB_DOCKABLE);
	menubar->Append(filemenu, _("&File"));
	menubar->Append(viewmenu, _("&View"));
	menubar->Append(reconstructmenu, _("&Reconstruct"));

	frame->SetMenuBar(menubar);
}

GLFrame::GLFrame(wxFrame* parent,
	const wxString& title, const wxSize& size, const wxPoint& pos)
 : wxFrame(parent, -1 /* XXX: DEFAULT_WINDOWID */, title, pos, size)
{
	_status_bar = CreateStatusBar(STATUSBAR_NUM_FIELDS);
	if (!_status_bar) {
		std::cerr << "Failed to create status bar." << std::endl;
		exit(1);
	}

	CreateMenuBar(this);
	_canvas = new GLCanvas(this);
	_canvas->SetCurrent();

	GPlatesControls::View::Redisplay.SetFrame(_canvas);
		

	Fit();
	CentreOnScreen();
}

void
GLFrame::OnMouseMove(wxMouseEvent& evt)
{
	std::ostringstream oss;
	oss << "Current window coordinate of mouse: (" 
		<< evt.GetX() << ", " << evt.GetY() << ")";
	SetStatusText(wxString(oss.str().c_str(), *wxConvCurrent));
}

void
GLFrame::OnExit(wxCommandEvent&)
{
	Destroy();
	GPlatesControls::File::Quit(0);
}

void
GLFrame::OnOpenData(wxCommandEvent&)
{
	static wxString default_dir = _("");
	wxFileDialog filedlg(this, 
						 _("Select a data file..."),
						 default_dir,  // default dir  = none
						 _(""),  // default file = none
						 _("GPlates Data files (*.gpml)|*.gpml|"
						 "PLATES Data files (*.dat)|*.dat|"
						 "All files (*.*)|*.*"),  // wildcard
						 wxOPEN | wxFILE_MUST_EXIST);  // An 'Open' dialog box

	if (filedlg.ShowModal() == wxID_OK) {
		std::string selected_file;
		default_dir = filedlg.GetDirectory();
		selected_file = filedlg.GetPath().mb_str();
		GPlatesControls::File::OpenData(selected_file);
	}
}

void
GLFrame::OnOpenRotation(wxCommandEvent&)
{
	static wxString default_dir = _("");
	wxFileDialog filedlg(this, 
						 _("Select a rotation file..."),
						 default_dir,  // default dir  = none
						 _(""),  // default file = none
						 _("PLATES Rotation files (*.rot)|*.rot|"
						 "All files (*.*)|*.*"),  // wildcard
						 wxOPEN | wxFILE_MUST_EXIST);  // An 'Open' dialog box

	if (filedlg.ShowModal() == wxID_OK) {
		std::string selected_file;
		default_dir = filedlg.GetDirectory();
		selected_file = filedlg.GetPath().mb_str();
		GPlatesControls::File::OpenRotation(selected_file);
	}
}

void
GLFrame::OnViewMetadata(wxCommandEvent&)
{
//	std::cout << GPlatesControls::View::DocumentMetadata()
//		<< std::endl;
}

void
GLFrame::OnReconstructTime(wxCommandEvent&)
{	
	ReconstructTimeDialog dialog(this);

	if (dialog.ShowModal() == wxID_OK)
		GPlatesControls::Reconstruct::Time_asdf(dialog.GetInput());
}

void
GLFrame::OnReconstructPresent(wxCommandEvent&)
{
	GPlatesControls::Reconstruct::Present();
}

void
GLFrame::OnReconstructAnimation(wxCommandEvent&)
{
	AnimationTimesDialog dialog(this);

	if (dialog.ShowModal() == wxID_OK) {
		GPlatesControls::Reconstruct::Animation(
			dialog.GetStartTime(), 
			dialog.GetEndTime(),
			dialog.GetNSteps());
	}
}

BEGIN_EVENT_TABLE(GLFrame, wxFrame)
	EVT_CLOSE(GLFrame::OnExit)
	EVT_MOTION(GLFrame::OnMouseMove)

	EVT_MENU(MENU_FILE_OPENDATA, GLFrame::OnOpenData)
	EVT_MENU(MENU_FILE_OPENROTATION, GLFrame::OnOpenRotation)
	EVT_MENU(MENU_FILE_EXIT, GLFrame::OnExit)

	EVT_MENU(MENU_VIEW_METADATA, GLFrame::OnViewMetadata)
		
	EVT_MENU(MENU_RECONSTRUCT_TIME, GLFrame::OnReconstructTime)
	EVT_MENU(MENU_RECONSTRUCT_PRESENT, GLFrame::OnReconstructPresent)
	EVT_MENU(MENU_RECONSTRUCT_ANIMATION, GLFrame::OnReconstructAnimation)
END_EVENT_TABLE()
