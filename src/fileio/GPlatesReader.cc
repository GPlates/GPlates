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

#include <map>
typedef std::map<std::string, std::string> GeneralData_t;


using namespace GPlatesFileIO;
using namespace GPlatesMaths;
using namespace GPlatesGeo;
using XMLParser::Element;
using Element::ElementList;

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

static PointOnSphere
GetPointOnSphere(const std::string* text)
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
TrimWhitespace(string& str)
{
	// trim leading whitespace
	string::size_type  notwhite = str.find_first_not_of(" \t\n");
	str.erase(0,notwhite);

	// trim trailing whitespace
	notwhite = str.find_last_not_of(" \t\n"); 
	str.erase(notwhite+1); 
}

static PointOnSphere
GetCoord(const Element* element)
{
	const std::string& text = element->_content;
	TrimWhitespace(text);
	if (text.empty()) {
		std::cerr << "Line " << text->get_line()
			<< ": Empty coord element." << std::endl;
		// FIXME: Dummy value; should allow the parser to skip
		// such values.
		return PointOnSphere(UnitVector3D(1.0, 0.0, 0.0));
	}

	return GetPointOnSphere(text);
}

static PointData
GetPointData(const Element* element)
{
	return PointData(NO_DATATYPE, NO_ROTATIONGROUP, NO_ATTRIBUTES,
		GetPointOnSphere(element));
}

static PolyLineOnSphere
GetCoordList(const Element* element)
{
	const ElementList* nodes = element->_children;
	PolyLineOnSphere coordlist;
	
	ElementList::const_iterator iter = nodes->begin();
	for ( ; iter != nodes->end(); ++iter) {
		if (iter->_name == "coord")
			coordlist.push_back(GetPointOnSphere(*iter));
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
		if (iter->_name == "coordlist")
			break;
	
	return LineData(NO_DATATYPE, NO_ROTATIONGROUP, NO_ATTRIBUTES,
		GetCoordList(*iter));
}

static DataGroup
GetDataGroup(const Element* element)
{
	DataGroup datagroup(NO_DATATYPE, NO_ROTATIONGROUP, NO_ATTRIBUTES);
	const ElementList* list = element->_children;
	ElementList::const_iterator iter = list->begin();

	for ( ; iter != list->end(); ++iter) {
		if (iter->_name == "pointdata")
			datagroup.Add(GetPointData(*iter));
		else if (iter->_name == "linedata")
			datagroup.Add(GetLineData(*iter));
		else if (iter->_name == "datagroup")
			// Hooray for recursion.
			datagroup.Add(GetDataGroup(*iter));
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
	DataGroup& dg = GetDataGroup(element);

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
		DataGroup(NO_DATATYPE, NO_ROTATIONGROUP, NO_ATTRIBUTES));
}
