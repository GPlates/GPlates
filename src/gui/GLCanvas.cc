/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
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
 */

#include <iostream>
#include <cmath>  /* fabsf, pow */
#include "GLCanvas.h"
#include "EventIDs.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/UnitQuaternion3D.h"
#include "maths/FiniteRotation.h"
#include "maths/OperationsOnSphere.h"
#include "fileio/GPlatesReader.h"


/**
 * At the initial zoom, the smaller dimension of the GLCanvas will be
 * @a FRAMING_RATIO times the diameter of the Globe.  Obviously, when the
 * GLCanvas is resized, the Globe will be scaled accordingly.
 *
 * This is purely cosmetic.
 */
static const GLfloat FRAMING_RATIO = 1.07;


static GLfloat eyex = 0.0, eyey = 0.0, eyez = -5.0;


namespace {

	GPlatesMaths::real_t
	calcGlobePosDiscrim(const GPlatesMaths::real_t &y,
	                    const GPlatesMaths::real_t &z) {

		return (y * y + z * z);
	}


	bool
	isOnGlobe(const GPlatesMaths::real_t &discrim) {

		return (discrim <= 1.0);
	}


	GPlatesMaths::PointOnSphere
	onGlobe(const GPlatesMaths::real_t &y,
	        const GPlatesMaths::real_t &z,
	        const GPlatesMaths::real_t &discrim) {

		using namespace GPlatesMaths;

		// Assumes that (discrim >= 0 && discrim <= 1), and that
		// (y*y + z*z + discrim == 1)
		real_t x = sqrt(1.0 - discrim);

		return PointOnSphere(UnitVector3D(x, y, z));
	}


	GPlatesMaths::PointOnSphere
	atIntersectionWithGlobe(const GPlatesMaths::real_t &y,
	                        const GPlatesMaths::real_t &z,
	                        const GPlatesMaths::real_t &discrim) {

		using namespace GPlatesMaths;

		// Assumes that (discrim >= 1)
		real_t norm_reciprocal = 1.0 / sqrt(discrim);

		return PointOnSphere(UnitVector3D(0.0,
		                                  y * norm_reciprocal,
		                                  z * norm_reciprocal));
	}


	GPlatesMaths::PointOnSphere
	virtualGlobePosition(const GPlatesMaths::real_t &y,
	                     const GPlatesMaths::real_t &z) {

		GPlatesMaths::real_t discrim = calcGlobePosDiscrim(y, z);
		if (isOnGlobe(discrim)) {

			// The current mouse position is on the globe.
			return onGlobe(y, z, discrim);

		} else {

			// The current mouse position is not on the globe.
			// Interpolate back to the intersection.
			return atIntersectionWithGlobe(y, z, discrim);
		}
	}
}


GPlatesGui::GLCanvas::GLCanvas(MainWindow *parent,
 const wxSize &size, const wxPoint &position) :
 wxGLCanvas(parent, -1, position, size),
 _parent(parent),
 _zoom_power(1.0),  // Zoom power of 1.0 => no zoom
 _wheel_rotation(0),
 _is_initialised(false) {

	_popup_menu = CreatePopupMenu();
	if ( ! _popup_menu) {

		std::cerr << "Failed to create popup menu." << std::endl;
		std::exit(1);
	}

	RecalcZoom();

	_parent->Show(TRUE);
}


void 
GPlatesGui::GLCanvas::OnPaint(wxPaintEvent&) {

	wxPaintDC dc(this);

	if (!GetContext())
		return;

	if (!_is_initialised)
		InitGL();

	SetCurrent();

	ClearCanvas();
	glLoadIdentity();
	glTranslatef(eyex, eyey, eyez);

	/*
	 * Set up our universe coordinate system (standard mathematical one):
	 *   Z points up
	 *   Y points right
	 *   X points out of screen
	 */
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	glRotatef(-90.0, 0.0, 0.0, 1.0);

	_globe.Paint();

	SwapBuffers();
}

void
GPlatesGui::GLCanvas::OnSize(wxSizeEvent &evt) {

	wxGLCanvas::OnSize(evt);

	if (!GetContext())
		return;

	if (!_is_initialised)
		InitGL();

	SetCurrent();
	SetView();
}


#if 0
GPlatesMaths::PointOnSphere *
GPlatesGui::GLCanvas::GetSphereCoordFromScreen(int screenx, int screeny) {

	using namespace GPlatesMaths;

	// the coordinate of the mouse projected onto the globe.
	GPlatesMaths::real_t x, y, z, discrim;  

	int width, height;
	GetClientSize(&width, &height);

	// Minimum
	GLdouble scale = static_cast<GLdouble>(width < height ? width : height);
	
	// Scale to "unit square".
	y = 2.0 * screenx - width;
	z = height - 2.0 * screeny;
	y /= scale;
	z /= scale;
	
	// Account for the zoom factor
	y *= FRAMING_RATIO / _zoom_factor;
	z *= FRAMING_RATIO / _zoom_factor;

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

	FiniteRotation rot = FiniteRotation::Create(merid * elev,
	                      0.0);  // present day
	
	UnitVector3D uv(x, y, z);
	uv = rot * uv;
	
	return new GPlatesMaths::PointOnSphere(uv);
}

