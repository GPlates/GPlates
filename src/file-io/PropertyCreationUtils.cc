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
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "PropertyCreationUtils.h"
#include "StructurePropertyCreatorMap.h"
#include "GpmlReaderUtils.h"
#include "utils/UnicodeStringUtils.h"
#include "maths/LatLonPointConversions.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "model/types.h"
#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/TemplateTypeParameterType.h"
#include <boost/optional.hpp>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <iostream>
#include <iterator>
#include <QTextStream>
#include "global/GPlatesException.h"


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


	template< typename T >
	boost::optional<T>
	find_and_create_optional(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			T (*creation_fn)(const GPlatesModel::XmlElementNode::non_null_ptr_type &elem),
			const GPlatesModel::PropertyName &prop_name)
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

		return (*creation_fn)(target);  // Can throw.
	}


	template< typename T >
	T
	find_and_create_one(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			T (*creation_fn)(const GPlatesModel::XmlElementNode::non_null_ptr_type &elem),
			const GPlatesModel::PropertyName &prop_name)
	{
		boost::optional<T> res = find_and_create_optional(elem, creation_fn, prop_name);
		if ( ! res) {
			// Cound't find the property!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
					EXCEPTION_SOURCE);
		}
		return *res;
	}


	template< typename T, typename CollectionOfT>
	void
	find_and_create_zero_or_more(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			T (*creation_fn)(const GPlatesModel::XmlElementNode::non_null_ptr_type &elem),
			const GPlatesModel::PropertyName &prop_name,
			CollectionOfT &destination)
	{
		GPlatesModel::XmlElementNode::named_child_const_iterator 
			iter = elem->get_next_child_by_name(prop_name, elem->children_begin());

		while (iter.first != elem->children_end()) {
			GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;

			destination.push_back((*creation_fn)(target));  // creation_fn can throw.
			
			// Increment iter:
			iter = elem->get_next_child_by_name(prop_name, ++iter.first);
		}
	}


	template< typename T, typename CollectionOfT>
	void
	find_and_create_one_or_more(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			T (*creation_fn)(const GPlatesModel::XmlElementNode::non_null_ptr_type &elem),
			const GPlatesModel::PropertyName &prop_name,
			CollectionOfT &destination)
	{
		find_and_create_zero_or_more(elem, creation_fn, prop_name, destination);
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
			const GPlatesModel::PropertyName &prop_name)
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
#if 0
		if ( ! (target && (*target)->attributes_empty() 
				&& ((*target)->number_of_children() == 1))) {
			// Can't find target value!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::BadOrMissingTargetForValueType,
					EXCEPTION_SOURCE);
		}
