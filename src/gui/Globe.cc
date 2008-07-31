/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "Globe.h"
#include "PlatesColourTable.h"
#include "NurbsRenderer.h"
#include "Texture.h"
#include "maths/Vector3D.h"
#include "state/Layout.h"


using namespace GPlatesMaths;
using namespace GPlatesState;


namespace 
{
	void
	CallVertexWithPoint(
			const PointOnSphere& p)
	{
		const UnitVector3D &uv = p.position_vector();
		glVertex3d(uv.x().dval(), uv.y().dval(), uv.z().dval());
	}


	void
	CallVertexWithLine(
			const PolylineOnSphere::const_iterator& begin, 
			const PolylineOnSphere::const_iterator& end,
			GPlatesGui::NurbsRenderer &nurbs_renderer)
	{
		// We will draw a NURBS if the two endpoints of the arc are
		// more than PI/36 radians (= 5 degrees) apart.
		static const double DISTANCE_THRESHOLD = std::cos(PI/36.0);

		PolylineOnSphere::const_iterator iter = begin;

		for ( ; iter != end; ++iter) {
			if (iter->dot_of_endpoints().dval() < DISTANCE_THRESHOLD) {
				nurbs_renderer.draw_great_circle_arc(*iter);
			} else {
				glBegin(GL_LINES);
					CallVertexWithPoint(iter->start_point());
					CallVertexWithPoint(iter->end_point());
				glEnd();
			}
		}
	}


	void
	PaintPointDataPos(Layout::PointDataPos& pointdata)
	{
		const PointOnSphere& point = *pointdata.first;
		
		glColor3fv(*pointdata.second);
		CallVertexWithPoint(point);
	}


	struct PaintLineDataPos {

		GPlatesGui::NurbsRenderer &d_nurbs_renderer;

		PaintLineDataPos(GPlatesGui::NurbsRenderer &nurbs_renderer) :
			d_nurbs_renderer(nurbs_renderer)
		{ }

		void
		operator()(Layout::LineDataPos& linedata)
		{
			const PolylineOnSphere& line = *linedata.first;
			glColor3fv(*linedata.second);
			
			glLineWidth(1.5f);
			CallVertexWithLine(line.begin(), line.end(), d_nurbs_renderer);
		}
	};


	void
	PaintPoints()
	{
		Layout::PointDataLayout::iterator 
			points_begin = Layout::PointDataLayoutBegin(),
			points_end   = Layout::PointDataLayoutEnd();

		glPointSize(4.0f);
		glBegin(GL_POINTS);
			for_each(points_begin, points_end, PaintPointDataPos);
		glEnd();
	}


	void
	PaintLines(GPlatesGui::NurbsRenderer &nurbs_renderer)
	{
		Layout::LineDataLayout::iterator 
			lines_begin = Layout::LineDataLayoutBegin(),
			lines_end   = Layout::LineDataLayoutEnd();
		
		for_each(lines_begin, lines_end, PaintLineDataPos(nurbs_renderer));
	}
}



void
GPlatesGui::Globe::SetNewHandlePos(const PointOnSphere &pos)
{
	m_globe_orientation.set_new_handle_at_pos(pos);
}


void
GPlatesGui::Globe::UpdateHandlePos(const PointOnSphere &pos)
{
	m_globe_orientation.move_handle_to_pos(pos);
}


PointOnSphere
GPlatesGui::Globe::Orient(const PointOnSphere &pos)
{
	return m_globe_orientation.reverse_orient_point(pos);
}


void
GPlatesGui::Globe::Paint()
{
	// Enable smoothing.
	glShadeModel(GL_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



	// NOTE: OpenGL rotations are *counter-clockwise* (API v1.4, p35).
	glPushMatrix();
		// rotate everything to get a nice almost-equatorial shot
//		glRotatef(-80.0, 1.0, 0.0, 0.0);

		UnitVector3D axis = m_globe_orientation.rotation_axis();
		real_t angle_in_deg =
		 radiansToDegrees(m_globe_orientation.rotation_angle());
		glRotatef(angle_in_deg.dval(),
		           axis.x().dval(), axis.y().dval(), axis.z().dval());
		
		/**
		 * Draw sphere.
		 * DepthRange calls push the sphere back in the depth buffer
		 * a bit to avoid Z-fighting with the LineData.
		 */
		glDepthRange(0.1, 1.0);
		_sphere.Paint();

		// Draw the texture slightly in front of the grey sphere, otherwise we 
		// get little bits of the sphere sticking out. 
		glDepthRange(0.05,1.0);
		d_texture.paint();
		
		/**
		 * Draw grid.
		 * DepthRange calls push the grid back in the depth buffer
		 * a bit to avoid Z-fighting with the LineData.
		 */
		glDepthRange(0.0, 0.9);

		_grid.Paint();
		
		// Restore DepthRange
		glDepthRange(0.0, 1.0);

		glPointSize(5.0f);
		
		/** 
		 * Paint the data.
		 */

		PaintPoints();
		
		PaintLines(d_nurbs_renderer);

	glPopMatrix();
}

void
GPlatesGui::Globe::paint_vector_output()
{

	_grid.paint_circumference(GPlatesGui::Colour::GREY);

	// NOTE: OpenGL rotations are *counter-clockwise* (API v1.4, p35).
	glPushMatrix();
		// rotate everything to get a nice almost-equatorial shot
//		glRotatef(-80.0, 1.0, 0.0, 0.0);

		UnitVector3D axis = m_globe_orientation.rotation_axis();
		real_t angle_in_deg =
		 radiansToDegrees(m_globe_orientation.rotation_angle());
		glRotatef(angle_in_deg.dval(),
		           axis.x().dval(), axis.y().dval(), axis.z().dval());

		// Set the grid's colour.
		glColor3fv(GPlatesGui::Colour::WHITE);
		
		/*
		 * Draw grid.
		 * DepthRange calls push the grid back in the depth buffer
		 * a bit to avoid Z-fighting with the LineData.
		 */
		glDepthRange(0.0, 0.9);
		_grid.Paint(Colour::GREY);

		// Restore DepthRange
		glDepthRange(0.0, 1.0);

		glPointSize(5.0f);
		
		/* 
		 * Paint the data.
		 */
		glColor3fv(GPlatesGui::Colour::GREEN);
		PaintPoints();
		
		glColor3fv(GPlatesGui::Colour::BLACK);
		PaintLines(d_nurbs_renderer);

	glPopMatrix();
}

void
GPlatesGui::Globe::initialise_texture()
{
//	d_texture.generate_test_texture2();
}

void
GPlatesGui::Globe::toggle_raster_image()
{
	d_texture.toggle();

}

void
GPlatesGui::Globe::enable_raster_display()
{
	d_texture.set_enabled(true);
}

void
GPlatesGui::Globe::disable_raster_display()
{
	d_texture.set_enabled(false);
}

