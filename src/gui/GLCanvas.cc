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
#include "maths/UnitQuaternion3D.h"
#include "maths/FiniteRotation.h"
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

	_globe.Paint();

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


static const GLfloat ORTHO_RATIO = 1.2;

void
GLCanvas::SetView()
{
	static const GLfloat Z_NEAR = 0.5;

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
		glOrtho(-zoom_ratio, zoom_ratio, -factor, factor, Z_NEAR, eye_dist+3.0);
	}
	else
	{
		// height is limiting factor
		factor = zoom_ratio * fwidth / fheight;
		glOrtho(-factor, factor, -zoom_ratio, zoom_ratio, Z_NEAR, eye_dist+3.0);
	}
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

GPlatesMaths::PointOnSphere*
GLCanvas::GetSphereCoordFromScreen(int screenx, int screeny)
{
	using namespace GPlatesMaths;

	// the coordinate of the mouse projected onto the globe.
	real_t x, y, z, discrim;  

	int width, height;
	GetClientSize(&width, &height);

	// Minimum
	GLdouble scale = static_cast<GLdouble>(width < height ? width : height);
	
	// Scale to "unit square".
	y = 2.0*screenx - width;
	z = height - 2.0*screeny;
	y /= scale;
	z /= scale;
	
	// Account for the zoom factor
	y *= _zoom_factor*ORTHO_RATIO;
	z *= _zoom_factor*ORTHO_RATIO;

	// Test if point is within the sphere's horizon.
	if ((discrim = y*y + z*z) > 1.0)
	{
		return NULL;
	}

	x = sqrt(1.0 - discrim);

	// Transform the screen coord to be in the globe's coordinate system.
	UnitQuaternion3D elev = UnitQuaternion3D::CreateEulerRotation(
			UnitVector3D(0.0, 1.0, 0.0),  // Rotate around Y-axis
			degreesToRadians(-1.0*_globe.GetElevation()));
	
	UnitQuaternion3D merid = UnitQuaternion3D::CreateEulerRotation(
			UnitVector3D(0.0, 0.0, 1.0),  // Rotate around Z-axis
			degreesToRadians(-1.0*_globe.GetMeridian()));

	FiniteRotation rot = FiniteRotation::CreateFiniteRotation(
			merid * elev,
			0.0);  // present day
	
	UnitVector3D uv(x, y, z);
	uv = rot * uv;
	
	return new GPlatesMaths::PointOnSphere(uv);
}

void
GLCanvas::OnSpin(wxMouseEvent& evt)
{
	// XXX: Eek!  Non-reentrant!
	static GLfloat last_x = 0.0, last_y = 0.0, last_zoom = 0.0;
	// Make the tolerance inversely proportional to the current zoom.  
	// That way the globe won't spin stupidly when the user is up close.
	GLfloat TOLERANCE = 5.0/_zoom_factor;  
	static const GLfloat ZOOM_TOLERANCE = 200.0;

	GLfloat& meridian = _globe.GetMeridian();
	GLfloat& elevation = _globe.GetElevation();

	if (evt.LeftIsDown())
	{
		if (evt.Dragging())
		{
			GLfloat dir = (elevation > 90.0 && elevation < 270.0) ?
									-1 : 1;
			meridian  += dir * (evt.GetX() - last_x)/TOLERANCE;
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

			// Clamp the zoom factor.
			if (_zoom_factor > 1.0)
				_zoom_factor = 1.0;
			else if (_zoom_factor < 0.01)
				_zoom_factor = 0.01;

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
