/* $Id$ */

/**
 * @file 
 * Contains the parser implementation for the GPML.  
 * FIXME Makes use of some horrible pointer arithmetic.
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
#include "geo/PointData.h"
#include "geo/LineData.h"
#include "geo/DataGroup.h"

#include <map>
typedef std::map<std::string, std::string> GeneralData_t;


using namespace GPlatesFileIO;
using namespace GPlatesMaths;
using namespace GPlatesGeo;

typedef XMLParser::Element Element;
typedef Element::ElementList ElementList;

static const std::string GENERAL_DATA_ELEMENTS[] = {
	"attribution",
	"description",
	"datatype",
	"datatypeid",
	"regionid",
	"plateid",
	"ageofappearance",
	"ageofdisappearance",
	"colour",
	"color",
};


#if 0
// FIXME (Not being used yet anyway.)
static std::string
CompressWhitespace(const std::string& str)
{
	// istreams skip whitespace
	std::istringstream istr(str);
	std::string word, result;

	while (istr >> word) {
		result += word + ' ';
	}
	// XXX: result will have an additional space at the end.
	
	return result;
}

static void
GetGeneralData(const Element* element, GeneralData_t& map)
{
}
#endif

static PointOnSphere
GetPointOnSphere(const std::string& text)
{
	std::istringstream istr(text);
	
	real_t lat, lon;
	istr >> lat >> lon;
	// FIXME: check return value of istr.
	
	UnitVector3D uv = 
		OperationsOnSphere::convertLatLongToUnitVector(lat, lon);
	return PointOnSphere(uv);
}

static void
TrimWhitespace(std::string& str)
{
	// trim leading whitespace
	std::string::size_type  notwhite = str.find_first_not_of(" \t\n");
	str.erase(0,notwhite);

	// trim trailing whitespace
	notwhite = str.find_last_not_of(" \t\n"); 
	str.erase(notwhite+1); 
}

static PointOnSphere
GetCoord(const Element* element)
{
	std::string& text = *element->_content;
	TrimWhitespace(text);
	if (text.empty()) {
		std::cerr << "Error: Empty coord element." << std::endl;
		// FIXME: Dummy value; should allow the parser to skip
		// such values.
		return PointOnSphere(UnitVector3D(1.0, 0.0, 0.0));
	}

	return GetPointOnSphere(text);
}

static PointData
GetPointData(const Element* element)
{
	// FIXME doesn't check for <coord> element.
	return PointData(GeologicalData::NO_DATATYPE, GeologicalData::NO_ROTATIONGROUP, GeologicalData::NO_ATTRIBUTES,
		GetCoord(element));
}

static PolyLineOnSphere
GetCoordList(const Element* element)
{
	const ElementList* nodes = element->_children;
	PolyLineOnSphere coordlist;
	
	ElementList::const_iterator iter = nodes->begin();
	for ( ; iter != nodes->end(); ++iter) {
		if (*(*iter)->_name == "coord")
			coordlist.push_back(GetCoord(*iter));
	}
	return coordlist;
}

/**
 * XXX Uses only the first coordlist found.
 */
static LineData
GetLineData(const Element* element)
{
	const ElementList* list = element->_children;
	ElementList::const_iterator iter = list->begin();

	for ( ; iter != list->end(); ++iter)
		if (*(*iter)->_name == "coordlist")
			break;
	
	return LineData(GeologicalData::NO_DATATYPE, GeologicalData::NO_ROTATIONGROUP, GeologicalData::NO_ATTRIBUTES,
		GetCoordList(*iter));
}

static DataGroup
GetDataGroup(const Element* element)
{
	DataGroup datagroup(GeologicalData::NO_DATATYPE, GeologicalData::NO_ROTATIONGROUP, GeologicalData::NO_ATTRIBUTES);
	const ElementList* list = element->_children;
	ElementList::const_iterator iter = list->begin();

	for ( ; iter != list->end(); ++iter) {
		const std::string& name = *(*iter)->_name;
		// FIXME need to work out who owns the results of these calls
		if (name == "pointdata")
			datagroup.Add(new PointData(GetPointData(*iter)));
		else if (name == "linedata")
			datagroup.Add(new LineData(GetLineData(*iter)));
		else if (name == "datagroup")
			// Hooray for recursion.
			datagroup.Add(new DataGroup(GetDataGroup(*iter)));
	}
	return datagroup;
}

/** 
 * Handle meta data (title, and meta elements) in addition to
 * normal datagroup stuff.
 */
static DataGroup
GetRootDataGroup(const Element* element)
{
#if 0
	const NodeList 
		titlelist = element->get_children("title"),
		metalist  = element->get_children("meta");

	NodeList::const_iterator iter = nodelist.begin();
	for ( ; iter != nodelist.end(); ++iter) {

	}
#endif

	// Pass parameters on to general datagroup handler.
	DataGroup dg = GetDataGroup(element);

	// Add stuff from title and meta tags
	// FIXME
	
	return dg;
}

DataGroup
GPlatesReader::Read()
{
	XMLParser parser;
	const Element* root;

	root = parser.Parse(_istr);
	
	return (root ? 
		GetRootDataGroup(root) :
		// Parse failed, return empty DataGroup.
		DataGroup(GeologicalData::NO_DATATYPE, GeologicalData::NO_ROTATIONGROUP, GeologicalData::NO_ATTRIBUTES));
}
