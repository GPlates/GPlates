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
#include "OpenGL.h"
#include "GLFrame.h"

using namespace GPlatesGui;

GLFrame::GLFrame(wxFrame* parent, const wxString& title,
	const wxSize& size, const wxPoint& pos)
 : wxFrame(parent, -1 /* XXX: DEFAULT_WINDOWID */, title, pos, size)
{
	_status_bar = CreateStatusBar(STATUSBAR_NUM_FIELDS);
	if (!_status_bar) {
		std::cerr << "Failed to create status bar." << std::endl;
		exit(1);
	}

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

BEGIN_EVENT_TABLE(GLFrame, wxFrame)
	EVT_CLOSE(GLFrame::OnExit)
	// FIXME: This isn't being notified of mouse movement.  Perhaps all the
	// mouse events are being captured by GLCanvas?
	EVT_MOTION(GLFrame::OnMouseMove)
END_EVENT_TABLE()
