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

using namespace GPlatesGui;


/**
 * Menu IDs.
 */
enum {
	MENU_FILE_OPENDATA = 100,
	MENU_FILE_OPENROTATION,
	MENU_FILE_EXIT
};


static void
CreateMenuBar(GLFrame* frame)
{
	wxMenu* filemenu = new wxMenu;

	filemenu->Append(MENU_FILE_OPENDATA, 
					 "Open Data", 
					 "Open a data file");
	filemenu->Append(MENU_FILE_OPENROTATION, 
					 "Open Rotation", 
					 "Open a rotation file");
	filemenu->AppendSeparator();
	filemenu->Append(MENU_FILE_EXIT, "Exit", "Exit GPlates");

	wxMenuBar* menubar = new wxMenuBar(wxMB_DOCKABLE);
	menubar->Append(filemenu, "File");

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

	Fit();
	CentreOnScreen();
}

void
GLFrame::OnMouseMove(wxMouseEvent& evt)
{
	std::ostringstream oss;
	oss << "Current window coordinate of mouse: (" << evt.GetX() << ", " << evt.GetY() << ")";
	SetStatusText(wxString(oss.str().c_str()));
}

void
GLFrame::OnOpenData(wxCommandEvent& evt)
{
	std::cout << "Menu activated: File->Open Data" << std::endl;
}

void
GLFrame::OnOpenRotation(wxCommandEvent& evt)
{
	std::cout << "Menu activated: File->Open Rotation" << std::endl;
}


BEGIN_EVENT_TABLE(GLFrame, wxFrame)
	EVT_CLOSE(GLFrame::OnExit)
	EVT_MOTION(GLFrame::OnMouseMove)

	EVT_MENU(MENU_FILE_OPENDATA, GLFrame::OnOpenData)
	EVT_MENU(MENU_FILE_OPENROTATION, GLFrame::OnOpenRotation)
	EVT_MENU(MENU_FILE_EXIT, GLFrame::OnExit)
END_EVENT_TABLE()
