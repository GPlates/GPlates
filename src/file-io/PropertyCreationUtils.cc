/* $Id$ */

/**
 * @file 
 * This file contains the implementation of the functions in the
 * PropertyCreationUtils namespace.  You should read the documentation
 * found in the file  "src/file-io/HOWTO-add_support_for_a_new_property_type"
 * before editting this file.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>
#include <set>
#include <boost/optional.hpp>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/foreach.hpp>
#include <QTextStream>
#include <QStringList>

#include "PropertyCreationUtils.h"

#include "GpmlReaderUtils.h"
#include "ReadErrorAccumulation.h"
#include "StructurePropertyCreatorMap.h"

#include "global/GPlatesException.h"

#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/types.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/TemplateTypeParameterType.h"

#include "utils/Parse.h"
#include "utils/UnicodeStringUtils.h"


#define EXCEPTION_SOURCE BOOST_CURRENT_FUNCTION

namespace
{

	typedef GPlatesFileIO::PropertyCreationUtils::GpmlReaderException GpmlReaderException;


	template< typename T >
	bool
	parse_integral_value(
			T (QString::*parse_func)(bool *, int) const, 
			const QString &str, 
			T &out)
	{
		// We always read numbers in base 10.
		static const int BASE = 10;

		bool success = false;
		T tmp = (str.*parse_func)(&success, BASE);

		if (success) {
			out = tmp;
		}
		return success;
	}


	template< typename T >
	bool
	parse_decimal_value(
			T (QString::*parse_func)(bool *) const, 
			const QString &str, 
			T &out)
	{
		bool success = false;
		T tmp = (str.*parse_func)(&success);

		if (success) {
			out = tmp;
		}
		return success;
	}


	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
	get_xml_attributes_from_child(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			const GPlatesModel::PropertyName &prop_name)
	{
		GPlatesModel::XmlElementNode::named_child_const_iterator
			iter = elem->get_next_child_by_name(prop_name, elem->children_begin());

		if (iter.first == elem->children_end())
		{
			// We didn't find the property, return empty map.
			return std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>();
		}

		GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> result(
				target->attributes_begin(), target->attributes_end());
		return result;
	}


	template< typename T >
	boost::optional<T>
	find_and_create_optional(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			T (*creation_fn)(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &,
					GPlatesFileIO::ReadErrorAccumulation &),
			const GPlatesModel::PropertyName &prop_name,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		GPlatesModel::XmlElementNode::named_child_const_iterator 
			iter = elem->get_next_child_by_name(prop_name, elem->children_begin());

		if (iter.first == elem->children_end()) {
			// We didn't find the property, but that's okay here.
			return boost::optional<T>();
		}

		GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;

		// Increment iter:
		iter = elem->get_next_child_by_name(prop_name, ++iter.first);
		if (iter.first != elem->children_end()) {
			// Found duplicate!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::DuplicateProperty,
					EXCEPTION_SOURCE);
		}

		return (*creation_fn)(target, read_errors);  // Can throw.
	}


	template< typename T >
	T
	find_and_create_one(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			T (*creation_fn)(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &,
					GPlatesFileIO::ReadErrorAccumulation &),
			const GPlatesModel::PropertyName &prop_name,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		boost::optional<T> res = find_and_create_optional(elem, creation_fn, prop_name, read_errors);
		if ( ! res) {
			// Couldn't find the property!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
					EXCEPTION_SOURCE);
		}
		return *res;
	}


	template< typename T, typename CollectionOfT>
	void
	find_and_create_zero_or_more(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			T (*creation_fn)(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &,
					GPlatesFileIO::ReadErrorAccumulation &),
			const GPlatesModel::PropertyName &prop_name,
			CollectionOfT &destination,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		GPlatesModel::XmlElementNode::named_child_const_iterator 
                        iter = elem->get_next_child_by_name(prop_name, elem->children_begin());

		while (iter.first != elem->children_end()) {
			GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;

			destination.push_back((*creation_fn)(target, read_errors));  // creation_fn can throw.
			
			// Increment iter:
			iter = elem->get_next_child_by_name(prop_name, ++iter.first);
		}
	}


	template< typename T, typename CollectionOfT>
	void
	find_and_create_one_or_more(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			T (*creation_fn)(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &,
					GPlatesFileIO::ReadErrorAccumulation &),
			const GPlatesModel::PropertyName &prop_name,
			CollectionOfT &destination,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		find_and_create_zero_or_more(elem, creation_fn, prop_name, destination, read_errors);
		if (destination.empty()) {
			// Require at least one element in destination!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
					EXCEPTION_SOURCE);
		}
	}


	GPlatesModel::PropertyValue::non_null_ptr_type
	find_and_create_from_type(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			const GPlatesPropertyValues::TemplateTypeParameterType &type,
			const GPlatesModel::PropertyName &prop_name,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		GPlatesFileIO::StructurePropertyCreatorMap *map = 
			GPlatesFileIO::StructurePropertyCreatorMap::instance();
		GPlatesFileIO::StructurePropertyCreatorMap::iterator iter = map->find(type);

		if (iter == map->end()) {
			// We can't create the given type!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::UnknownValueType,
					EXCEPTION_SOURCE);
		}

		boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> 
			target = elem->get_child_by_name(prop_name);


		// Allow any number of children for string-types.
		static const GPlatesPropertyValues::TemplateTypeParameterType string_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("string");

		if ( ! (target 
				&& (*target)->attributes_empty() 
				&& ( (*target)->number_of_children() == 1 || type == string_type)
				)) {

			// Can't find target value!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::BadOrMissingTargetForValueType,
					EXCEPTION_SOURCE);
		}

		return (*iter->second)(*target, read_errors);
	}


	void
	find_and_create_one_or_more_from_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesPropertyValues::TemplateTypeParameterType &type,
		const GPlatesModel::PropertyName &prop_name,
		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &members,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		GPlatesFileIO::StructurePropertyCreatorMap *map =
			GPlatesFileIO::StructurePropertyCreatorMap::instance();
		GPlatesFileIO::StructurePropertyCreatorMap::iterator map_iter = map->find(type);

		if (map_iter == map->end()) {
			// We can't create the given type!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::UnknownValueType,
				EXCEPTION_SOURCE);
		}

		GPlatesModel::XmlElementNode::named_child_const_iterator
			iter = elem->get_next_child_by_name(prop_name, elem->children_begin());

		while (iter.first != elem->children_end()) {
			GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;

			//May need to check for attributes and number of children before adding to vector.
			members.push_back((*map_iter->second)(target, read_errors));  // creation_fn can throw.

			// Increment iter:
			iter = elem->get_next_child_by_name(prop_name, ++iter.first);
		}

	}



	class TextExtractionVisitor
		: public GPlatesModel::XmlNodeVisitor
	{
	public:
		explicit
		TextExtractionVisitor():
			d_encountered_subelement(false)
		{ }

		virtual
		void
		visit_element_node(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem) {
			d_encountered_subelement = true;
		}

		virtual
		void
		visit_text_node(
				const GPlatesModel::XmlTextNode::non_null_ptr_type &text) {
			d_text += text->get_text();
		}

		bool
		encountered_subelement() const {
			return d_encountered_subelement;
		}

		const QString &
		get_text() const {
			return d_text;
		}

	private:
		QString d_text;
		bool d_encountered_subelement;


		// Disallow copying
		TextExtractionVisitor(
				const TextExtractionVisitor &);

		// Disallow assignment.
		TextExtractionVisitor &
		operator=(
				const TextExtractionVisitor &);
	};


	QString
	create_string_without_trimming(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		TextExtractionVisitor visitor;
		std::for_each(
				elem->children_begin(), elem->children_end(),
				boost::bind(&GPlatesModel::XmlNode::accept_visitor, _1, boost::ref(visitor)));

		if (visitor.encountered_subelement()) {
			// String is wrong.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidString,
					EXCEPTION_SOURCE);
		}

		return visitor.get_text();
	}


	QString
	create_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		return create_string_without_trimming(elem, read_errors).trimmed();
	}


	QString
	create_nonempty_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_string(elem, read_errors);
		if (str.isEmpty()) {
			// Unexpected empty string.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::UnexpectedEmptyString,
					EXCEPTION_SOURCE);
		}
		return str;
	}


	GPlatesUtils::UnicodeString
	create_unicode_string(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		return GPlatesUtils::make_icu_string_from_qstring(create_string(elem, read_errors));
	}


	GPlatesPropertyValues::Enumeration::non_null_ptr_type
	create_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesUtils::UnicodeString &enum_type,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString enum_value = create_nonempty_string(elem, read_errors);
		return GPlatesPropertyValues::Enumeration::create(enum_type, 
				GPlatesUtils::make_icu_string_from_qstring(enum_value));
	}


	bool
	create_boolean(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		static const QString TRUE_STR = "true";
		static const QString FALSE_STR = "false";

		QString str = create_nonempty_string(elem, read_errors);

		if (str.compare(TRUE_STR, Qt::CaseInsensitive) == 0) {
			return true;
		} else if (str.compare(FALSE_STR, Qt::CaseInsensitive) == 0) {
			return false;
		}

		unsigned long val = 0;
		if ( ! parse_integral_value(&QString::toULong, str, val)) {
			// Can't convert the string to an integer.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidBoolean,
					EXCEPTION_SOURCE);
		}
		return val;
	}


	double
	create_double(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_nonempty_string(elem, read_errors);

		double res = 0.0;
		if ( ! parse_decimal_value(&QString::toDouble, str, res)) {
			// Can't convert the string to a double.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidDouble,
					EXCEPTION_SOURCE);
		}
		return res;
	}


	std::vector<double>
	create_double_list(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QStringList tokens = create_string(elem, read_errors).split(" ", QString::SkipEmptyParts);

		std::vector<double> result;
		result.reserve(tokens.count());
		GPlatesUtils::Parse<double> parse;
		try
		{
			BOOST_FOREACH(const QString &token, tokens)
			{
				result.push_back(parse(token));
			}
			
			return result;
		}
		catch (const GPlatesUtils::ParseError &)
		{
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidDouble,
					EXCEPTION_SOURCE);
		}
	}


	unsigned long
	create_ulong(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_nonempty_string(elem, read_errors);

		unsigned long res = 0;
		if ( ! parse_integral_value(&QString::toULong, str, res)) {
			// Can't convert the string to an unsigned long.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidUnsignedLong,
					EXCEPTION_SOURCE);
		}
		return res;
	}


	GPlatesPropertyValues::TemplateTypeParameterType 
	create_template_type_parameter_type(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_nonempty_string(elem, read_errors);

		QString alias = str.section(':', 0, 0);  // First chunk before a ':'.
		QString type = str.section(':', 1);  // The chunk after the ':'.
		boost::optional<QString> ns = elem->get_namespace_from_alias(alias);
		if ( ! ns) {
			// Couldn't find the namespace alias.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::MissingNamespaceAlias,
					EXCEPTION_SOURCE);
		}

		return GPlatesPropertyValues::TemplateTypeParameterType(*ns, alias, type);
	}


	int
	create_int(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_nonempty_string(elem, read_errors);

		int res = 0;
		if ( ! parse_integral_value(&QString::toInt, str, res)) {
			// Can't convert the string to an int.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidInt,
					EXCEPTION_SOURCE);
		}
		return res;
	}


	std::vector<int>
	create_int_list(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QStringList tokens = create_string(elem, read_errors).split(" ", QString::SkipEmptyParts);

		std::vector<int> result;
		result.reserve(tokens.count());
		GPlatesUtils::Parse<int> parse;
		try
		{
			BOOST_FOREACH(const QString &token, tokens)
			{
				result.push_back(parse(token));
			}
			
			return result;
		}
		catch (const GPlatesUtils::ParseError &)
		{
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidInt,
					EXCEPTION_SOURCE);
		}
	}


	unsigned int
	create_uint(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_nonempty_string(elem, read_errors);

		unsigned int res = 0;
		if ( ! parse_integral_value(&QString::toUInt, str, res)) {
			// Can't convert the string to an unsigned int.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidUnsignedInt,
					EXCEPTION_SOURCE);
		}
		return res;
	}


	size_t
	estimate_number_of_points(
			const QString &str)
	{
		/* This guess is based on the assumption that each coordinate will have
		 * three significant figures; thus every five characters will correspond to
		 * a coordinate (three for the coordinate, one for the decimal point, and 
		 * one for the delimiting space.
		 *
		 * Note that this estimate is deliberately conservative, since underestimating
		 * the number of chars in coordinate will result in an over-estimate of the 
		 * total number of coordinates, thus making reallocation of the vector (in 
		 * create_polyline below) much less likely.
		 *
		 * Also note that, at this stage, we're assuming that we're only reading in
		 * lat long points, hence there are two (2) coords per point.
		 */
		static const size_t CHARS_PER_COORD_ESTIMATE = 5;
		static const size_t COORDS_PER_POINT = 2;
		return str.length()/(CHARS_PER_COORD_ESTIMATE*COORDS_PER_POINT);
	}


	GPlatesMaths::PointOnSphere
	create_pos(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_nonempty_string(elem, read_errors);

		// XXX: Currently assuming srsDimension is 2!!

		QTextStream is(&str, QIODevice::ReadOnly);

		double lat = 0.0;
		double lon = 0.0;

		// FIXME: What should I do if one (or both) of these are screwed?
		// NOTE: We are assuming GPML is using (lat,lon) ordering.
		// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
		is >> lat;
		// FIXME: Check is.status() here!
		is >> lon;

		if ( ! (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
				GPlatesMaths::LatLonPoint::is_valid_longitude(lon))) {
			// Bad coordinates.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
					EXCEPTION_SOURCE);
		}
		return GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(lat, lon));
	}


	// Similar to create_pos but returns it as (lon, lat) pair.
	// This is to ensure that the longitude doesn't get wiped when reading in a
	// point physically at the north pole.
	std::pair<double, double>
	create_lon_lat_pos(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_nonempty_string(elem, read_errors);

		// XXX: Currently assuming srsDimension is 2!!

		QTextStream is(&str, QIODevice::ReadOnly);

		double lat = 0.0;
		double lon = 0.0;

		// FIXME: What should I do if one (or both) of these are screwed?
		// NOTE: We are assuming GPML is using (lat,lon) ordering.
		// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
		is >> lat;
		// FIXME: Check is.status() here!
		is >> lon;

		if ( ! (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
				GPlatesMaths::LatLonPoint::is_valid_longitude(lon))) {
			// Bad coordinates.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
					EXCEPTION_SOURCE);
		}
		return std::make_pair(lon, lat);
	}


	/**
	 * The same as @a create_pos, except that there's a comma between the two
	 * values instead of whitespace.
	 */
	GPlatesMaths::PointOnSphere
	create_coordinates(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_nonempty_string(elem, read_errors);

		// XXX: Currently assuming srsDimension is 2!!

		QStringList tokens = str.split(",");

		if (tokens.count() == 2)
		{
			try
			{
				GPlatesUtils::Parse<double> parse;
				double lat = parse(tokens.at(0));
				double lon = parse(tokens.at(1));

				if (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
						GPlatesMaths::LatLonPoint::is_valid_longitude(lon))
				{
					return GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(lat, lon));
				}
			}
			catch (...)
			{
				// Do nothing, fall through.
			}
		}

		throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
				EXCEPTION_SOURCE);
	}


	std::pair<double, double>
	create_lon_lat_coordinates(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString str = create_nonempty_string(elem, read_errors);

		// XXX: Currently assuming srsDimension is 2!!

		QStringList tokens = str.split(",");

		if (tokens.count() == 2)
		{
			try
			{
				GPlatesUtils::Parse<double> parse;
				double lat = parse(tokens.at(0));
				double lon = parse(tokens.at(1));

				if (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
						GPlatesMaths::LatLonPoint::is_valid_longitude(lon))
				{
					return std::make_pair(lon, lat);
				}
			}
			catch (...)
			{
				// Do nothing, fall through.
			}
		}

		throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
				EXCEPTION_SOURCE);
	}


	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	create_polyline(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		typedef GPlatesMaths::PolylineOnSphere polyline_type;

		QString str = create_nonempty_string(elem, read_errors);

		// XXX: Currently assuming srsDimension is 2!!

		std::vector<GPlatesMaths::PointOnSphere> points;
		points.reserve(estimate_number_of_points(str));

		QTextStream is(&str, QIODevice::ReadOnly);
		while ( ! is.atEnd() && (is.status() == QTextStream::Ok))
		{
			double lat = 0.0;
			double lon = 0.0;

			// FIXME: What should I do if one (or both) of these are screwed?
			// NOTE: We are assuming GPML is using (lat,lon) ordering.
			// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
			is >> lat;
			// FIXME: Check is.status() here!
			is >> lon;

			if ( ! (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
					GPlatesMaths::LatLonPoint::is_valid_longitude(lon))) {
				// Bad coordinates!
				throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
						EXCEPTION_SOURCE);
			}
			points.push_back(GPlatesMaths::make_point_on_sphere(
						GPlatesMaths::LatLonPoint(lat,lon)));
		}

		// Set up the return-parameter for the evaluate_construction_parameter_validity() function.
		std::pair<
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator, 
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator>
				invalid_points;
		// We want to return a different ReadError Description for each possible return
		// value of evaluate_construction_parameter_validity().
		polyline_type::ConstructionParameterValidity polyline_validity =
				polyline_type::evaluate_construction_parameter_validity(points, invalid_points);
		switch (polyline_validity)
		{
		case polyline_type::VALID:
				// All good.
				break;
		
		case polyline_type::INVALID_INSUFFICIENT_DISTINCT_POINTS:
				// Not enough points to make even a single (valid) line segment.
				throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InsufficientDistinctPointsInPolyline,
						EXCEPTION_SOURCE);
				break;
				
		case polyline_type::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
				// Segments of a polyline cannot be defined between two points which are antipodal.
				throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::AntipodalAdjacentPointsInPolyline,
						EXCEPTION_SOURCE);
				break;

		default:
				// Incompatible points encountered! For no defined reason!
				throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidPointsInPolyline,
						EXCEPTION_SOURCE);
				break;
		}
		return polyline_type::create_on_heap(points);
	}


	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
	create_polygon(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		typedef GPlatesMaths::PolygonOnSphere polygon_type;

		QString str = create_nonempty_string(elem, read_errors);

		// XXX: Currently assuming srsDimension is 2!!

		std::vector<GPlatesMaths::PointOnSphere> points;
		points.reserve(estimate_number_of_points(str));

		// Transform the text into a sequence of PointOnSphere.
		QTextStream is(&str, QIODevice::ReadOnly);
		while ( ! is.atEnd() && (is.status() == QTextStream::Ok))
		{
			double lat = 0.0;
			double lon = 0.0;

			// FIXME: What should I do if one (or both) of these are screwed?
			// NOTE: We are assuming GPML is using (lat,lon) ordering.
			// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
			is >> lat;
			// FIXME: Check is.status() here!
			is >> lon;

			if ( ! (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
					GPlatesMaths::LatLonPoint::is_valid_longitude(lon))) {
				// Bad coordinates!
				throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
						EXCEPTION_SOURCE);
			}
			points.push_back(GPlatesMaths::make_point_on_sphere(
						GPlatesMaths::LatLonPoint(lat,lon)));
		}
		
		// GML Polygons require the first and last points of a polygon to be identical,
		// because the format wasn't verbose enough. GPlates expects that the first
		// and last points of a PolygonOnSphere are implicitly joined.
		if (points.size() >= 4) {
			GPlatesMaths::PointOnSphere &p1 = *(points.begin());
			GPlatesMaths::PointOnSphere &p2 = *(--points.end());
			if (p1 == p2) {
				points.pop_back();
			} else {
				throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidPolygonEndPoint,
						EXCEPTION_SOURCE);
			}
		} else {
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InsufficientPointsInPolygon,
					EXCEPTION_SOURCE);
		}

		// Set up the return-parameter for the evaluate_construction_parameter_validity() function.
		std::pair<
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator, 
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator>
				invalid_points;
		// We want to return a different ReadError Description for each possible return
		// value of evaluate_construction_parameter_validity().
		polygon_type::ConstructionParameterValidity polygon_validity =
				polygon_type::evaluate_construction_parameter_validity(points, invalid_points);
		switch (polygon_validity)
		{
		case polygon_type::VALID:
				// All good.
				break;
		
		case polygon_type::INVALID_INSUFFICIENT_DISTINCT_POINTS:
				// Less good - not enough points, although we have already checked for
				// this earlier in the function. So it must be a problem with coincident points.
				throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InsufficientDistinctPointsInPolygon,
						EXCEPTION_SOURCE);
				break;
				
		case polygon_type::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
				// Segments of a polygon cannot be defined between two points which are antipodal.
				throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::AntipodalAdjacentPointsInPolygon,
						EXCEPTION_SOURCE);
				break;

		default:
				// Incompatible points encountered! For no defined reason!
				throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidPointsInPolygon,
						EXCEPTION_SOURCE);
				break;
		}
		return polygon_type::create_on_heap(points);
	}


	/**
	 * This function will extract the single child of the given elem and return
	 * it.  If there is more than one child, or the type was not found,
	 * an exception is thrown.
	 */
	GPlatesModel::XmlElementNode::non_null_ptr_type
	get_structural_type_element(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			const GPlatesModel::PropertyName &prop_name)
	{
		// Look for the structural type...
		boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type >
			structural_elem = elem->get_child_by_name(prop_name);

		if (elem->number_of_children() > 1) {
			// Properties with multiple inline structural elements are not (yet) handled!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::NonUniqueStructuralElement,
					EXCEPTION_SOURCE);
		} else if (! structural_elem) {
			// Could not locate structural element!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::StructuralElementNotFound,
					EXCEPTION_SOURCE);
		}
		return *structural_elem;
	}


	/**
	 * This function is used by create_gml_polygon to traverse the LinearRing intermediate junk.
	 */
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
	create_linear_ring(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		static const GPlatesModel::PropertyName 
			STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("LinearRing"),
			POS_LIST = GPlatesModel::PropertyName::create_gml("posList");
	
		GPlatesModel::XmlElementNode::non_null_ptr_type
			elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
			polygon = find_and_create_one(elem, &create_polygon, POS_LIST, read_errors);
	
		// FIXME: We need to give the srsName et al. attributes from the posList 
		// (or the gml:FeatureCollection tag?) to the GmlPolygon (or the FeatureCollection)!
		return polygon;
	}


	/**
	 * This function is used by create_point and create_gml_multi_point to do the
	 * common work of creating a GPlatesMaths::PointOnSphere.
	 */
	std::pair<GPlatesMaths::PointOnSphere, GPlatesPropertyValues::GmlPoint::GmlProperty>
	create_point_on_sphere(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		static const GPlatesModel::PropertyName
			STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("Point"),
			POS = GPlatesModel::PropertyName::create_gml("pos"),
			COORDINATES = GPlatesModel::PropertyName::create_gml("coordinates");

		GPlatesModel::XmlElementNode::non_null_ptr_type
			elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

		// FIXME: We need to give the srsName et al. attributes from the pos 
		// (or the gml:FeatureCollection tag?) to the GmlPoint or GmlMultiPoint.
		boost::optional<GPlatesMaths::PointOnSphere> point_as_pos =
			find_and_create_optional(elem, &create_pos, POS, read_errors);
		boost::optional<GPlatesMaths::PointOnSphere> point_as_coordinates =
			find_and_create_optional(elem, &create_coordinates, COORDINATES, read_errors);

		// The gml:Point needs one of gml:pos and gml:coordinates, but not both.
		if (point_as_pos && point_as_coordinates)
		{
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::DuplicateProperty,
					EXCEPTION_SOURCE);
		}
		if (!point_as_pos && !point_as_coordinates)
		{
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
					EXCEPTION_SOURCE);
		}

		if (point_as_pos)
		{
			return std::make_pair(*point_as_pos, GPlatesPropertyValues::GmlPoint::POS);
		}
		else
		{
			return std::make_pair(*point_as_coordinates, GPlatesPropertyValues::GmlPoint::COORDINATES);
		}
	}


	std::pair<std::pair<double, double>, GPlatesPropertyValues::GmlPoint::GmlProperty>
	create_lon_lat_point_on_sphere(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		static const GPlatesModel::PropertyName
			STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("Point"),
			POS = GPlatesModel::PropertyName::create_gml("pos"),
			COORDINATES = GPlatesModel::PropertyName::create_gml("coordinates");

		GPlatesModel::XmlElementNode::non_null_ptr_type
			elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

		// FIXME: We need to give the srsName et al. attributes from the pos 
		// (or the gml:FeatureCollection tag?) to the GmlPoint or GmlMultiPoint.
		boost::optional<std::pair<double, double> > point_as_pos =
			find_and_create_optional(elem, &create_lon_lat_pos, POS, read_errors);
		boost::optional<std::pair<double, double> > point_as_coordinates =
			find_and_create_optional(elem, &create_lon_lat_coordinates, COORDINATES, read_errors);

		// The gml:Point needs one of gml:pos and gml:coordinates, but not both.
		if (point_as_pos && point_as_coordinates)
		{
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::DuplicateProperty,
					EXCEPTION_SOURCE);
		}
		if (!point_as_pos && !point_as_coordinates)
		{
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
					EXCEPTION_SOURCE);
		}

		if (point_as_pos)
		{
			return std::make_pair(*point_as_pos, GPlatesPropertyValues::GmlPoint::POS);
		}
		else
		{
			return std::make_pair(*point_as_coordinates, GPlatesPropertyValues::GmlPoint::COORDINATES);
		}
	}


	class ValueObjectTemplateVisitor :
			public GPlatesModel::XmlNodeVisitor
	{
	public:

		virtual
		void
		visit_text_node(
				const GPlatesModel::XmlTextNode::non_null_ptr_type &text)
		{
			// Do nothing; we don't want text nodes.
		}

		virtual
		void
		visit_element_node(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
		{
			GPlatesPropertyValues::GmlFile::xml_attributes_type xml_attributes(
					elem->attributes_begin(), elem->attributes_end());
			d_result = GPlatesPropertyValues::GmlFile::value_component_type(
					GPlatesPropertyValues::ValueObjectType(elem->get_name()),
					xml_attributes);
		}

		const boost::optional<GPlatesPropertyValues::GmlFile::value_component_type> &
		get_result() const
		{
			return d_result;
		}

	private:

		boost::optional<GPlatesPropertyValues::GmlFile::value_component_type> d_result;
	};


	/**
	 * Extracts out the value object template, i.e. the app:Temperature part of the
	 * example on p.253 of the GML book.
	 */
	GPlatesPropertyValues::GmlFile::value_component_type
	create_gml_file_value_component(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		if (parent->number_of_children() > 1)
		{
			// Properties with multiple inline structural elements are not (yet) handled!
			throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::NonUniqueStructuralElement,
					EXCEPTION_SOURCE);
		}
		else if (parent->number_of_children() == 0)
		{
			// Could not locate structural element template!
			throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::StructuralElementNotFound,
					EXCEPTION_SOURCE);
		}

		// Pull the answer out of the child if it is an XmlElementNode.
		GPlatesModel::XmlNode::non_null_ptr_type elem = *parent->children_begin();
		ValueObjectTemplateVisitor visitor;
		elem->accept_visitor(visitor);

		if (visitor.get_result())
		{
			return *visitor.get_result();
		}
		else
		{
			// It must have been a text element inside the <gml:valueComponent>.
			throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::StructuralElementNotFound,
					EXCEPTION_SOURCE);
		}
	}


	/**
	 * Used by create_file to create the gml:CompositeValue structural type inside
	 * a gml:File.
	 */
	GPlatesPropertyValues::GmlFile::composite_value_type
	create_gml_file_composite_value(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		using namespace GPlatesPropertyValues;

		static const GPlatesModel::PropertyName
			STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("CompositeValue"),
			VALUE_COMPONENT = GPlatesModel::PropertyName::create_gml("valueComponent");

		GPlatesModel::XmlElementNode::non_null_ptr_type elem =
			get_structural_type_element(parent, STRUCTURAL_TYPE);

		GmlFile::composite_value_type result;
		find_and_create_zero_or_more(elem, &create_gml_file_value_component, VALUE_COMPONENT, result, read_errors);

		return result;
	}
}


