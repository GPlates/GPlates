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
	SetStatusText("Guten Abend", 0);
	SetStatusText("Dietmar", 1);

	_canvas = new GLCanvas(this, wxSize(640,640));
	_canvas->InitGL();

	Fit();
	CentreOnScreen();
}

BEGIN_EVENT_TABLE(GLFrame, wxFrame)
	EVT_CLOSE(GLFrame::OnExit)
END_EVENT_TABLE()
