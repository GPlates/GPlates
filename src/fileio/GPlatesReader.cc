/* $Id$ */

/**
 * @file 
 * Contains the parser implementation for the GPML.  
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


#include <sstream>
#include <iterator>  /* for inserters */

#include "GPlatesReader.h"
#include "XMLParser.h"
#include "FileFormatException.h"
#include "global/types.h"
#include "maths/types.h"
#include "maths/OperationsOnSphere.h"
#include "maths/PointOnSphere.h"
#include "geo/GeologicalData.h"
#include "geo/PointData.h"
#include "geo/LineData.h"
#include "geo/DataGroup.h"
#include "geo/TimeWindow.h"
#include "geo/StringValue.h"
#include "geo/LiteralStringValue.h"
#include "geo/DerivedStringValue.h"


using namespace GPlatesFileIO;
using namespace GPlatesMaths;
using namespace GPlatesGeo;

typedef XMLParser::Element Element;
typedef Element::ElementList_type ElementList;

static void
ReadError(const char* was_reading, unsigned int line)
{
	std::ostringstream oss;
	oss << "Error when reading " << was_reading << " (line " 
		<< line << ")." << std::endl;
	throw FileFormatException(oss.str().c_str());
}

static void
InvalidDataError(const char* datatype, const char* got,
		const char* wanted, unsigned int line)
{
	std::ostringstream oss;
	oss << "Invald " << datatype << " data encountered on line "
		<< line << "." << std::endl
		<< "Got: " << got << std::endl
		<< "Wanted: " << wanted << std::endl;
	throw FileFormatException(oss.str().c_str());
}

static void
MultipleDefinitionError(const char* of_elem, const char* in_elem, 
	const ElementList& list, unsigned int line)
{
	std::ostringstream oss;
	oss << "Mulitple <" << of_elem << "> elements defined in element "
		<< in_elem << " (line " << line << ")." << std::endl
		<< "Offending data: " << std::endl;
	
	ElementList::const_iterator iter = list.begin();
	for ( ; iter != list.end(); ++iter)
	{
		oss << "\t->  " << (*iter)->GetContent() << " (line "
			<< (*iter)->GetLineNumber() << ")" << std::endl;
	}
	
	throw FileFormatException(oss.str().c_str());
}

template < typename T >
static T
ReadUnique(const Element* element, const char* to_read, 
	const T& default_value)
{
	const ElementList& nodes = element->GetChildren(to_read);
	if (nodes.size() > 1)
	{
		MultipleDefinitionError(to_read, element->GetName().c_str(),
			nodes, element->GetLineNumber());
	}
	else if (nodes.empty())
	{
		// No data was defined, so return the default
		return default_value;
	}
	
	const Element* node = *nodes.begin();

	std::istringstream iss(node->GetContent());
	T data;

	if ( ! (iss >> data))
		ReadError(to_read, node->GetLineNumber());
	
	return data;
}

/**
 * Extract the RotationGroupId from the given string.
 */
static GPlatesGlobal::rid_t
GetRotationGroupId(const Element* element)
{
	// FIXME: When tables are implemented, should check that the
	// return value is a valid plate.
	return rid_t(ReadUnique< unsigned int >(element, "plateid",
		1000000));
}

/**
 * Extract the DataType from the given string.
 */
static GeologicalData::DataType_t
GetDataType(const Element* element)
{
	return ReadUnique< GeologicalData::DataType_t >(element, "datatype",
		GeologicalData::NO_DATATYPE);
}

static TimeWindow
GetTimeWindow(const Element* element)
{
	GPlatesGlobal::fpdata_t appearance, disappearance;

	appearance = ReadUnique< GPlatesGlobal::fpdata_t >(element, 
		"appearance", 0.0);

	disappearance = ReadUnique< GPlatesGlobal::fpdata_t >(element, 
		"disappearance", 0.0);
	
	return TimeWindow(appearance, disappearance);
}

static GeologicalData::Attributes_t
GetAttributes(const Element*)
{
	return GeologicalData::NO_ATTRIBUTES;
}