GPlatesPropertyValues::XsBoolean::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_xs_boolean(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::XsBoolean::create(create_boolean(elem, read_errors));
}


GPlatesPropertyValues::XsInteger::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_xs_integer(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::XsInteger::create(create_int(elem, read_errors));
}


GPlatesPropertyValues::XsString::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_xs_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::XsString::create(
			GPlatesUtils::make_icu_string_from_qstring(create_string(elem, read_errors)));
}


GPlatesPropertyValues::XsDouble::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_xs_double(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::XsDouble::create(create_double(elem, read_errors));
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_absolute_reference_frame_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:AbsoluteReferenceFrameEnumeration", read_errors);
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_continental_boundary_crust_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:ContinentalBoundaryCrustEnumeration", read_errors);
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_continental_boundary_edge_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:ContinentalBoundaryEdgeEnumeration", read_errors);
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_continental_boundary_side_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:ContinentalBoundarySideEnumeration", read_errors);
}

GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_reconstruction_method_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:ReconstructionMethodEnumeration", read_errors);
}

GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_dip_side_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:DipSideEnumeration", read_errors);
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_dip_slip_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:DipSlipEnumeration", read_errors);
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_fold_plane_annotation_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:FoldPlaneAnnotationEnumeration", read_errors);
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_slip_component_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:SlipComponentEnumeration", read_errors);
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_strike_slip_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:StrikeSlipEnumeration", read_errors);
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_subduction_polarity_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:SubductionPolarityEnumeration", read_errors);
}

GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_slab_edge_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return create_enumeration(elem, "gpml:SlabEdgeEnumeration", read_errors);
}

GPlatesModel::FeatureId
GPlatesFileIO::PropertyCreationUtils::create_feature_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesModel::FeatureId(
		GPlatesUtils::make_icu_string_from_qstring(create_nonempty_string(elem, read_errors)));
}


GPlatesModel::RevisionId
GPlatesFileIO::PropertyCreationUtils::create_revision_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{	
	return GPlatesModel::RevisionId(
		GPlatesUtils::make_icu_string_from_qstring(create_nonempty_string(elem, read_errors)));
}


GPlatesPropertyValues::GpmlRevisionId::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_revision_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::GpmlRevisionId::create(create_revision_id(elem, read_errors));
}


GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_plate_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::GpmlPlateId::create(create_ulong(elem, read_errors));
}


GPlatesPropertyValues::GeoTimeInstant
GPlatesFileIO::PropertyCreationUtils::create_geo_time_instant(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{
	// FIXME:  Find and store the 'frame' attribute in the GeoTimeInstant.

	QString text = create_nonempty_string(elem, read_errors);
	if (text.compare("http://gplates.org/times/distantFuture", Qt::CaseInsensitive) == 0) {
		return GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	}
	if (text.compare("http://gplates.org/times/distantPast", Qt::CaseInsensitive) == 0) {
		return GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
	}

	double time = 0.0;
	if ( ! parse_decimal_value(&QString::toDouble, text, time)) {
		// Can't convert the string to a geo time.
		throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidGeoTime,
				EXCEPTION_SOURCE);
	}
	return GPlatesPropertyValues::GeoTimeInstant(time);
}


GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_time_instant(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("TimeInstant"),
		TIME_POSITION = GPlatesModel::PropertyName::create_gml("timePosition");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GeoTimeInstant
		time = find_and_create_one(elem, &create_geo_time_instant, TIME_POSITION, read_errors);

	// The XML attributes are read from the timePosition property, not the TimeInstant property.
	return GPlatesPropertyValues::GmlTimeInstant::create(
			time,
			get_xml_attributes_from_child(elem, TIME_POSITION));
}


GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_time_period(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("TimePeriod"),
		BEGIN_TIME = GPlatesModel::PropertyName::create_gml("begin"),
		END_TIME = GPlatesModel::PropertyName::create_gml("end");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		begin_time = find_and_create_one(elem, &create_time_instant, BEGIN_TIME, read_errors), 
		end_time = find_and_create_one(elem, &create_time_instant, END_TIME, read_errors);

	return GPlatesPropertyValues::GmlTimePeriod::create(begin_time, end_time);
}


GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_constant_value(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("ConstantValue"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		VALUE = GPlatesModel::PropertyName::create_gpml("value"),
		DESCRIPTION = GPlatesModel::PropertyName::create_gpml("description");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	boost::optional<QString> 
		description = find_and_create_optional(elem, &create_string, DESCRIPTION, read_errors);
	GPlatesPropertyValues::TemplateTypeParameterType 
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);
	GPlatesModel::PropertyValue::non_null_ptr_type value = 
		find_and_create_from_type(elem, type, VALUE, read_errors);

	if (description) {
		return GPlatesPropertyValues::GpmlConstantValue::create(value, type, 
				GPlatesUtils::make_icu_string_from_qstring(*description));
	}
	return GPlatesPropertyValues::GpmlConstantValue::create(value, type);
}