#endif

		// Allow any number of children for string-types.

		static const GPlatesPropertyValues::TemplateTypeParameterType string_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("string");

		if ( ! (target && (*target)->attributes_empty() 
				&& (((*target)->number_of_children() == 1)) || (type == string_type) )) {

			// Can't find target value!
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::BadOrMissingTargetForValueType,
					EXCEPTION_SOURCE);
		}

		return (*iter->second)(*target);
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
	create_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
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
		// Trim everything:
		return visitor.get_text().trimmed();
	}


	QString
	create_nonempty_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		QString str = create_string(elem);
		if (str.isEmpty()) {
			// Unexpected empty string.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::UnexpectedEmptyString,
					EXCEPTION_SOURCE);
		}
		return str;
	}
	
	
	GPlatesPropertyValues::Enumeration::non_null_ptr_type
	create_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const UnicodeString &enum_type)
	{
		QString enum_value = create_nonempty_string(elem);
		return GPlatesPropertyValues::Enumeration::create(enum_type, 
				GPlatesUtils::make_icu_string_from_qstring(enum_value));
	}


	bool
	create_boolean(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		static const QString TRUE_STR = "true";
		static const QString FALSE_STR = "false";

		QString str = create_nonempty_string(elem);

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
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		QString str = create_nonempty_string(elem);

		double res = 0.0;
		if ( ! parse_decimal_value(&QString::toDouble, str, res)) {
			// Can't convert the string to a double.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidDouble,
					EXCEPTION_SOURCE);
		}
		return res;
	}


	unsigned long
	create_ulong(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		QString str = create_nonempty_string(elem);

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
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		QString str = create_nonempty_string(elem);

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
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		QString str = create_nonempty_string(elem);

		int res = 0;
		if ( ! parse_integral_value(&QString::toInt, str, res)) {
			// Can't convert the string to an int.
			throw GpmlReaderException(elem, GPlatesFileIO::ReadErrors::InvalidInt,
					EXCEPTION_SOURCE);
		}
		return res;
	}


	unsigned int
	create_uint(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		QString str = create_nonempty_string(elem);

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
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		QString str = create_nonempty_string(elem);

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


	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	create_polyline(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		typedef GPlatesMaths::PolylineOnSphere polyline_type;

		QString str = create_nonempty_string(elem);

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
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		typedef GPlatesMaths::PolygonOnSphere polygon_type;

		QString str = create_nonempty_string(elem);

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
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
	{
		static const GPlatesModel::PropertyName 
			STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("LinearRing"),
			POS_LIST = GPlatesModel::PropertyName::create_gml("posList");
	
		GPlatesModel::XmlElementNode::non_null_ptr_type
			elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
			polygon = find_and_create_one(elem, &create_polygon, POS_LIST);
	
		// FIXME: We need to give the srsName et al. attributes from the posList 
		// (or the gml:FeatureCollection tag?) to the GmlPolygon (or the FeatureCollection)!
		return polygon;
	}


	/**
	 * This function is used by create_point and create_gml_multi_point to do the
	 * common work of creating a GPlatesMaths::PointOnSphere.
	 */
	GPlatesMaths::PointOnSphere
	create_point_on_sphere(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
	{
		static const GPlatesModel::PropertyName
			STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("Point"),
			POS = GPlatesModel::PropertyName::create_gml("pos");

		GPlatesModel::XmlElementNode::non_null_ptr_type
			elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

		GPlatesMaths::PointOnSphere point = find_and_create_one(elem, &create_pos, POS);
		// FIXME: We need to give the srsName et al. attributes from the pos 
		// (or the gml:FeatureCollection tag?) to the GmlPoint or GmlMultiPoint.
		return point;
	}
}


GPlatesPropertyValues::XsBoolean::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_xs_boolean(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return GPlatesPropertyValues::XsBoolean::create(create_boolean(elem));
}


GPlatesPropertyValues::XsInteger::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_xs_integer(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return GPlatesPropertyValues::XsInteger::create(create_int(elem));
}


GPlatesPropertyValues::XsString::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_xs_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return GPlatesPropertyValues::XsString::create(
			GPlatesUtils::make_icu_string_from_qstring(create_string(elem)));
}


GPlatesPropertyValues::XsDouble::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_xs_double(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return GPlatesPropertyValues::XsDouble::create(create_double(elem));
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_absolute_reference_frame_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:AbsoluteReferenceFrameEnumeration");
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_continental_boundary_crust_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:ContinentalBoundaryCrustEnumeration");
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_continental_boundary_edge_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:ContinentalBoundaryEdgeEnumeration");
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_continental_boundary_side_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:ContinentalBoundarySideEnumeration");
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_dip_side_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:DipSideEnumeration");
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_dip_slip_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:DipSlipEnumeration");
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_fold_plane_annotation_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:FoldPlaneAnnotationEnumeration");
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_slip_component_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:SlipComponentEnumeration");
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_strike_slip_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:StrikeSlipEnumeration");
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_subduction_side_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return create_enumeration(elem, "gpml:SubductionSideEnumeration");
}


GPlatesModel::FeatureId
GPlatesFileIO::PropertyCreationUtils::create_feature_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return GPlatesModel::FeatureId(
		GPlatesUtils::make_icu_string_from_qstring(create_nonempty_string(elem)));
}


GPlatesModel::RevisionId
GPlatesFileIO::PropertyCreationUtils::create_revision_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{	
	return GPlatesModel::RevisionId(
		GPlatesUtils::make_icu_string_from_qstring(create_nonempty_string(elem)));
}


GPlatesPropertyValues::GpmlRevisionId::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gpml_revision_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return GPlatesPropertyValues::GpmlRevisionId::create(create_revision_id(elem));
}


GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_plate_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	return GPlatesPropertyValues::GpmlPlateId::create(create_ulong(elem));
}


GPlatesPropertyValues::GeoTimeInstant
GPlatesFileIO::PropertyCreationUtils::create_geo_time_instant(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	// FIXME:  Find and store the 'frame' attribute in the GeoTimeInstant.

	QString text = create_nonempty_string(elem);
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
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("TimeInstant"),
		TIME_POSITION = GPlatesModel::PropertyName::create_gml("timePosition");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GeoTimeInstant
		time = find_and_create_one(elem, &create_geo_time_instant, TIME_POSITION);

	// FIXME: The xml_attrs should be read from the timePosition property.
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
		xml_attrs(elem->attributes_begin(), elem->attributes_end());
	return GPlatesPropertyValues::GmlTimeInstant::create(time, xml_attrs);
}


GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_time_period(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("TimePeriod"),
		BEGIN_TIME = GPlatesModel::PropertyName::create_gml("begin"),
		END_TIME = GPlatesModel::PropertyName::create_gml("end");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		begin_time = find_and_create_one(elem, &create_time_instant, BEGIN_TIME), 
		end_time = find_and_create_one(elem, &create_time_instant, END_TIME);

	return GPlatesPropertyValues::GmlTimePeriod::create(begin_time, end_time);
}


GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_constant_value(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("ConstantValue"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		VALUE = GPlatesModel::PropertyName::create_gpml("value"),
		DESCRIPTION = GPlatesModel::PropertyName::create_gpml("description");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	boost::optional<QString> 
		description = find_and_create_optional(elem, &create_string, DESCRIPTION);
	GPlatesPropertyValues::TemplateTypeParameterType 
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);
	GPlatesModel::PropertyValue::non_null_ptr_type value = 
		find_and_create_from_type(elem, type, VALUE);

	if (description) {
		return GPlatesPropertyValues::GpmlConstantValue::create(value, type, 
				GPlatesUtils::make_icu_string_from_qstring(*description));
	}
	return GPlatesPropertyValues::GpmlConstantValue::create(value, type);
}


GPlatesPropertyValues::GpmlTimeSample
GPlatesFileIO::PropertyCreationUtils::create_time_sample(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("TimeSample"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		VALUE = GPlatesModel::PropertyName::create_gpml("value"),
		VALID_TIME = GPlatesModel::PropertyName::create_gpml("validTime"),
		DESCRIPTION = GPlatesModel::PropertyName::create_gpml("description"),
		IS_DISABLED = GPlatesModel::PropertyName::create_gpml("isDisabled");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);
	GPlatesModel::PropertyValue::non_null_ptr_type 
		value = find_and_create_from_type(elem, type, VALUE);
	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		valid_time = find_and_create_one(elem, &create_time_instant, VALID_TIME);
	boost::optional<QString>
		description = find_and_create_optional(elem, &create_string, DESCRIPTION);
	boost::optional<bool>
		is_disabled = find_and_create_optional(elem, &create_boolean, IS_DISABLED);

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
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
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
				TIME_DEPENDENT_PROPERTY_VALUE);
	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
		time_period = find_and_create_one(elem, &create_time_period, VALID_TIME);
	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);

	return GPlatesPropertyValues::GpmlTimeWindow(time_dep_prop_val, time_period, type);
}


GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_piecewise_aggregation(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("PiecewiseAggregation"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TIME_WINDOW = GPlatesModel::PropertyName::create_gpml("timeWindow");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);

	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows;
	find_and_create_zero_or_more(elem, &create_time_window, TIME_WINDOW, time_windows);

	return GPlatesPropertyValues::GpmlPiecewiseAggregation::create(time_windows, type);
}


GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_irregular_sampling(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("IrregularSampling"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TIME_SAMPLE = GPlatesModel::PropertyName::create_gpml("timeSample"),
		INTERPOLATION_FUNCTION = GPlatesModel::PropertyName::create_gpml("interpolationFunction");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);
	boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
		interp_func = find_and_create_optional(elem, &create_interpolation_function, 
				INTERPOLATION_FUNCTION);

	std::vector<GPlatesPropertyValues::GpmlTimeSample> time_samples;
	find_and_create_one_or_more(elem, &create_time_sample, TIME_SAMPLE, time_samples);

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
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
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
		position = find_and_create_one(elem, &create_point, POSITION);
	boost::optional< GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type >
		trail_width = find_and_create_optional(elem, &create_measure, TRAIL_WIDTH);
	boost::optional< GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type >
		measured_age = find_and_create_optional(elem, &create_time_instant, MEASURED_AGE);
	boost::optional< GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type >
		measured_age_range = find_and_create_optional(elem, &create_time_period, 
				MEASURED_AGE_RANGE);

	return GPlatesPropertyValues::GpmlHotSpotTrailMark::create(
			position, trail_width, measured_age, measured_age_range);
}


GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_measure(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{	
	double quantity = create_double(elem);

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
		xml_attrs(elem->attributes_begin(), elem->attributes_end());
	return GPlatesPropertyValues::GpmlMeasure::create(quantity, xml_attrs);
}


GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_feature_reference(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("FeatureReference"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::PropertyName::create_gpml("targetFeature");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		value_type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);
	GPlatesModel::FeatureId
		target_feature = find_and_create_one(elem, &create_feature_id, TARGET_FEATURE);

	return GPlatesPropertyValues::GpmlFeatureReference::create(target_feature, value_type);
}


GPlatesPropertyValues::GpmlFeatureSnapshotReference::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_feature_snapshot_reference(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("FeatureSnapshotReference"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::PropertyName::create_gpml("targetFeature"),
		TARGET_REVISION = GPlatesModel::PropertyName::create_gpml("targetRevision");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		value_type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);
	GPlatesModel::FeatureId
		target_feature = find_and_create_one(elem, &create_feature_id, TARGET_FEATURE);
	GPlatesModel::RevisionId
		target_revision = find_and_create_one(elem, &create_revision_id, TARGET_REVISION);

	return GPlatesPropertyValues::GpmlFeatureSnapshotReference::create(
			target_feature, target_revision, value_type);
}


GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_property_delegate(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("PropertyDelegate"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::PropertyName::create_gpml("targetFeature"),
		TARGET_PROPERTY = GPlatesModel::PropertyName::create_gpml("targetProperty");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		value_type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);
	GPlatesModel::FeatureId
		target_feature = find_and_create_one(elem, &create_feature_id, TARGET_FEATURE);
	GPlatesPropertyValues::TemplateTypeParameterType
		target_property = find_and_create_one(elem, &create_template_type_parameter_type, 
				TARGET_PROPERTY);

	GPlatesModel::PropertyName prop_name(target_property);
	return GPlatesPropertyValues::GpmlPropertyDelegate::create(
			target_feature, prop_name, value_type);
}


GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_polarity_chron_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("PolarityChronId"),
		ERA   = GPlatesModel::PropertyName::create_gpml("era"),
		MAJOR = GPlatesModel::PropertyName::create_gpml("major"),
		MINOR = GPlatesModel::PropertyName::create_gpml("minor");
	
	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	boost::optional<QString>
		era = find_and_create_optional(elem, &create_string, ERA);
	boost::optional<unsigned int>
		major_region = find_and_create_optional(elem, &create_uint, MAJOR);
	boost::optional<QString>
		minor_region = find_and_create_optional(elem, &create_string, MINOR);

	return GPlatesPropertyValues::GpmlPolarityChronId::create(era, major_region, minor_region);
}


GPlatesPropertyValues::GmlPoint::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_point(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	GPlatesMaths::PointOnSphere point = create_point_on_sphere(parent);

	// FIXME: We need to give the srsName et al. attributes from the posList 
	// to the line string!
	return GPlatesPropertyValues::GmlPoint::create(point);
}


GPlatesPropertyValues::GmlLineString::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_line_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("LineString"),
		POS_LIST = GPlatesModel::PropertyName::create_gml("posList");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		polyline = find_and_create_one(elem, &create_polyline, POS_LIST);

	// FIXME: We need to give the srsName et al. attributes from the posList 
	// to the line string!
	return GPlatesPropertyValues::GmlLineString::create(polyline);
}


GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gml_multi_point(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("MultiPoint"),
		POINT_MEMBER = GPlatesModel::PropertyName::create_gml("pointMember");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	// GmlMultiPoint has multiple gml:pointMember properties each containing a
	// single gml:Point.
	std::vector<GPlatesMaths::PointOnSphere> points;
	find_and_create_one_or_more(elem, &create_point_on_sphere, POINT_MEMBER, points);
	
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
		multipoint = GPlatesMaths::MultiPointOnSphere::create_on_heap(points);

	// FIXME: We need to give the srsName et al. attributes from the gml:Point
	// (or the gml:FeatureCollection tag?) to the GmlMultiPoint (or the FeatureCollection)!
	return GPlatesPropertyValues::GmlMultiPoint::create(multipoint);
}


GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_orientable_curve(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("OrientableCurve"),
		BASE_CURVE = GPlatesModel::PropertyName::create_gml("baseCurve");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GmlLineString::non_null_ptr_type
		line_string = find_and_create_one(elem, &create_line_string, BASE_CURVE);

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
		xml_attrs(elem->attributes_begin(), elem->attributes_end());
	return GPlatesPropertyValues::GmlOrientableCurve::create(line_string, xml_attrs);
}