void
GPlatesGui::GLCanvas::OnSpin(wxMouseEvent& evt)
{
	using namespace GPlatesMaths;

	// XXX: Eek!  Non-reentrant!
	static GLfloat last_x = 0.0, last_y = 0.0, last_zoom = 0.0;
	// Make the tolerance inversely proportional to the current zoom.  
	// That way the globe won't spin stupidly when the user is up close.
	GLfloat TOLERANCE = 5.0 * _zoom_factor;  
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
		}
		last_zoom = evt.GetY();
	}
	else if (evt.GetWheelRotation() != 0)
	{
	}
	else if (evt.Moving())
	{
		// This is purely a motion event (no buttons depressed)
		PointOnSphere *pos =
		 GetSphereCoordFromScreen(evt.GetX(), evt.GetY());

		if (pos == NULL)
		{
			_parent->SetCurrentGlobePosOffGlobe();
		}
		else
		{
			LatLonPoint point =
			 OperationsOnSphere::convertPointOnSphereToLatLonPoint(
			  *pos);
			_parent->SetCurrentGlobePos(point.latitude().dval(),
			 point.longitude().dval());
			delete pos;
		}
	}

	// Pass the mouse event to the parent's process queue
	// GetParent()->GetEventHandler()->AddPendingEvent(evt);
}
#endif


void
GPlatesGui::GLCanvas::OnMouseEvent(wxMouseEvent &evt) {

	if (evt.RightDown()) {

		// The right mouse button was just pressed down.
		long x = evt.GetX();
		long y = evt.GetY();
		PopupMenu(_popup_menu, x, y);
		return;
	}

	if (evt.LeftDown()) {

		// The state of the left mouse button just changed to "down".
		// The effect of this is determined by the mode of operation.
		_mouse_x = evt.GetX();
		_mouse_y = evt.GetY();
		HandleLeftMouseEvent(MOUSE_EVENT_DOWN);
		HandleMouseMotion();
		Refresh();
		return;
	}

	if (evt.LeftIsDown()) {

		// Some event occurred with the left mouse button depressed.
		// The effect of this is determined by the mode of operation.
		_mouse_x = evt.GetX();
		_mouse_y = evt.GetY();
		HandleLeftMouseEvent(MOUSE_EVENT_DRAG);
		HandleMouseMotion();
		Refresh();
		return;
	}

	if (evt.GetWheelRotation() != 0) {

		// Some wheel rotation occurred.
		_wheel_rotation += evt.GetWheelRotation();
		HandleWheelRotation(evt.GetWheelDelta());
		return;
	}

	if (evt.Moving()) {

		// This is purely a motion event (no buttons depressed).
		_mouse_x = evt.GetX();
		_mouse_y = evt.GetY();
		HandleMouseMotion();
		return;
	}

	// else, pass this along to the next event handler
	evt.Skip();
}


void
GPlatesGui::GLCanvas::ZoomIn() {

	if (_zoom_power < 2.0) {

		IncrZoomPower();
		RecalcZoom();
	}
}


void
GPlatesGui::GLCanvas::ZoomOut() {

	if (_zoom_power > 1.0) {

		DecrZoomPower();
		RecalcZoom();
	}
}


void
GPlatesGui::GLCanvas::ZoomReset() {

	_zoom_power = 1.0;
	RecalcZoom();
}


void
GPlatesGui::GLCanvas::InitGL() {

	SetCurrent();

	// Enable depth buffering.
	glEnable(GL_DEPTH_TEST);
	// FIXME: enable polygon offset here or in Globe?

	ClearCanvas();
	_is_initialised = true;
}