GPlatesPropertyValues::GpmlTimeSample
GPlatesFileIO::PropertyCreationUtils::create_time_sample(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("TimeSample"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		VALUE = GPlatesModel::PropertyName::create_gpml("value"),
		VALID_TIME = GPlatesModel::PropertyName::create_gpml("validTime"),
		DESCRIPTION = GPlatesModel::PropertyName::create_gml("description"),
		IS_DISABLED = GPlatesModel::PropertyName::create_gpml("isDisabled");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);
	GPlatesModel::PropertyValue::non_null_ptr_type 
		value = find_and_create_from_type(elem, type, VALUE, read_errors);
	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		valid_time = find_and_create_one(elem, &create_time_instant, VALID_TIME, read_errors);
	boost::optional<QString>
		description = find_and_create_optional(elem, &create_string_without_trimming, DESCRIPTION, read_errors);
	boost::optional<bool>
		is_disabled = find_and_create_optional(elem, &create_boolean, IS_DISABLED, read_errors);

	boost::intrusive_ptr<GPlatesPropertyValues::XsString> desc;
	if (description) {
		GPlatesPropertyValues::XsString::non_null_ptr_type tmp = 
			GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(*description));
		desc = GPlatesUtils::get_intrusive_ptr(tmp);
	}

	if (is_disabled) {
		return GPlatesPropertyValues::GpmlTimeSample(
				value, valid_time, desc, type, *is_disabled);
	}
	return GPlatesPropertyValues::GpmlTimeSample(value, valid_time, desc, type);
}


GPlatesPropertyValues::GpmlTimeWindow
GPlatesFileIO::PropertyCreationUtils::create_time_window(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("TimeWindow"),
		TIME_DEPENDENT_PROPERTY_VALUE = 
			GPlatesModel::PropertyName::create_gpml("timeDependentPropertyValue"),
		VALID_TIME = GPlatesModel::PropertyName::create_gpml("validTime"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesModel::PropertyValue::non_null_ptr_type
		time_dep_prop_val = find_and_create_one(elem, &create_time_dependent_property_value,
				TIME_DEPENDENT_PROPERTY_VALUE, read_errors);
	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
		time_period = find_and_create_one(elem, &create_time_period, VALID_TIME, read_errors);
	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);

	return GPlatesPropertyValues::GpmlTimeWindow(time_dep_prop_val, time_period, type);
}


GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_piecewise_aggregation(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("PiecewiseAggregation"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TIME_WINDOW = GPlatesModel::PropertyName::create_gpml("timeWindow");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);

	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows;

	find_and_create_zero_or_more(elem, &create_time_window, TIME_WINDOW, time_windows, read_errors);

	return GPlatesPropertyValues::GpmlPiecewiseAggregation::create(time_windows, type);
}


GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_irregular_sampling(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("IrregularSampling"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TIME_SAMPLE = GPlatesModel::PropertyName::create_gpml("timeSample"),
		INTERPOLATION_FUNCTION = GPlatesModel::PropertyName::create_gpml("interpolationFunction");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);
	boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
		interp_func = find_and_create_optional(elem, &create_interpolation_function, 
				INTERPOLATION_FUNCTION, read_errors);

	std::vector<GPlatesPropertyValues::GpmlTimeSample> time_samples;
	find_and_create_one_or_more(elem, &create_time_sample, TIME_SAMPLE, time_samples, read_errors);

	if (interp_func) {
		return GPlatesPropertyValues::GpmlIrregularSampling::create(
				time_samples, GPlatesUtils::get_intrusive_ptr(*interp_func), type);
	}
	return GPlatesPropertyValues::GpmlIrregularSampling::create(
				time_samples, 
				GPlatesPropertyValues::GpmlInterpolationFunction::maybe_null_ptr_type(), 
				type);
}


GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_hot_spot_trail_mark(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{	
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("HotSpotTrailMark"),
		POSITION = GPlatesModel::PropertyName::create_gpml("position"),
		TRAIL_WIDTH = GPlatesModel::PropertyName::create_gpml("trailWidth"),
		MEASURED_AGE = GPlatesModel::PropertyName::create_gpml("measuredAge"),
		MEASURED_AGE_RANGE = GPlatesModel::PropertyName::create_gpml("measuredAgeRange");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GmlPoint::non_null_ptr_type
		position = find_and_create_one(elem, &create_point, POSITION, read_errors);
	boost::optional< GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type >
		trail_width = find_and_create_optional(elem, &create_measure, TRAIL_WIDTH, read_errors);
	boost::optional< GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type >
		measured_age = find_and_create_optional(elem, &create_time_instant, MEASURED_AGE, read_errors);
	boost::optional< GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type >
		measured_age_range = find_and_create_optional(elem, &create_time_period, 
				MEASURED_AGE_RANGE, read_errors);

	return GPlatesPropertyValues::GpmlHotSpotTrailMark::create(
			position, trail_width, measured_age, measured_age_range);
}


GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_measure(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		ReadErrorAccumulation &read_errors)
{	
	double quantity = create_double(elem, read_errors);

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
		xml_attrs(elem->attributes_begin(), elem->attributes_end());
	return GPlatesPropertyValues::GpmlMeasure::create(quantity, xml_attrs);
}


GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_feature_reference(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("FeatureReference"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::PropertyName::create_gpml("targetFeature");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		value_type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);
	GPlatesModel::FeatureId
		target_feature = find_and_create_one(elem, &create_feature_id, TARGET_FEATURE, read_errors);

	return GPlatesPropertyValues::GpmlFeatureReference::create(target_feature, value_type);
}


GPlatesPropertyValues::GpmlFeatureSnapshotReference::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_feature_snapshot_reference(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("FeatureSnapshotReference"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::PropertyName::create_gpml("targetFeature"),
		TARGET_REVISION = GPlatesModel::PropertyName::create_gpml("targetRevision");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		value_type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);
	GPlatesModel::FeatureId
		target_feature = find_and_create_one(elem, &create_feature_id, TARGET_FEATURE, read_errors);
	GPlatesModel::RevisionId
		target_revision = find_and_create_one(elem, &create_revision_id, TARGET_REVISION, read_errors);

	return GPlatesPropertyValues::GpmlFeatureSnapshotReference::create(
			target_feature, target_revision, value_type);
}


GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_property_delegate(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("PropertyDelegate"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::PropertyName::create_gpml("targetFeature"),
		TARGET_PROPERTY = GPlatesModel::PropertyName::create_gpml("targetProperty");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		value_type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);
	GPlatesModel::FeatureId
		target_feature = find_and_create_one(elem, &create_feature_id, TARGET_FEATURE, read_errors);
	GPlatesPropertyValues::TemplateTypeParameterType
		target_property = find_and_create_one(elem, &create_template_type_parameter_type, 
				TARGET_PROPERTY, read_errors);

	GPlatesModel::PropertyName prop_name(target_property);
	return GPlatesPropertyValues::GpmlPropertyDelegate::create(
			target_feature, prop_name, value_type);
}


GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_polarity_chron_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("PolarityChronId"),
		ERA   = GPlatesModel::PropertyName::create_gpml("era"),
		MAJOR = GPlatesModel::PropertyName::create_gpml("major"),
		MINOR = GPlatesModel::PropertyName::create_gpml("minor");
	
	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	boost::optional<QString>
		era = find_and_create_optional(elem, &create_string, ERA, read_errors);
	boost::optional<unsigned int>
		major_region = find_and_create_optional(elem, &create_uint, MAJOR, read_errors);
	boost::optional<QString>
		minor_region = find_and_create_optional(elem, &create_string, MINOR, read_errors);

	return GPlatesPropertyValues::GpmlPolarityChronId::create(era, major_region, minor_region);
}


