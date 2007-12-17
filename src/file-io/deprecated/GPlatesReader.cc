/* $Id$ */

/**
 * @file 
 * Contains the parser implementation for the GPML.  
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <sstream>
#include <iterator>  /* for inserters */
#include <memory>    /* for auto_ptr */

#include "GPlatesReader.h"
#include "XMLParser.h"
#include "FileFormatException.h"
#include "InvalidDataException.h"
#include "global/types.h"
#include "maths/types.h"
#include "maths/LatLonPointConversions.h"
#include "maths/PointOnSphere.h"
#include "geo/Visitor.h"
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
typedef const Element* Element_ptr;
typedef Element::ElementList_type ElementList;


namespace
{

	void
	ReadError(const char* was_reading, unsigned int line)
	{
		std::ostringstream oss;
		oss << "Error when reading " << was_reading << " (line " 
			<< line << ")." << std::endl;
		throw FileFormatException(oss.str().c_str());
	}

	
	void
	InvalidDataError(const char* datatype, const char* got,
			const char* wanted, unsigned int line)
	{
		std::ostringstream oss;
		oss << "Invald " << datatype << " data encountered on line "
			<< line << "." << std::endl
			<< "Got: " << got << std::endl
			<< "Wanted: " << wanted << std::endl;
		throw InvalidDataException(oss.str().c_str());
	}

	
	void
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

	
	/**
	 * Read element @a to_read, which should be unique within
	 * @a element.  If @a element has no children, return
	 * @a default_value.
	 *
	 * @param element The element being read from.
	 * @param to_read The name of the element being read.  Only one
	 *   element with this name should be inside @a element.
	 * @param default_value If no element called @a to_read is found,
	 *   then this is returned.
	 * @throw FileFormatException Thrown if multiple children with
	 *   the same name are found.
	 */
	template < typename T >
	T
	ReadUnique(Element_ptr element, const char* to_read, 
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
	 * Extract the RotationGroupId from the given @a element's 
	 * content.
	 *
	 * @todo When tables are implemented, should check that the
	 *   return value is a valid plate.
	 */
	GPlatesGlobal::rid_t
	GetRotationGroupId(Element_ptr element)
	{
		return rid_t(ReadUnique< unsigned int >(element, "plateid",
			1000000));
	}

	
	/**
	 * Extract the DataType from the given @a element's content.
	 *
	 * @todo When tables are implemented, should check that the
	 *   return value is a valid datatype.
	 */
	GeologicalData::DataType_t
	GetDataType(Element_ptr element)
	{
		return ReadUnique< GeologicalData::DataType_t >(element, "datatype",
			GeologicalData::NO_DATATYPE);
	}

	
	/**
	 * Extract the ages of appearance and disappearance from the
	 * given @a element's content.
	 */
	TimeWindow
	GetTimeWindow(Element_ptr element)
	{
		GPlatesGlobal::fpdata_t appearance, disappearance;

		appearance = ReadUnique< GPlatesGlobal::fpdata_t >(element, 
			"ageofappearance", 0.0);

		disappearance = ReadUnique< GPlatesGlobal::fpdata_t >(element, 
			"ageofdisappearance", 0.0);
		
		return TimeWindow(appearance, disappearance);
	}

	
	/**
	 * Extract the Attributes from the given @a element.
	 *
	 * @todo Not yet implemented.
	 */
	GeologicalData::Attributes_t
	GetAttributes(Element_ptr)
	{
		return GeologicalData::NO_ATTRIBUTES;
	}

	
	/**
	 * Extract a LatLonPoint from the given @a text.  The line
	 * parameter is used for parse error messages, and should 
	 * refer to the line of the file from which the @a text
	 * was taken.
	 *
	 * @throw InvalidDataException Thrown if the latitude and
	 *   longitude values read are not in the valid range.
	 * @see GPlatesMaths::LatLonPoint::isValidLat(), 
	 *   GPlatesMaths::LatLonPoint::isValidLon().
	 */
	LatLonPoint
	GetLatLonPoint(Element_ptr element)
	{
		const std::string& text = element->GetContent();
		const unsigned int line = element->GetLineNumber();

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
		
		return LatLonPoint::LatLonPoint(lat, lon);
	}

	
	/**
	 * Create a new PointData object from the given @a element.
	 *
	 * @pre @a element must refer to a \<pointdata> element.
	 */
	PointData*
	GetPointData(Element_ptr element)
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
			 "", "",  // These are only used by the PLATES format.
			 GetAttributes(element),
			 make_point_on_sphere(GetLatLonPoint(*list.begin())));
	}

	
	/**
	 * Create a new PolylineOnSphere object from the given @a element.
	 *
	 * @pre @a element must refer to a \<coordlist> element.
	 * @throw FileFormatException Thrown if there is less than two
	 *   \<coord> elements in the \<coordlist>, since we need at least
	 *   two coordinates to specify a PolylineOnSphere.
	 */
	PolylineOnSphere
	GetCoordList(Element_ptr element)
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
			std::back_inserter(coordlist), &GetLatLonPoint);

		return make_polyline_on_sphere(coordlist);
	}

	
	/**
	 * Create a new LineData object from the given @a element.
	 *
	 * @pre @a element must refer to a \<linedata> element.
	 */
	LineData*
	GetLineData(Element_ptr element)
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
			"", "",  // These are only used by the PLATES format.
			GetAttributes(element),
			GetCoordList(*list.begin()));
	}

	
	/**
	 * Create a new DataGroup object from the given @a element.
	 *
	 * @pre @a element must refer to a \<datagroup> element.
	 * @warning This method rearranges the order of the children.
	 * @todo Construct a visitor to replace this method.
	 */
	DataGroup*
	GetDataGroup(Element_ptr element)
	{
		DataGroup::Children_t children;

		try
		{
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
				&GetDataGroup);  // Hooray for recursion.
		} 
		catch (...)
		{
			// Delete any children that we've read so far.
			DataGroup::Children_t::iterator iter = children.begin();
			for ( ; iter != children.end(); ++iter)
				delete *iter;

			throw;  // Rethrow...
		}
			
		return new DataGroup(GetDataType(element), 
				  GetRotationGroupId(element),
				  GetTimeWindow(element),
				  GetAttributes(element),
				  children);
	}

	
	/** 
	 * Handle meta data (title, and meta elements) in addition to
	 * normal datagroup stuff.
	 *
	 * @todo Doesn't read the meta data yet.
	 */
	DataGroup*
	GetRootDataGroup(Element_ptr element)
	{
		// Pass parameters on to general datagroup handler.
		DataGroup* dg = GetDataGroup(element);

		// Add stuff from title and meta tags
		// FIXME
		
		return dg;
	}

}  // End anonymous namespace


DataGroup*
GPlatesReader::Read()
{
	DataGroup* datagroup = NULL;

	try
	{
		// Create the pseudo-DOM hierarchy from the input.
		const std::auto_ptr<Element> root(XMLParser::Parse(_istr));

		// Transform the hierarchy into our internal format.
		datagroup = GetRootDataGroup(root.get());
	} 
	catch (...)
	{
		// Cleanup...
		// (root is automatically deleted.)
		delete datagroup;  // can safely delete NULL.

		// ...and rethrow
		throw;
	}

	return datagroup;	
}
