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
typedef Element::ElementList ElementList;

/**
 * Extract the RotationGroupId from the given string.
 */
static GeologicalData::RotationGroupId_t
GetRotationGroupId(const Element*)
{
	return GeologicalData::NO_ROTATIONGROUP;
}

/**
 * Extract the DataType from the given string.
 */
static GeologicalData::DataType_t
GetDataType(const Element*)
{
	return GeologicalData::NO_DATATYPE;
}

static TimeWindow
GetTimeWindow(const Element*)
{
	return GeologicalData::NO_TIMEWINDOW;
}

static GeologicalData::Attributes_t
GetAttributes(const Element*)
{
	return GeologicalData::NO_ATTRIBUTES;
}

static LatLonPoint
GetLatLonPoint(const std::string& text)
{
	std::istringstream istr(text);
	
	real_t lat, lon;
	istr >> lat >> lon;
	// FIXME: check return value of istr.
	
	return LatLonPoint::CreateLatLonPoint(lat, lon);
}

static LatLonPoint
GetCoord(const Element* element)
{
	const std::string& text = element->_content;
	if (text.empty()) {
		std::cerr << "Error: Empty coord element." << std::endl;
		// FIXME: Dummy value; should allow the parser to skip
		// such values.
		return LatLonPoint::CreateLatLonPoint(0.0, 0.0);
	}

	return GetLatLonPoint(text);
}

static PointData
GetPointData(const Element* element)
{
	const ElementList& list = element->_children;
	ElementList::const_iterator iter = list.begin();

	for ( ; iter != list.end(); ++iter)
		if ((*iter)->_name == "coord")
			break;
	
	if (iter == list.end()) {
		std::cerr << "Empty coord." << std::endl;
		exit(-1);
	}
	return PointData(GetDataType(element), 
					 GetRotationGroupId(element), 
					 GetTimeWindow(element),
					 GetAttributes(element),
					 OperationsOnSphere::
					  convertLatLonPointToPointOnSphere(GetCoord(*iter)));
}

static PolyLineOnSphere
GetCoordList(const Element* element)
{
	const ElementList& nodes = element->_children;
	std::list<LatLonPoint> coordlist;
	
	ElementList::const_iterator iter = nodes.begin();
	for ( ; iter != nodes.end(); ++iter) {
		if ((*iter)->_name == "coord")
			coordlist.push_back(GetCoord(*iter));
	}
	return OperationsOnSphere::
			convertLatLonPointListToPolyLineOnSphere(coordlist);
}

/**
 * XXX Uses only the first coordlist found.
 */
static LineData
GetLineData(const Element* element)
{
	const ElementList& list = element->_children;
	ElementList::const_iterator iter = list.begin();

	for ( ; iter != list.end(); ++iter)
		if ((*iter)->_name == "coordlist")
			break;
	
	if (iter == list.end()) {
		std::cerr << "Empty coordlist." << std::endl;
		exit(-1);
	}

	return LineData(GetDataType(element), 
					GetRotationGroupId(element), 
					GetTimeWindow(element),
					GetAttributes(element),
					GetCoordList(*iter));
}

static DataGroup*
GetDataGroup(const Element* element)
{
	DataGroup* datagroup = 
		new DataGroup(GetDataType(element), 
					  GetRotationGroupId(element),
					  GetTimeWindow(element),
					  GetAttributes(element));
	const ElementList& list = element->_children;
	ElementList::const_iterator iter = list.begin();

	for ( ; iter != list.end(); ++iter) {
		const std::string& name = (*iter)->_name;
		// FIXME need to work out who owns the results of these calls
		if (name == "pointdata")
			datagroup->Add(new PointData(GetPointData(*iter)));
		else if (name == "linedata")
			datagroup->Add(new LineData(GetLineData(*iter)));
		else if (name == "datagroup")
			// Hooray for recursion.
			datagroup->Add(GetDataGroup(*iter));
	}
	return datagroup;
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
	XMLParser parser;
	const Element* root;

	// Create the pseudo-DOM hierarchy from the input.
	root = parser.Parse(_istr);
	
	if (root)
		return GetRootDataGroup(root);

	// !root => parse failed => return NULL.
	return static_cast<DataGroup*>(NULL);
}
