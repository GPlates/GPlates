/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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
#include <algorithm>

#include "Globe.h"
#include "PlatesColourTable.h"
#include "state/Layout.h"


using namespace GPlatesMaths;
using namespace GPlatesState;


static void
CallVertexWithPoint(const PointOnSphere& p)
{
	const UnitVector3D &uv = p.position_vector();
	glVertex3d(uv.x().dval(), uv.y().dval(), uv.z().dval());
}


static void
CallVertexWithLine(const PolylineOnSphere::const_iterator& begin, 
                   const PolylineOnSphere::const_iterator& end)
{
	PolylineOnSphere::const_iterator iter = begin;

	glBegin(GL_LINE_STRIP);
		CallVertexWithPoint(iter->start_point());
		for ( ; iter != end; ++iter)
			CallVertexWithPoint(iter->end_point());
	glEnd();
}


static void
PaintPointDataPos(Layout::PointDataPos& pointdata)
{
	GPlatesGeo::PointData *datum = pointdata.first;
	if ( ! datum->ShouldBePainted()) return;
	PointOnSphere& point = pointdata.second;
	CallVertexWithPoint(point);
}


static void
PaintLineDataPos(Layout::LineDataPos& linedata)
{
	using namespace GPlatesGui;

	GPlatesGeo::LineData *datum = linedata.first;
	if ( ! datum->ShouldBePainted()) return;

	const PlatesColourTable &ctab = *(PlatesColourTable::Instance());
	const PolylineOnSphere& line = linedata.second;

	GPlatesGlobal::rid_t rgid = datum->GetRotationGroupId();
	PlatesColourTable::const_iterator it = ctab.lookup(rgid);
	if (it != ctab.end()) {

		// There is an entry for this RG-ID in the colour table.
		glColor3fv(*it);

	} else glColor3fv(GPlatesGui::Colour::RED);
	CallVertexWithLine(line.begin(), line.end());
}


static void
PaintPoints()
{
	Layout::PointDataLayout::iterator 
		points_begin = Layout::PointDataLayoutBegin(),
		points_end   = Layout::PointDataLayoutEnd();

	glBegin(GL_POINTS);
		for_each(points_begin, points_end, PaintPointDataPos);
	glEnd();
}


static void
PaintLines()
{
	Layout::LineDataLayout::iterator 
		lines_begin = Layout::LineDataLayoutBegin(),
		lines_end   = Layout::LineDataLayoutEnd();
	
	for_each(lines_begin, lines_end, PaintLineDataPos);
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
	// NOTE: OpenGL rotations are *counter-clockwise* (API v1.4, p35).
	glPushMatrix();
		// rotate everything to get a nice almost-equatorial shot
//		glRotatef(-80.0, 1.0, 0.0, 0.0);

		UnitVector3D axis = m_globe_orientation.rotation_axis();
		real_t angle_in_deg =
		 radiansToDegrees(m_globe_orientation.rotation_angle());
		glRotatef(angle_in_deg.dval(),
		           axis.x().dval(), axis.y().dval(), axis.z().dval());

		// Set the sphere's colour.
		glColor3fv(GPlatesGui::Colour(0.35, 0.35, 0.35));
		
		/*
		 * Draw sphere.
		 * DepthRange calls push the sphere back in the depth buffer
		 * a bit to avoid Z-fighting with the LineData.
		 */
		glDepthRange(0.1, 1.0);
		_sphere.Paint();

		// Set the grid's colour.
		glColor3fv(Colour::WHITE);
		
		/*
		 * Draw grid.
		 * DepthRange calls push the grid back in the depth buffer
		 * a bit to avoid Z-fighting with the LineData.
		 */
		glDepthRange(0.0, 0.9);
		_grid.Paint();

		// Restore DepthRange
		glDepthRange(0.0, 1.0);

		glPointSize(5.0f);
		
		/* 
		 * Paint the data.
		 */
		glColor3fv(GPlatesGui::Colour::GREEN);
		PaintPoints();
		
		glColor3fv(GPlatesGui::Colour::BLACK);
		PaintLines();

	glPopMatrix();
}
