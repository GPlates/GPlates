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
 */

#include <cmath>  /* fabsf() */
#include "GLCanvas.h"
#include "Colour.h"

using namespace GPlatesGui;

static void
ClearCanvas(const Colour& c = Colour::BLACK)
{
	// Set colour buffer's clearing colour
	glClearColor(c.Red(), c.Green(), c.Blue(), c.Alpha());
	// Clear window to current clearing colour.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static GLfloat eyex = 0.0, eyey = 0.0, eyez = -5.0;

void 
GLCanvas::OnPaint(wxPaintEvent&)
{
	wxPaintDC dc(this);
	if (!GetContext())
		return;
	SetCurrent();
	wxSizeEvent evt(GetSize());
	OnSize(evt);
	
	ClearCanvas();
	glLoadIdentity();
	glTranslatef(eyex, eyey, eyez);
	_globe.Paint();

	SwapBuffers();
}

void
GLCanvas::InitGL()
{
	ClearCanvas();
	// Enable depth buffering.
	glEnable(GL_DEPTH_TEST);
	// FIXME: enable polygon offset here or in Globe?
	
}

static GLfloat ORTHO_RATIO = 1.2;

void
GLCanvas::OnSize(wxSizeEvent& evt)
{
	wxGLCanvas::OnSize(evt);
	int width, height;

	GetClientSize(&width, &height);
	if (GetContext()) {
		SetCurrent();
		glViewport(0, 0, static_cast<GLsizei>(width),
			static_cast<GLsizei>(height));
	}
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	GLfloat fwidth = static_cast<GLfloat>(width);
	GLfloat fheight = static_cast<GLfloat>(height);
	GLfloat eye_dist = static_cast<GLfloat>(fabsf(eyez));
	GLfloat aspect = (width <= height) ? fheight/fwidth : fwidth/fheight;
	
	glOrtho(-ORTHO_RATIO, ORTHO_RATIO, -ORTHO_RATIO*aspect, 
	 ORTHO_RATIO*aspect, 0.1, eye_dist);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

BEGIN_EVENT_TABLE(GLCanvas, wxGLCanvas)
	EVT_SIZE(GLCanvas::OnSize)
	EVT_PAINT(GLCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(GLCanvas::OnEraseBackground)
END_EVENT_TABLE()
