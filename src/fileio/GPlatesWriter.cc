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

#include <iostream>
#include <string>
#include "global/ControlFlowException.h"
#include "GPlatesWriter.h"
#include "geo/GeologicalData.h"
#include "geo/PointData.h"
#include "geo/LineData.h"
#include "geo/DataGroup.h"
#include "maths/PolyLineOnSphere.h"
#include "maths/OperationsOnSphere.h"

using namespace GPlatesFileIO;
using namespace GPlatesGeo;
using namespace GPlatesMaths;
using namespace GPlatesGlobal;

static const char *XML_HEADER = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>";

GPlatesWriter::GPlatesWriter()
{
	// All XML files must have this line.
	_accum << XML_HEADER << std::endl;
}

static void
WriteTimeWindow(std::ostream& os, const TimeWindow& tw, std::string indent)
{
	os << indent 
		<< "<ageofappearance>" 
			<< tw.GetBeginning() 
		<< "</ageofappearance>" << std::endl
		<< indent
		<< "<ageofdisappearance>"
			<< tw.GetEnd()
		<< "</ageofdisappearance>" << std::endl;
}

static void
WritePlateID(std::ostream& os, const rid_t& id, std::string indent)
{
	os << indent 
		<< "<plateid>" 
			<< id.ival() 
		<< "</plateid>" << std::endl;
}

static void
WriteAttributes(std::ostream& os, 
				const GeologicalData* data, 
				std::string indent)
{
	WritePlateID(os, data->GetRotationGroupId(), indent);
	WriteTimeWindow(os, data->GetTimeWindow(), indent);
}


static void
WriteCoord(std::ostream& os, const PointOnSphere& point, std::string indent)
{
	// convertPointOnSphereToLatLonPoint(point)
	LatLonPoint llp = OperationsOnSphere::
		convertPointOnSphereToLatLonPoint(point);

	os << indent
		<< "<coord>" 
			<< llp.latitude() << " " << llp.longitude()
		<< "</coord>" << std::endl;
}

static void
WriteCoordList(std::ostream& os,
			   const PolyLineOnSphere::const_iterator& begin, 
			   const PolyLineOnSphere::const_iterator& end, 
			   std::string indent)
{
	os << indent
		<< "<coordlist>" << std::endl;

	PolyLineOnSphere::const_iterator iter = begin;
	WriteCoord(os, PointOnSphere(iter->startPoint()), indent + '\t');
	for ( ; iter != end; ++iter)
		WriteCoord(os, PointOnSphere(iter->endPoint()), indent + '\t');

	os << indent
		<< "</coordlist>" << std::endl;
}

static void
WritePointData(std::ostream& os, const PointData* data, std::string indent)
{
	os << indent
		<< "<pointdata>" << std::endl;

	// Print attributes.
	WriteAttributes(os, data, indent + '\t');
	
	// Print point
	WriteCoord(os, data->GetPointOnSphere(), indent + '\t');

	os << indent
		<< "</pointdata>" << std::endl;
}

static void
WriteLineData(std::ostream& os, const LineData* data, std::string indent)
{
	os << indent
		<< "<linedata>" << std::endl;

	// Print attributes.
	WriteAttributes(os, data, indent + '\t');

	// Print PolyLine
	WriteCoordList(os, data->Begin(), data->End(), indent + '\t');
	
	os << indent
		<< "</linedata>" << std::endl;
}

static void
WriteDataGroup(std::ostream& os, const DataGroup* data, std::string indent)
{
	os << indent 
		<< "<datagroup>" << std::endl;
	
	// Print attributes.
	WriteAttributes(os, data, indent + '\t');

	DataGroup::Children_t::const_iterator iter = data->ChildrenBegin();
	for ( ; iter != data->ChildrenEnd(); ++iter) {
		const LineData* ld;
		const PointData* pd;
		const DataGroup* dg;

		if ((ld = dynamic_cast< const LineData* >(*iter)))
			WriteLineData(os, ld, indent + '\t');
		else if ((pd = dynamic_cast< const PointData* >(*iter)))
			WritePointData(os, pd, indent + '\t');
		else if ((dg = dynamic_cast< const DataGroup* >(*iter)))
			WriteDataGroup(os, dg, indent + '\t');
		else {
			std::ostringstream oss;
			oss << "Encountered a DataGroup member that would not "
				"cast to one of LineData, PointData or DataGroup.";
			throw ControlFlowException(oss.str().c_str());
		}
	}

	os << indent 
		<< "</datagroup>" << std::endl;
}

void
GPlatesWriter::Visit(const DataGroup* data)
{
	/**
	 * The current level of indentation.  Used to make the output
	 * a bit more readable.
	 */
	std::string indent;

//	_accum.precision(10);
//	_accum.setf(std::ios_base::showpoint);
	
	WriteDataGroup(_accum, data, indent);
}

void
GPlatesWriter::PrintOut(std::ostream& os)
{
	os << _accum.str();
}
