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

static void
CallVertexWithPoint(const GPlatesMaths::PointOnSphere& point)
{
	GLdouble x, y, z;
	const GPlatesMaths::UnitVector3D& uv = point.unitvector();

	x = uv.x().dval();
	y = uv.y().dval();
	z = uv.z().dval();

	glVertex3d(x, y, z);
}

void
RenderVisitor::Visit(const PointData& point)
{
	glBegin(GL_POINTS);
		CallVertexWithPoint(point.GetPointOnSphere());
	glEnd();
}

void
RenderVisitor::Visit(const LineData& line)
{
	GPlatesMaths::PolyLineOnSphere::const_iterator iter = line.Begin();

	glBegin(GL_LINE_STRIP);
		for ( ; iter != line.End(); ++iter)
			CallVertexWithPoint(*iter);
	glEnd();
}

void
RenderVisitor::Visit(const DataGroup& data)
{
	DataGroup::Children_t::const_iterator iter = data.ChildrenBegin();
	for ( ; iter != data.ChildrenEnd(); ++iter) {
		const PointData* pd = dynamic_cast<const PointData*>(*iter);
		const LineData*  ld = dynamic_cast<const LineData*>(*iter);
		const DataGroup* dg = dynamic_cast<const DataGroup*>(*iter);
		
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
