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

#include "RenderVisitor.h"
#include "maths/PointOnSphere.h"
#include "OpenGL.h"

using namespace GPlatesGeo;
using namespace GPlatesGui;
using namespace GPlatesMaths;


static void
CallVertexWithPoint(const UnitVector3D& uv)
{
	glVertex3d(
		uv.x().dval(),
		uv.y().dval(),
		uv.z().dval()
	);
}

/**
 * FIXME doesn't display the last point in the line!
 */
static void
CallVertexWithLine(const PolyLineOnSphere::const_iterator& begin, 
				   const PolyLineOnSphere::const_iterator& end)
{
	PolyLineOnSphere::const_iterator iter = begin;

	glBegin(GL_LINE_STRIP);
		for ( ; iter != end; ++iter)
			CallVertexWithPoint(iter->startPoint());
	glEnd();
}

void
RenderVisitor::Visit(const PointData& point)
{
	glBegin(GL_POINTS);
		CallVertexWithPoint(point.GetPointOnSphere().unitvector());
	glEnd();
}

void
RenderVisitor::Visit(const LineData& line)
{
	glBegin(GL_LINE_STRIP);
		CallVertexWithLine(line.Begin(), line.End());
	glEnd();
}

void
RenderVisitor::Visit(const DataGroup& data)
{
	const GeologicalData* gd;
	const PointData* pd;
	const LineData*  ld;
	const DataGroup* dg;

	DataGroup::Children_t::const_iterator iter = data.ChildrenBegin();
	for ( ; iter != data.ChildrenEnd(); ++iter) {
		gd = *iter;
		pd = dynamic_cast<const PointData*>(gd);
		ld = dynamic_cast<const LineData*>(gd);
		dg = dynamic_cast<const DataGroup*>(gd);
		
		if (pd)
			Visit(*pd);
		else if (ld)
			Visit(*ld);
		else if (dg)
			Visit(*dg);
		else
			std::cerr << "Error: Child of DataGroup was not derived from "
				"GeologicalData!" << std::endl;
	}
}