GPlatesPropertyValues::GmlPoint::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_point(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	std::pair<std::pair<double, double>, GPlatesPropertyValues::GmlPoint::GmlProperty> point =
		create_lon_lat_point_on_sphere(parent, read_errors);

	// FIXME: We need to give the srsName et al. attributes from the posList 
	// to the line string!
	return GPlatesPropertyValues::GmlPoint::create(point.first, point.second);
}


GPlatesPropertyValues::GmlLineString::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_line_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("LineString"),
		POS_LIST = GPlatesModel::PropertyName::create_gml("posList");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		polyline = find_and_create_one(elem, &create_polyline, POS_LIST, read_errors);

	// FIXME: We need to give the srsName et al. attributes from the posList 
	// to the line string!
	return GPlatesPropertyValues::GmlLineString::create(polyline);
}


GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gml_multi_point(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("MultiPoint"),
		POINT_MEMBER = GPlatesModel::PropertyName::create_gml("pointMember");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	// GmlMultiPoint has multiple gml:pointMember properties each containing a
	// single gml:Point.
	typedef std::pair<GPlatesMaths::PointOnSphere, GPlatesPropertyValues::GmlPoint::GmlProperty> point_and_property_type;
	std::vector<point_and_property_type> points_and_properties;
	find_and_create_one_or_more(elem, &create_point_on_sphere, POINT_MEMBER, points_and_properties, read_errors);

	// Unpack the vector of pairs into two vectors.
	std::vector<GPlatesMaths::PointOnSphere> points;
	points.reserve(points_and_properties.size());
	std::vector<GPlatesPropertyValues::GmlPoint::GmlProperty> properties;
	properties.reserve(points_and_properties.size());
	BOOST_FOREACH(const point_and_property_type &point_and_property, points_and_properties)
	{
		points.push_back(point_and_property.first);
		properties.push_back(point_and_property.second);
	}

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
		multipoint = GPlatesMaths::MultiPointOnSphere::create_on_heap(points);

	// FIXME: We need to give the srsName et al. attributes from the gml:Point
	// (or the gml:FeatureCollection tag?) to the GmlMultiPoint (or the FeatureCollection)!
	return GPlatesPropertyValues::GmlMultiPoint::create(multipoint, properties);
}


GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_orientable_curve(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("OrientableCurve"),
		BASE_CURVE = GPlatesModel::PropertyName::create_gml("baseCurve");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GmlLineString::non_null_ptr_type
		line_string = find_and_create_one(elem, &create_line_string, BASE_CURVE, read_errors);

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
		xml_attrs(elem->attributes_begin(), elem->attributes_end());
	return GPlatesPropertyValues::GmlOrientableCurve::create(line_string, xml_attrs);
}


GPlatesPropertyValues::GmlPolygon::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gml_polygon(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("Polygon"),
		INTERIOR = GPlatesModel::PropertyName::create_gml("interior"),
		EXTERIOR = GPlatesModel::PropertyName::create_gml("exterior");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	// GmlPolygon has exactly one exterior gml:LinearRing
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		exterior = find_and_create_one(elem, &create_linear_ring, EXTERIOR, read_errors);

	// GmlPolygon has zero or more interior gml:LinearRing
	std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> interiors;
	find_and_create_zero_or_more(elem, &create_linear_ring, INTERIOR, interiors, read_errors);

	// FIXME: We need to give the srsName et al. attributes from the posList 
	// (or the gml:FeatureCollection tag?) to the GmlPolygon (or the FeatureCollection)!
	return GPlatesPropertyValues::GmlPolygon::create<
			std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> >(exterior, interiors);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_geometry(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	static GPlatesModel::PropertyName
		POINT = GPlatesModel::PropertyName::create_gml("Point"),
		LINE_STRING = GPlatesModel::PropertyName::create_gml("LineString"),
		ORIENTABLE_CURVE = GPlatesModel::PropertyName::create_gml("OrientableCurve"),
		POLYGON = GPlatesModel::PropertyName::create_gml("Polygon"),
		CONSTANT_VALUE = GPlatesModel::PropertyName::create_gpml("ConstantValue");
	
	if (parent->number_of_children() > 1) {
		// Too many children!
		throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				EXCEPTION_SOURCE);
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(POINT);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_point(parent, read_errors));
	}

	structural_elem = parent->get_child_by_name(LINE_STRING);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_line_string(parent, read_errors));
	}

	structural_elem = parent->get_child_by_name(ORIENTABLE_CURVE);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_orientable_curve(parent, read_errors));
	}
	
	structural_elem = parent->get_child_by_name(POLYGON);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_gml_polygon(parent, read_errors));
	}
	
	// If we reach this point, we have found no valid children for a gml:_Geometry property value.
	// However, we can still test for a few common things to aid debugging.
	
	// Did someone use a gpml:ConstantValue<gml:_Geometry> property where a regular gml:_Geometry
	// was expected?
	structural_elem = parent->get_child_by_name(CONSTANT_VALUE);
	if (structural_elem) {
#if 0
		// FIXME: Proper behaviour? I'd prefer to just add a warning to the ReadErrorAccumulation and
		// handle the ConstantValue by recursing to this function (skipping the ConstantValue),
		// but for the moment the only way to get word out is exceptions - a non-fatal warning
		// would need some clever refactoring.
		throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::ConstantValueOnNonTimeDependentProperty,
			EXCEPTION_SOURCE);
