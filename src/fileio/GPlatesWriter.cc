/* $Id$ */

/**
 * \file 
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

#include <string>
#include "GPlatesWriter.h"

using namespace GPlatesFileIO;
using namespace GPlatesGeo;

static const char *XML_HEADER = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>";

GPlatesWriter::GPlatesWriter()
	: _indent(0)
{
	// All XML files must have this line.
	_accum << XML_HEADER << std::endl;
}

#if 0
static std::string
Indent(int indent)
{
	return std::string(indent, '\t');
}

static void
WriteTimeWindow(std::ostream& os, const TimeWindow& tw)
{
}

static void
WritePlateID(std::ostream& os, const rid_t& id)
{
}

static void
WriteCoord(std::ostream& os, const PointOnSphere& point)
{
}

WriteCoordList(std::ostream& os, const PolyLineOnSphere& line)
{
}

static void
WritePointData(std::ostream& os, const PointData* data)
{
}

static void
WriteLineData(std::ostream& os, const LineData* data)
{
}

static void
WriteDataGroup(std::ostream& os, const DataGroup* data)
{
	static const char *DATAGROUP_START = "<datagroup>";
	static const char *DATAGROUP_END   = "</datagroup>";
	
	// XXX don't have access to _indent
	os << Indent(_indent) << DATAGROUP_START << std::endl;
	
	// Make sure children are indented.
	++_indent;

	

	os << Indent(_indent) << DATAGROUP_END << std::endl;
}
#endif

void
GPlatesWriter::Visit(const DataGroup* /*data*/)
{
//	WriteDataGroup(_accum, data);
}

void
GPlatesWriter::PrintOut(std::ostream& os)
{
	os << _accum.str();
}
