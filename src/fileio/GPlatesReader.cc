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


#include <libxml++/libxml++.h>
#include <sstream>
#include <iterator>  /* for inserters */

#include "GPlatesReader.h"
#include "maths/types.h"
#include "maths/OperationsOnSphere.h"
#include "maths/PointOnSphere.h"

#include <map>
typedef std::map<std::string, std::string> GeneralData_t;


using namespace GPlatesFileIO;
using namespace GPlatesMaths;
using namespace GPlatesGeo;
using namespace xmlpp;

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
GetPointOnSphere(const ContentNode* text)
{
	std::istringstream istr(text->get_content());
	
	real_t lat, lon;
	istr >> lat >> lon;
	// FIXME: check return value of istr.
	
	return PointOnSphere(convertLatLongToUnitVector(lat, lon));
}

static PointOnSphere
GetCoord(const Element* element)
{
	const ContentNode* text = element->get_child_content();
	if (text->is_white_space()) {
		std::cerr << "Line " << text->get_line()
			<< ": Empty coord element." << std::endl;
		// FIXME: Dummy value, should allow the parser to skip
		// such values.
		return PointOnSphere;
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
	const NodeList nodes = element->get_children("coord");
	PolyLineOnSphere coordlist;
	
	NodeList::const_iterator iter = nodes.begin();
	for ( ; iter != nodes.end(); ++iter) {
		const Element* el = dynamic_cast<Element*>(*iter);
		if (!el) {
			std::cerr << "CRITICAL: received a node which is not an element" 
				<< std::endl
			exit(-1);
		}
		coordlist.push_back(GetPointOnSphere(el));
	}
	return coordlist;
}

static LineData
GetLineData(const Element* element)
{
	const NodeList nodes = element->get_children("coordlist");
	
	if (nodes.length() > 1) {
		std::cerr << "Warning: multiple coordlist elements specified inside"
			" a <linedata> element.  Using first of these." << std::endl;
	}

	const Element* element = dynamic_cast<Element*>(*nodes.begin());
	if (!element) {
		std::cerr << "CRITICAL: received a node which is not an element" 
			<< std::endl
		exit(-1);
	}
	return LineData(NO_DATATYPE, NO_ROTATIONGROUP, NO_ATTRIBUTES,
		GetCoordList(element));
}

static void
GetDataGroup(const Element* element, DataGroup& datagroup)
{
	const NodeList 
		pointlist = element->get_children("pointdata"),
		linelist  = element->get_children("linedata"),
		grouplist = element->get_children("datagroup");
	
	// FIXME AAARGH!  Horrible repetition of code!
	NodeList::const_iterator iter = pointlist.begin();
	for ( ; iter != pointlist.end(); ++iter) {
		const Element* el = dynamic_cast<Element*>(*iter);
		if (!el) {
			std::cerr << "CRITICAL: received a node which is not an element" 
				<< std::endl;
			exit(-1);
		}
		datagroup.Add(GetPointData(el));
	}
	
	iter = linelist.begin();
	for ( ; iter != linelist.end(); ++iter) {
		const Element* el = dynamic_cast<Element*>(*iter);
		if (!el) {
			std::cerr << "CRITICAL: received a node which is not an element" 
				<< std::endl;
			exit(-1);
		}
		datagroup.Add(GetLineData(el));
	}
	
	iter = grouplist.begin();
	for ( ; iter != grouplist.end(); ++iter) {
		const Element* el = dynamic_cast<Element*>(*iter);
		if (!el) {
			std::cerr << "CRITICAL: received a node which is not an element" 
				<< std::endl;
			exit(-1);
		}
		// Hooray for recursion.
		datagroup.Add(GetDataGroup(el));
	}
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

	DataGroup datagroup(NO_DATATYPE, NO_ROTATIONGROUP, NO_ATTRIBUTES);
	// Pass parameters on to general datagroup handler.
	GetDataGroup(element, datagroup);
}

DataGroup
GPlatesReader::Read()
{
	DomParser parser;
	const Element* root;

	try {
		parser.parse_stream(_istr);

		if (parser) {
			// root is deleted by the parser.
			root = parser.get_document()->get_root_node();

			return GetRootDataGroup(root);

		} else
			std::cerr << "Parse failed." << std::endl;

	} catch (const std::exception& ex) {
		std::cerr << "Caught exception: " << ex.what() << std::endl;
	}

	// Parse failed, return empty DataGroup.
	return DataGroup(NO_DATATYPE, NO_ROTATIONGROUP, NO_ATTRIBUTES);
}