#else
		// The alternative for now is, just assume the ConstantValue is there for a good reason,
		// read it, and return it (including whatever it was wrapping, which we should hope was
		// some geometry!)
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_constant_value(parent, read_errors));
#endif
	}

	// (Unknown) Invalid child!
	throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_time_dependent_property_value(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		CONSTANT_VALUE = GPlatesModel::PropertyName::create_gpml("ConstantValue"),
		IRREGULAR_SAMPLING = GPlatesModel::PropertyName::create_gpml("IrregularSampling"),
		PIECEWISE_AGGREGATION = GPlatesModel::PropertyName::create_gpml("PiecewiseAggregation");

	if (parent->number_of_children() > 1) {
		// Too many children!
		throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				EXCEPTION_SOURCE);
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(CONSTANT_VALUE);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(
				create_constant_value(parent, read_errors));
	}

	structural_elem = parent->get_child_by_name(IRREGULAR_SAMPLING);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(
				create_irregular_sampling(parent, read_errors));
	}

	structural_elem = parent->get_child_by_name(PIECEWISE_AGGREGATION);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(
				create_piecewise_aggregation(parent, read_errors));
	}

	// Invalid child!
	throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_interpolation_function(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		FINITE_ROTATION_SLERP = GPlatesModel::PropertyName::create_gpml("FiniteRotationSlerp");

	if (parent->number_of_children() > 1) {
		// Too many children!
		throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				EXCEPTION_SOURCE);
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(FINITE_ROTATION_SLERP);
	if (structural_elem) {
		return GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type(
				create_finite_rotation_slerp(parent, read_errors));
	}

	// Invalid child!
	throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_finite_rotation(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		AXIS_ANGLE_FINITE_ROTATION = 
			GPlatesModel::PropertyName::create_gpml("AxisAngleFiniteRotation"),
		ZERO_FINITE_ROTATION = GPlatesModel::PropertyName::create_gpml("ZeroFiniteRotation");

	if (parent->number_of_children() > 1) {
		// Too many children!
		throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				EXCEPTION_SOURCE);
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;
   
	structural_elem = parent->get_child_by_name(AXIS_ANGLE_FINITE_ROTATION);
	if (structural_elem) {
		static const GPlatesModel::PropertyName
			EULER_POLE = GPlatesModel::PropertyName::create_gpml("eulerPole"),
			ANGLE = GPlatesModel::PropertyName::create_gpml("angle");
		
		GPlatesPropertyValues::GmlPoint::non_null_ptr_type
			euler_pole = find_and_create_one(*structural_elem, &create_point, EULER_POLE, read_errors);
		GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type
			angle = find_and_create_one(*structural_elem, &create_measure, ANGLE, read_errors);

		return GPlatesPropertyValues::GpmlFiniteRotation::create(euler_pole, angle);
	} 

	structural_elem = parent->get_child_by_name(ZERO_FINITE_ROTATION);
	if (structural_elem) {
		return GPlatesPropertyValues::GpmlFiniteRotation::create_zero_rotation();
	}

	// Invalid child!
	throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_finite_rotation_slerp(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("FiniteRotationSlerp"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		value_type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);

	return GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(value_type);
}


GPlatesPropertyValues::GpmlStringList::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_string_list(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("StringList"),
		ELEMENT = GPlatesModel::PropertyName::create_gpml("element");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesUtils::UnicodeString> elements;
	find_and_create_zero_or_more(elem, &create_unicode_string, ELEMENT, elements, read_errors);
	return GPlatesPropertyValues::GpmlStringList::create_copy(elements);
}


GPlatesPropertyValues::GpmlTopologicalInterior::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_topological_interior(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = 
			GPlatesModel::PropertyName::create_gpml("TopologicalInterior"),
		SECTION = 
			GPlatesModel::PropertyName::create_gpml("section");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> sections;

	find_and_create_one_or_more(elem, &create_topological_section, SECTION, sections, read_errors);

	return GPlatesPropertyValues::GpmlTopologicalInterior::create( sections );
}

GPlatesPropertyValues::GpmlTopologicalPolygon::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_topological_polygon(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = 
			GPlatesModel::PropertyName::create_gpml("TopologicalPolygon"),
		SECTION = 
			GPlatesModel::PropertyName::create_gpml("section");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> sections;

	find_and_create_one_or_more(elem, &create_topological_section, SECTION, sections, read_errors);

	return GPlatesPropertyValues::GpmlTopologicalPolygon::create( sections );
}


GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_topological_line(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = 
			GPlatesModel::PropertyName::create_gpml("TopologicalLine"),
		SECTION = 
			GPlatesModel::PropertyName::create_gpml("section");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> sections;

	find_and_create_one_or_more(elem, &create_topological_section, SECTION, sections, read_errors);

	return GPlatesPropertyValues::GpmlTopologicalLine::create( sections );
}


GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_topological_section(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		TOPOLOGICAL_LINE_SECTION = 
			GPlatesModel::PropertyName::create_gpml("TopologicalLineSection"),
		TOPOLOGICAL_POINT = 
			GPlatesModel::PropertyName::create_gpml("TopologicalPoint");

	if (parent->number_of_children() > 1) {
		// Too many children!
		throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				EXCEPTION_SOURCE);
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(TOPOLOGICAL_LINE_SECTION);
	if (structural_elem) {
		return GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type(
				create_topological_line_section(parent, read_errors));
	}

	structural_elem = parent->get_child_by_name(TOPOLOGICAL_POINT);
	if (structural_elem) {
		return GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type(
				create_topological_point(parent, read_errors));
	}

	// Invalid child!
	throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_topological_line_section(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = 
			GPlatesModel::PropertyName::create_gpml("TopologicalLineSection"),
		SOURCE_GEOMETRY = 
			GPlatesModel::PropertyName::create_gpml("sourceGeometry"),
		START_INTERSECTION = 
			GPlatesModel::PropertyName::create_gpml("startIntersection"),
		END_INTERSECTION = 
			GPlatesModel::PropertyName::create_gpml("endIntersection"),
		REVERSE_ORDER = 
			GPlatesModel::PropertyName::create_gpml("reverseOrder");


	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type source_geometry = 
		find_and_create_one(elem, &create_property_delegate, SOURCE_GEOMETRY, read_errors);

	boost::optional<GPlatesPropertyValues::GpmlTopologicalIntersection> start_inter = 
		find_and_create_optional(elem, &create_topological_intersection, START_INTERSECTION, read_errors);

	boost::optional<GPlatesPropertyValues::GpmlTopologicalIntersection> end_inter = 
		find_and_create_optional(elem, &create_topological_intersection, END_INTERSECTION, read_errors);

	bool reverse_order = 
		find_and_create_one(elem, &create_boolean, REVERSE_ORDER, read_errors);

	return GPlatesPropertyValues::GpmlTopologicalLineSection::create(
		source_geometry, 
		start_inter,
		end_inter,
		reverse_order);
}

GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_topological_point(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = 
			GPlatesModel::PropertyName::create_gpml("TopologicalPoint"),
		SOURCE_GEOMETRY = 
			GPlatesModel::PropertyName::create_gpml("sourceGeometry");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type source_geometry = 
		find_and_create_one(elem, &create_property_delegate, SOURCE_GEOMETRY, read_errors);

	return GPlatesPropertyValues::GpmlTopologicalPoint::create(
		source_geometry);
}


GPlatesPropertyValues::GpmlTopologicalIntersection
GPlatesFileIO::PropertyCreationUtils::create_topological_intersection(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = 
			GPlatesModel::PropertyName::create_gpml("TopologicalIntersection"),
		INTERSECTION_GEOMETRY =
			GPlatesModel::PropertyName::create_gpml("intersectionGeometry"),
		REFERENCE_POINT = 
			GPlatesModel::PropertyName::create_gpml("referencePoint"),
		REFERENCE_POINT_PLATE_ID = 
			GPlatesModel::PropertyName::create_gpml("referencePointPlateId");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type intersection_geometry = 
		find_and_create_one(elem, &create_property_delegate, INTERSECTION_GEOMETRY, read_errors);

	GPlatesPropertyValues::GmlPoint::non_null_ptr_type reference_point = 
		find_and_create_one(elem, &create_point, REFERENCE_POINT, read_errors);

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type reference_point_plate_id = 
		find_and_create_one(elem, &create_property_delegate, REFERENCE_POINT_PLATE_ID, read_errors);

	return GPlatesPropertyValues::GpmlTopologicalIntersection(
		intersection_geometry, 
		reference_point,
		reference_point_plate_id);
}





GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_old_plates_header(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("OldPlatesHeader"),
		REGION_NUMBER = GPlatesModel::PropertyName::create_gpml("regionNumber"),
		REFERENCE_NUMBER = GPlatesModel::PropertyName::create_gpml("referenceNumber"),
		STRING_NUMBER = GPlatesModel::PropertyName::create_gpml("stringNumber"),
		GEOGRAPHIC_DESCRIPTION = GPlatesModel::PropertyName::create_gpml("geographicDescription"),
		PLATE_ID_NUMBER = GPlatesModel::PropertyName::create_gpml("plateIdNumber"),
		AGE_OF_APPEARANCE = GPlatesModel::PropertyName::create_gpml("ageOfAppearance"),
		AGE_OF_DISAPPEARANCE = GPlatesModel::PropertyName::create_gpml("ageOfDisappearance"),
		DATA_TYPE_CODE = GPlatesModel::PropertyName::create_gpml("dataTypeCode"),
		DATA_TYPE_CODE_NUMBER = GPlatesModel::PropertyName::create_gpml("dataTypeCodeNumber"),
		DATA_TYPE_CODE_NUMBER_ADDITIONAL = GPlatesModel::PropertyName::create_gpml("dataTypeCodeNumberAdditional"),
		CONJUGATE_PLATE_ID_NUMBER = GPlatesModel::PropertyName::create_gpml("conjugatePlateIdNumber"),
		COLOUR_CODE = GPlatesModel::PropertyName::create_gpml("colourCode"),
		NUMBER_OF_POINTS = GPlatesModel::PropertyName::create_gpml("numberOfPoints");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	unsigned int region_number = find_and_create_one(elem, &create_uint, REGION_NUMBER, read_errors);
	unsigned int reference_number = find_and_create_one(elem, &create_uint, REFERENCE_NUMBER, read_errors);
	unsigned int string_number = find_and_create_one(elem, &create_uint, STRING_NUMBER, read_errors);
	QString geographic_description = 
		find_and_create_one(elem, &create_string, GEOGRAPHIC_DESCRIPTION, read_errors);
	GPlatesModel::integer_plate_id_type
		plate_id_number = find_and_create_one(elem, &create_ulong, PLATE_ID_NUMBER, read_errors);
	double age_of_appearance = find_and_create_one(elem, &create_double, AGE_OF_APPEARANCE, read_errors);
	double age_of_disappearance = find_and_create_one(elem, &create_double, AGE_OF_DISAPPEARANCE, read_errors);
	QString data_type_code = find_and_create_one(elem, &create_string, DATA_TYPE_CODE, read_errors);
	unsigned int data_type_code_number = 
		find_and_create_one(elem, &create_uint, DATA_TYPE_CODE_NUMBER, read_errors);
	QString data_type_code_number_additional =
		find_and_create_one(elem, &create_string, DATA_TYPE_CODE_NUMBER_ADDITIONAL, read_errors);
	GPlatesModel::integer_plate_id_type conjugate_plate_id_number = 
		find_and_create_one(elem, &create_uint, CONJUGATE_PLATE_ID_NUMBER, read_errors);
	unsigned int colour_code = find_and_create_one(elem, &create_uint, COLOUR_CODE, read_errors);
	unsigned int number_of_points = find_and_create_one(elem, &create_uint, NUMBER_OF_POINTS, read_errors);

	return GPlatesPropertyValues::GpmlOldPlatesHeader::create(
			region_number, reference_number, string_number, 
			GPlatesUtils::make_icu_string_from_qstring(geographic_description),
			plate_id_number, age_of_appearance, age_of_disappearance, 
			GPlatesUtils::make_icu_string_from_qstring(data_type_code),
			data_type_code_number, 
			GPlatesUtils::make_icu_string_from_qstring(data_type_code_number_additional),
			conjugate_plate_id_number, colour_code, number_of_points);
}

GPlatesPropertyValues::GpmlKeyValueDictionaryElement
GPlatesFileIO::PropertyCreationUtils::create_key_value_dictionary_element(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("KeyValueDictionaryElement"),
		KEY = GPlatesModel::PropertyName::create_gpml("key"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		VALUE = GPlatesModel::PropertyName::create_gpml("value");


	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);
	GPlatesModel::PropertyValue::non_null_ptr_type 
		value = find_and_create_from_type(elem, type, VALUE, read_errors);
	GPlatesPropertyValues::XsString::non_null_ptr_type
		key = find_and_create_one(elem, &create_xs_string, KEY, read_errors);

	return GPlatesPropertyValues::GpmlKeyValueDictionaryElement(key, value, type);
}


GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_key_value_dictionary(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("KeyValueDictionary"),
		ELEMENT = GPlatesModel::PropertyName::create_gpml("element");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> elements;
	find_and_create_one_or_more(elem, &create_key_value_dictionary_element, ELEMENT, elements, read_errors);
	return GPlatesPropertyValues::GpmlKeyValueDictionary::create(elements);
}


GPlatesPropertyValues::GmlGridEnvelope::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_grid_envelope(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("GridEnvelope"),
		LOW = GPlatesModel::PropertyName::create_gml("low"),
		HIGH = GPlatesModel::PropertyName::create_gml("high");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<int> low = find_and_create_one(elem, &create_int_list, LOW, read_errors);
	std::vector<int> high = find_and_create_one(elem, &create_int_list, HIGH, read_errors);

	return GPlatesPropertyValues::GmlGridEnvelope::create(low, high);
}


GPlatesPropertyValues::GmlRectifiedGrid::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_rectified_grid(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("RectifiedGrid"),
		LIMITS = GPlatesModel::PropertyName::create_gml("limits"),
		AXIS_NAME = GPlatesModel::PropertyName::create_gml("axisName"),
		ORIGIN = GPlatesModel::PropertyName::create_gml("origin"),
		OFFSET_VECTOR = GPlatesModel::PropertyName::create_gml("offsetVector");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes(
			elem->attributes_begin(), elem->attributes_end());

	// <gml:limits>
	GmlGridEnvelope::non_null_ptr_type limits = find_and_create_one(elem, &create_grid_envelope, LIMITS, read_errors);

	// <gml:axisName>
	std::vector<XsString::non_null_ptr_type> non_const_axes;
	find_and_create_one_or_more(elem, &create_xs_string, AXIS_NAME, non_const_axes, read_errors);
	GmlRectifiedGrid::axes_list_type axes(non_const_axes.begin(), non_const_axes.end());

	// <gml:origin>
	GmlPoint::non_null_ptr_type origin = find_and_create_one(elem, &create_point, ORIGIN, read_errors);

	// <gml:offsetVector>
	GmlRectifiedGrid::offset_vector_list_type offset_vectors;
	find_and_create_one_or_more(elem, &create_double_list, OFFSET_VECTOR, offset_vectors, read_errors);

	return GmlRectifiedGrid::create(limits, axes, origin, offset_vectors, xml_attributes);
}


namespace
{
	template<typename T>
	boost::optional<GPlatesUtils::non_null_intrusive_ptr<const T> >
	to_optional_of_ptr_to_const(
			const boost::optional<GPlatesUtils::non_null_intrusive_ptr<T> > &opt)
	{
		if (opt)
		{
			return GPlatesUtils::non_null_intrusive_ptr<const T>(*opt);
		}
		else
		{
			return boost::none;
		}
	}
}


GPlatesPropertyValues::GmlFile::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_file(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("File"),
		RANGE_PARAMETERS = GPlatesModel::PropertyName::create_gml("rangeParameters"),
		FILE_NAME = GPlatesModel::PropertyName::create_gml("fileName"),
		FILE_STRUCTURE = GPlatesModel::PropertyName::create_gml("fileStructure"),
		MIME_TYPE = GPlatesModel::PropertyName::create_gml("mimeType"),
		COMPRESSION = GPlatesModel::PropertyName::create_gml("compression");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	// <gml:rangeParameters>
	GmlFile::composite_value_type range_parameters =
		find_and_create_one(elem, &create_gml_file_composite_value, RANGE_PARAMETERS, read_errors);
	
	// <gml:fileName>
	XsString::non_null_ptr_type file_name = find_and_create_one(elem, &create_xs_string, FILE_NAME, read_errors);

	// <gml:fileStructure>
	XsString::non_null_ptr_type file_structure = find_and_create_one(elem, &create_xs_string, FILE_STRUCTURE, read_errors);

	// <gml:mimeType>
	boost::optional<XsString::non_null_ptr_to_const_type> mime_type = to_optional_of_ptr_to_const(
			find_and_create_optional(elem, &create_xs_string, MIME_TYPE, read_errors));

	// <gml:compression>
	boost::optional<XsString::non_null_ptr_to_const_type> compression = to_optional_of_ptr_to_const(
			find_and_create_optional(elem, &create_xs_string, COMPRESSION, read_errors));

	return GmlFile::create(range_parameters, file_name, file_structure, mime_type, compression, &read_errors);
}


GPlatesPropertyValues::GpmlRasterBandNames::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_raster_band_names(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("RasterBandNames"),
		BAND_NAME = GPlatesModel::PropertyName::create_gpml("bandName");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<XsString::non_null_ptr_type> band_names;
        find_and_create_zero_or_more(elem, &create_xs_string, BAND_NAME, band_names, read_errors);

	// Check for uniqueness of band names.
	std::set<GPlatesUtils::UnicodeString> band_name_set;
	BOOST_FOREACH(const XsString::non_null_ptr_type &band_name, band_names)
	{
		if (!band_name_set.insert(band_name->value().get()).second)
		{
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::DuplicateRasterBandName,
					EXCEPTION_SOURCE);
		}
	}

	return GpmlRasterBandNames::create(band_names.begin(), band_names.end());
}


GPlatesPropertyValues::GpmlScalarField3DFile::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_scalar_field_3d_file(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("ScalarField3DFile"),
		FILE_NAME = GPlatesModel::PropertyName::create_gpml("fileName");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	const XsString::non_null_ptr_type filename =
        find_and_create_one(elem, &create_xs_string, FILE_NAME, read_errors);

	return GpmlScalarField3DFile::create(filename);
}


#if 0
GPlatesPropertyValues::GpmlArrayMember
GPlatesFileIO::PropertyCreationUtils::create_array_member(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("ArrayMember"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		VALUE = GPlatesModel::PropertyName::create_gpml("value");


	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);
	GPlatesModel::PropertyValue::non_null_ptr_type 
		value = find_and_create_from_type(elem, type, VALUE, read_errors);

	return GPlatesPropertyValues::GpmlArrayMember(value, type);
}
#endif

GPlatesPropertyValues::GpmlArray::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_array(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("Array"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		MEMBER = GPlatesModel::PropertyName::create_gpml("member");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		mem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(mem, &create_template_type_parameter_type, VALUE_TYPE, read_errors);

	std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> members;
	find_and_create_one_or_more_from_type(mem, type, MEMBER, members, read_errors);

	return GPlatesPropertyValues::GpmlArray::create(type,members);
}