GPlatesPropertyValues::GmlPolygon::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_gml_polygon(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("Polygon"),
		INTERIOR = GPlatesModel::PropertyName::create_gml("interior"),
		EXTERIOR = GPlatesModel::PropertyName::create_gml("exterior");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	// GmlPolygon has exactly one exterior gml:LinearRing
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		exterior = find_and_create_one(elem, &create_linear_ring, EXTERIOR);

	// GmlPolygon has zero or more interior gml:LinearRing
	std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> interiors;
	find_and_create_zero_or_more(elem, &create_linear_ring, INTERIOR, interiors);

	// FIXME: We need to give the srsName et al. attributes from the posList 
	// (or the gml:FeatureCollection tag?) to the GmlPolygon (or the FeatureCollection)!
	return GPlatesPropertyValues::GmlPolygon::create<
			std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> >(exterior, interiors);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_geometry(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
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
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_point(parent));
	}

	structural_elem = parent->get_child_by_name(LINE_STRING);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_line_string(parent));
	}

	structural_elem = parent->get_child_by_name(ORIENTABLE_CURVE);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_orientable_curve(parent));
	}
	
	structural_elem = parent->get_child_by_name(POLYGON);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_gml_polygon(parent));
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
		return GPlatesModel::PropertyValue::non_null_ptr_type(create_constant_value(parent));
#endif
	}

	// (Unknown) Invalid child!
	throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_time_dependent_property_value(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
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
				create_constant_value(parent));
	}

	structural_elem = parent->get_child_by_name(IRREGULAR_SAMPLING);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(
				create_irregular_sampling(parent));
	}

	structural_elem = parent->get_child_by_name(PIECEWISE_AGGREGATION);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(
				create_piecewise_aggregation(parent));
	}

	// Invalid child!
	throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_interpolation_function(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
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
				create_finite_rotation_slerp(parent));
	}

	// Invalid child!
	throw GpmlReaderException(parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_finite_rotation(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
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
			euler_pole = find_and_create_one(*structural_elem, &create_point, EULER_POLE);
		GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type
			angle = find_and_create_one(*structural_elem, &create_measure, ANGLE);

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
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("FiniteRotationSlerp"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		value_type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);

	return GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(value_type);
}


GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_old_plates_header(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
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

	unsigned int region_number = find_and_create_one(elem, &create_uint, REGION_NUMBER);
	unsigned int reference_number = find_and_create_one(elem, &create_uint, REFERENCE_NUMBER);
	unsigned int string_number = find_and_create_one(elem, &create_uint, STRING_NUMBER);
	QString geographic_description = 
		find_and_create_one(elem, &create_string, GEOGRAPHIC_DESCRIPTION);
	GPlatesModel::integer_plate_id_type
		plate_id_number = find_and_create_one(elem, &create_ulong, PLATE_ID_NUMBER);
	double age_of_appearance = find_and_create_one(elem, &create_double, AGE_OF_APPEARANCE);
	double age_of_disappearance = find_and_create_one(elem, &create_double, AGE_OF_DISAPPEARANCE);
	QString data_type_code = find_and_create_one(elem, &create_string, DATA_TYPE_CODE);
	unsigned int data_type_code_number = 
		find_and_create_one(elem, &create_uint, DATA_TYPE_CODE_NUMBER);
	QString data_type_code_number_additional =
		find_and_create_one(elem, &create_string, DATA_TYPE_CODE_NUMBER_ADDITIONAL);
	GPlatesModel::integer_plate_id_type conjugate_plate_id_number = 
		find_and_create_one(elem, &create_uint, CONJUGATE_PLATE_ID_NUMBER);
	unsigned int colour_code = find_and_create_one(elem, &create_uint, COLOUR_CODE);
	unsigned int number_of_points = find_and_create_one(elem, &create_uint, NUMBER_OF_POINTS);

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
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("KeyValueDictionaryElement"),
		KEY = GPlatesModel::PropertyName::create_gpml("key"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		VALUE = GPlatesModel::PropertyName::create_gpml("value");


	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::TemplateTypeParameterType
		type = find_and_create_one(elem, &create_template_type_parameter_type, VALUE_TYPE);
	GPlatesModel::PropertyValue::non_null_ptr_type 
		value = find_and_create_from_type(elem, type, VALUE);
	GPlatesPropertyValues::XsString::non_null_ptr_type
		key = find_and_create_one(elem, &create_xs_string, KEY);

	return GPlatesPropertyValues::GpmlKeyValueDictionaryElement(key, value, type);
}

GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
GPlatesFileIO::PropertyCreationUtils::create_key_value_dictionary(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("KeyValueDictionary"),
		ELEMENT = GPlatesModel::PropertyName::create_gpml("element");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> elements;
	find_and_create_one_or_more(elem, &create_key_value_dictionary_element, ELEMENT, elements);
//	find_and_create_one(elem, &create_key_value_dictionary_element, ELEMENTS, elements);
	return GPlatesPropertyValues::GpmlKeyValueDictionary::create(elements);
}

