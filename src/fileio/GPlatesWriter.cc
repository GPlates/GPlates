/* $Id$ */

/**
 * @file 
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
#include <iomanip>
#include <functional>
#include <string>
#include "global/ControlFlowException.h"
#include "GPlatesWriter.h"
#include "geo/GeologicalData.h"
#include "geo/PointData.h"
#include "geo/LineData.h"
#include "geo/DataGroup.h"
#include "maths/PolyLineOnSphere.h"
#include "maths/LatLonPointConversions.h"

using namespace GPlatesFileIO;
using namespace GPlatesGeo;
using namespace GPlatesMaths;
using namespace GPlatesGlobal;

static const char *XML_HEADER = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>";


GPlatesWriter::GPlatesWriter()
: _indent(0)
{
	// XXX: Why 14?
	_accum.precision(14);

	// All XML files must have this line.
	_accum << XML_HEADER << std::endl;
}


namespace
{
	inline std::string
	Indent(int indent)
	{
		// Create a new string with indent copies of tab.
		return std::string(indent, '\t');
	}


	void
	WriteTimeWindow(std::ostream& os, const TimeWindow& tw, int indent)
	{
		os << Indent(indent)
			<< "<ageofappearance>" 
				<< tw.GetBeginning() 
			<< "</ageofappearance>" << std::endl
			<< Indent(indent)
			<< "<ageofdisappearance>"
				<< tw.GetEnd()
			<< "</ageofdisappearance>" << std::endl;
	}


	void
	WritePlateID(std::ostream& os, const rid_t& id, int indent)
	{
		os << Indent(indent)
			<< "<plateid>" 
				<< id.ival() 
			<< "</plateid>" << std::endl;
	}


	void
	WriteAttributes(std::ostream& os, 
					const GeologicalData& data, 
					int indent)
	{
		WritePlateID(os, data.GetRotationGroupId(), indent);
		WriteTimeWindow(os, data.GetTimeWindow(), indent);
	}


	void
	WriteCoord(std::ostream& os, const PointOnSphere& point, int indent)
	{
		LatLonPoint llp = LatLonPointConversions::
			convertPointOnSphereToLatLonPoint(point);

		os << Indent(indent)
			<< "<coord>" 
				<< llp.latitude() << " " << llp.longitude()
			<< "</coord>" << std::endl;
	}


	void
	WriteCoordList(std::ostream& os,
				   const PolyLineOnSphere::const_iterator& begin, 
				   const PolyLineOnSphere::const_iterator& end, 
				   int indent)
	{
		os << Indent(indent)
			<< "<coordlist>" << std::endl;

		PolyLineOnSphere::const_iterator iter = begin;
		WriteCoord(os, PointOnSphere(iter->start_point()), indent + 1);
		for ( ; iter != end; ++iter)
			WriteCoord(os, PointOnSphere(iter->end_point()), indent + 1);

		os << Indent(indent)
			<< "</coordlist>" << std::endl;
	}

}  // End anonymous namespace


void
GPlatesWriter::Visit(const PointData& data)
{
	_accum << Indent(_indent)
		<< "<pointdata>" << std::endl;

	// Print attributes.
	WriteAttributes(_accum, data, _indent + 1);
	
	// Print point
	WriteCoord(_accum, data.GetPointOnSphere(), _indent + 1);

	_accum << Indent(_indent)
		<< "</pointdata>" << std::endl;
}


void
GPlatesWriter::Visit(const LineData& data)
{
	_accum << Indent(_indent)
		<< "<linedata>" << std::endl;

	// Print attributes.
	WriteAttributes(_accum, data, _indent + 1);

	// Print PolyLine
	WriteCoordList(_accum, data.Begin(), data.End(), _indent + 1);
	
	_accum << Indent(_indent)
		<< "</linedata>" << std::endl;
}


void
GPlatesWriter::Visit(const DataGroup& data)
{
	_accum << Indent(_indent)
		<< "<datagroup>" << std::endl;
	
	// Indent children
	++_indent;
	
	// Print attributes.
	WriteAttributes(_accum, data, _indent);

	/*
	 * Call Accept(this) on each of the datagroup's children.
	 */
	DataGroup::Children_t::const_iterator iter = data.ChildrenBegin();
	for ( ; iter != data.ChildrenEnd(); ++iter)
		(*iter)->Accept(*this);
	
	--_indent;
	_accum << Indent(_indent)
		<< "</datagroup>" << std::endl;
}


void
GPlatesWriter::PrintOut(std::ostream& os)
{
	os << _accum.str();
}
