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
#include "maths/UnitVector3D.h"
#include "fileio/GPlatesReader.h"

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

	if (!_is_initialised)
		InitGL();

	SetCurrent();

	ClearCanvas();
	glLoadIdentity();
	glTranslatef(eyex, eyey, eyez);

	// Set up our coordinate system (standard mathematical one):
	//   Z points up
	//   Y points right
	//   X points out of screen
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	glRotatef(-90.0, 0.0, 0.0, 1.0);

	// XXX Read Datagroup from proper place
	GPlatesFileIO::GPlatesReader parser(std::cin);
	_globe.Paint(parser.Read());

	SwapBuffers();
}

void
GLCanvas::InitGL()
{
	SetCurrent();

	// Enable depth buffering.
	glEnable(GL_DEPTH_TEST);
	// FIXME: enable polygon offset here or in Globe?
	
	ClearCanvas();
	_is_initialised = true;
}

void
GLCanvas::OnSize(wxSizeEvent& evt)
{
	wxGLCanvas::OnSize(evt);

	if (!GetContext())
		return;

	if (!_is_initialised)
		InitGL();

	SetCurrent();
	SetView();
}


void
GLCanvas::SetView()
{
	static const GLfloat ORTHO_RATIO = 1.2;
	static const GLfloat Z_NEAR = 0.1;

	// Always fill up the all of the available space.
	int width, height;
	GetClientSize(&width, &height);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);  
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	GLfloat fwidth = static_cast<GLfloat>(width);
	GLfloat fheight = static_cast<GLfloat>(height);
	GLfloat eye_dist = static_cast<GLfloat>(fabsf(eyez));
	GLfloat zoom_ratio = _zoom_factor * ORTHO_RATIO;
	GLfloat factor;

	if (width <= height)
	{
		// Width is limiting factor
		factor = zoom_ratio * fheight / fwidth;
		glOrtho(-zoom_ratio, zoom_ratio, -factor, factor, Z_NEAR, eye_dist);
	}
	else
	{
		// height is limiting factor
		factor = zoom_ratio * fwidth / fheight;
		glOrtho(-factor, factor, -zoom_ratio, zoom_ratio, Z_NEAR, eye_dist);
	}
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

#if 0
/*
 * FIXME: There ought to be a more efficient way of doing this.
 */
static GPlatesMaths::DirVector3D
GetWorldCoordFromWindow(const GLdouble& winx, const GLdouble& winy)
{
	GLdouble modelmatrix[16], projmatrix[16], viewport[16];
	GLdouble objx, objy, objz;  // World coordinates.
	
	glGetDoublev(GL_MODELVIEW_MATRIX, &modelmatrix[0]);
	glGetDoublev(GL_PROJECTION_MATRIX, &projmatrix[0]);
	glGetDoublev(GL_VIEWPORT_MATRIX, &viewport[0]);
	
	gluUnProject(winx, winy, 0.0, modelmatrix, projmatrix, viewport, 
		&objx, &objy, &objz);
	
	return GPlatesMaths::DirVector3D(real_t(objx), real_t(objy), real_t(objz));
}
#endif

void
GLCanvas::OnSpin(wxMouseEvent& evt)
{
	// XXX: Eek!  Non-reentrant!
	static GLfloat last_x = 0.0, last_y = 0.0, last_zoom = 0.0;
	static const GLfloat TOLERANCE = 5.0, ZOOM_TOLERANCE = 200.0;

	GLfloat& meridian = _globe.GetMeridian();
	GLfloat& elevation = _globe.GetElevation();

	if (evt.LeftIsDown())
	{
		if (evt.Dragging())
		{
			meridian  += (evt.GetX() - last_x)/TOLERANCE;
			elevation += (evt.GetY() - last_y)/TOLERANCE;
			Refresh();  // Send a "Repaint" event.
		}
		last_x = evt.GetX();
		last_y = evt.GetY();
	}
	else if (evt.RightIsDown())
	{
		// Zoom.
		if (evt.Dragging())
		{
			_zoom_factor += (evt.GetY() - last_zoom)/ZOOM_TOLERANCE;

			SetView();
			Refresh();
		}
		last_zoom = evt.GetY();
	}

	// Pass the mouse event to the parent's process queue
	GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

BEGIN_EVENT_TABLE(GLCanvas, wxGLCanvas)
	EVT_SIZE(GLCanvas::OnSize)
	EVT_PAINT(GLCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(GLCanvas::OnEraseBackground)
//	EVT_LEFT_DCLICK(GLCanvas::OnReposition)
	EVT_MOUSE_EVENTS(GLCanvas::OnSpin)
END_EVENT_TABLE()