void
GPlatesGui::GLCanvas::SetView() {

	static const GLdouble depth_near_clipping = 0.5;

	// Always fill up the all of the available space.
	GetDimensions();
	glViewport(0, 0, (GLsizei)_width, (GLsizei)_height);  
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// The coords of the symmetrical clipping planes which bound
	// the smaller dimension.
	GLdouble smaller_dim_clipping = FRAMING_RATIO / _zoom_factor;

	// The coords of the symmetrical clipping planes which bound
	// the larger dimension.
	GLdouble dim_ratio = _larger_dim / _smaller_dim;
	GLdouble larger_dim_clipping = smaller_dim_clipping * dim_ratio;

	// The coords of the further clipping plane in the depth dimension
	GLdouble depth_far_clipping = fabsf(eyez);

	if (_width <= _height) {

		// Width is the smaller dimension.
		glOrtho(-smaller_dim_clipping, smaller_dim_clipping,
		        -larger_dim_clipping, larger_dim_clipping,
		        depth_near_clipping, depth_far_clipping);

	} else {

		// Height is the smaller dimension.
		glOrtho(-larger_dim_clipping, larger_dim_clipping,
		        -smaller_dim_clipping, smaller_dim_clipping,
		        depth_near_clipping, depth_far_clipping);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


namespace {

	const double ZOOM_POWER_DELTA = 0.05;
}


void
GPlatesGui::GLCanvas::IncrZoomPower() {

	_zoom_power += ZOOM_POWER_DELTA;
}


void
GPlatesGui::GLCanvas::DecrZoomPower() {

	_zoom_power -= ZOOM_POWER_DELTA;
}


void
GPlatesGui::GLCanvas::RecalcZoom() {

	double zoom_percent = std::pow(10.0, _zoom_power + 1.0);
	unsigned zp_rounded = static_cast< unsigned >(zoom_percent + 0.5);
	_parent->SetCurrentZoom(zp_rounded);

	_zoom_factor = zoom_percent / 100.0;

	SetView();
	Refresh();
	HandleMouseMotion();
}


void
GPlatesGui::GLCanvas::GetDimensions() {

	GetClientSize(&_width, &_height);
	if (_width <= _height) {

		_smaller_dim = static_cast< GLdouble >(_width);
		_larger_dim = static_cast< GLdouble >(_height);

	} else {

		_smaller_dim = static_cast< GLdouble >(_height);
		_larger_dim = static_cast< GLdouble >(_width);
	}
}


void
GPlatesGui::GLCanvas::ClearCanvas(const Colour &c) {

	// Set colour buffer's clearing colour
	glClearColor(c.Red(), c.Green(), c.Blue(), c.Alpha());
	// Clear window to current clearing colour.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


GPlatesMaths::real_t
GPlatesGui::GLCanvas::getUniverseCoordY(int screen_x) {

	// Scale screen to "unit square".
	GPlatesMaths::real_t y = (2.0 * screen_x - _width) / _smaller_dim;

	return (y * FRAMING_RATIO / _zoom_factor);
}


GPlatesMaths::real_t
GPlatesGui::GLCanvas::getUniverseCoordZ(int screen_y) {

	// Scale screen to "unit square".
	GPlatesMaths::real_t z = (_height - 2.0 * screen_y) / _smaller_dim;

	return (z * FRAMING_RATIO / _zoom_factor);
}


wxMenu *
GPlatesGui::GLCanvas::CreatePopupMenu() {

	wxMenu *popupmenu = new wxMenu;

	popupmenu->Append(EventIDs::POPUP_SPIN_GLOBE, _("Spin Globe"));

	return popupmenu;
}


void
GPlatesGui::GLCanvas::HandleLeftMouseEvent(enum mouse_event_type type) {

	using namespace GPlatesMaths;

	real_t y = getUniverseCoordY(_mouse_x);
	real_t z = getUniverseCoordZ(_mouse_y);

	PointOnSphere p = virtualGlobePosition(y, z);
	if (type == MOUSE_EVENT_DOWN) {

		// The left mouse button was just clicked down.
		_globe.SetNewHandlePos(p);

	} else if (type == MOUSE_EVENT_DRAG) {

		// The pointer was dragged with the left mouse button down.
		_globe.UpdateHandlePos(p);
	}
}


void
GPlatesGui::GLCanvas::HandleWheelRotation(int delta) {

	if (delta == 0) {

		// There seems to be a bug in wxMouseEvent::GetWheelDelta
		delta = 120;
	}

	if (_wheel_rotation > 0) {

		while (_wheel_rotation >= delta) {

			_wheel_rotation -= delta;
			ZoomIn();
		}
		return;
	}
	if (_wheel_rotation < 0) {

		while (_wheel_rotation <= -delta) {

			_wheel_rotation += delta;
			ZoomOut();
		}
		return;
	}
}


void
GPlatesGui::GLCanvas::HandleMouseMotion() {

	using namespace GPlatesMaths;

	real_t y = getUniverseCoordY(_mouse_x);
	real_t z = getUniverseCoordZ(_mouse_y);

	real_t discrim = calcGlobePosDiscrim(y, z);
	if (isOnGlobe(discrim)) {

		// The current mouse position is on the globe.
		PointOnSphere p = onGlobe(y, z, discrim);

		// Compensate for rotated globe
		PointOnSphere rotated_p = _globe.Orient(p);

		LatLonPoint llp =
		 OperationsOnSphere::
		  convertPointOnSphereToLatLonPoint(rotated_p);

		_parent->SetCurrentGlobePos(llp.latitude().dval(),
		 llp.longitude().dval());

	} else {

		// The current mouse position is not on the globe.
		_parent->SetCurrentGlobePosOffGlobe();
	}
}


void
GPlatesGui::GLCanvas::OnSpinGlobe(wxCommandEvent &evt) {

}


BEGIN_EVENT_TABLE(GPlatesGui::GLCanvas, wxGLCanvas)

	EVT_SIZE(GPlatesGui::GLCanvas::OnSize)
	EVT_PAINT(GPlatesGui::GLCanvas::OnPaint)
	EVT_MOUSE_EVENTS(GPlatesGui::GLCanvas::OnMouseEvent)
	EVT_ERASE_BACKGROUND(GPlatesGui::GLCanvas::OnEraseBackground)

	EVT_MENU(EventIDs::POPUP_SPIN_GLOBE, GPlatesGui::GLCanvas::OnSpinGlobe)

END_EVENT_TABLE()