static LatLonPoint
GetLatLonPoint(const std::string& text, unsigned int line)
{
	std::istringstream istr(text);
	
	real_t lat, lon;
	if ( ! (istr >> lat))
		ReadError("latitude", line);
	
	if ( ! (istr >> lon))
		ReadError("longitude", line);
	
	if ( ! LatLonPoint::isValidLat(lat))
	{
		InvalidDataError("latitude", text.c_str(), 
			"in range [-90.0, 90.0]", line);
	}
						
	if ( ! LatLonPoint::isValidLon(lon))
	{
		InvalidDataError("longitude", text.c_str(),
			"in range (-180.0, 180.0]", line);
	}
	
	return LatLonPoint::CreateLatLonPoint(lat, lon);
}

static LatLonPoint
GetCoord(const Element* element)
{
	return GetLatLonPoint(element->GetContent(), element->GetLineNumber());
}

static PointData*
GetPointData(const Element* element)
{
	const ElementList& list = element->GetChildren("coord");
	
	if (list.empty())
	{
		std::ostringstream oss;
		oss << "No coord element found in <pointdata> at line "
			<< element->GetLineNumber() << "." << std::endl;
		throw FileFormatException(oss.str().c_str());
	}
	return new PointData(GetDataType(element), 
					 GetRotationGroupId(element), 
					 GetTimeWindow(element),
					 GetAttributes(element),
					 OperationsOnSphere::
					  convertLatLonPointToPointOnSphere(GetCoord(*list.begin())));
}

static PolyLineOnSphere
GetCoordList(const Element* element)
{
	const ElementList& nodes = element->GetChildren("coord");
	std::list<LatLonPoint> coordlist;
	
	if (nodes.size() < 2)
	{
		std::ostringstream oss;
		oss << nodes.size() << " <coord>s";
		InvalidDataError("coordlist", oss.str().c_str(),
			"2 or more <coords>", element->GetLineNumber());
	}
	
	std::transform(nodes.begin(), nodes.end(),
		std::back_inserter(coordlist), &GetCoord);

	return OperationsOnSphere::
			convertLatLonPointListToPolyLineOnSphere(coordlist);
}

static LineData*
GetLineData(const Element* element)
{
	const ElementList& list = element->GetChildren("coordlist");

	if (list.empty())
	{
		std::ostringstream oss;
		oss << "No coordlist element found in <linedata> at line "
			<< element->GetLineNumber() << "." << std::endl;
		throw FileFormatException(oss.str().c_str());
	}

	return new LineData(GetDataType(element), 
					GetRotationGroupId(element), 
					GetTimeWindow(element),
					GetAttributes(element),
					GetCoordList(*list.begin()));
}

static DataGroup*
GetDataGroup(const Element* element)
{
	DataGroup::Children_t children;

	const ElementList& points = element->GetChildren("pointdata");
	std::transform(points.begin(), points.end(),
		std::back_inserter(children),
		&GetPointData);
	
	const ElementList& lines = element->GetChildren("linedata");
	std::transform(lines.begin(), lines.end(),
		std::back_inserter(children),
		&GetLineData);
	
	const ElementList& dgs = element->GetChildren("datagroup");
	std::transform(dgs.begin(), dgs.end(),
		std::back_inserter(children),
		&GetDataGroup); // Hooray for recursion.
	
	return new DataGroup(GetDataType(element), 
					  GetRotationGroupId(element),
					  GetTimeWindow(element),
					  GetAttributes(element),
					  children);
}

/** 
 * Handle meta data (title, and meta elements) in addition to
 * normal datagroup stuff.
 */
static DataGroup*
GetRootDataGroup(const Element* element)
{
	// Pass parameters on to general datagroup handler.
	DataGroup* dg = GetDataGroup(element);

	// Add stuff from title and meta tags
	// FIXME
	
	return dg;
}

DataGroup*
GPlatesReader::Read()
{
	Element* root = NULL;
	DataGroup* datagroup = NULL;

	try
	{
		// Create the pseudo-DOM hierarchy from the input.
		root = XMLParser::Parse(_istr);

		// Transform the hierarchy into our internal format.
		datagroup = GetRootDataGroup(root);
	} 
	catch (const FileFormatException&)
	{
		// Cleanup...
		if (root)
			delete root;
		if (datagroup)
			delete datagroup;

		// ...and rethrow
		throw;
	}

	return datagroup;	
}
