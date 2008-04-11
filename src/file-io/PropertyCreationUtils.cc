/* $Id$ */

/**
 * @file 
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
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "model/types.h"
#include "property-values/GmlLineString.h"
#include "property-values/TemplateTypeParameterType.h"
#include <boost/optional.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <iterator>
#include <QTextStream>


namespace
{
	template< typename T >
	bool
	parse_integral_value(
			T (QString::*parse_func)(bool *, int) const, 
			const QString &str, 
			boost::optional<T> &out)
	{
		// We always read numbers in base 10.
		static const int BASE = 10;

		bool success = false;
		T tmp = (str.*parse_func)(&success, BASE);

		if ( ! success) {
			std::cout << "Error reading integral value from \"" << str.toStdString() << "\"." << std::endl;
		} else {
			out = tmp;
		}
		return success;
	}


	template< typename T >
	bool
	parse_decimal_value(
			T (QString::*parse_func)(bool *) const, 
			const QString &str, 
			boost::optional<T> &out)
	{
		bool success = false;
		T tmp = (str.*parse_func)(&success);

		if ( ! success) {
			std::cout << "Error reading decimal value from \"" << str.toStdString() << "\"." << std::endl;
		} else {
			out = tmp;
		}
		return success;
	}


	template< typename T >
	boost::optional<T>
	find_and_create(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			boost::optional<T> (*creation_fn)(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem),
			const GPlatesModel::PropertyName &prop_name)
	{
		boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> target = 
			elem->get_child_by_name(prop_name);

		boost::optional<T> res;
		if (target) {
			res = (*creation_fn)(*target);
		}
		return res;
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
	};


	boost::optional<QString>
	create_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		TextExtractionVisitor visitor;
		std::for_each(
				elem->children_begin(), elem->children_end(),
				boost::bind(&GPlatesModel::XmlNode::accept_visitor, _1, boost::ref(visitor)));

		if (visitor.encountered_subelement() || !elem->attributes_empty()) {
			return boost::optional<QString>();
		}
		// Trim everything:
		return visitor.get_text().trimmed();
	}


	boost::optional<bool>
	create_boolean(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		static const QString TRUE_STR = "true";
		static const QString FALSE_STR = "false";

		boost::optional<bool> res;
		boost::optional<QString> str = create_string(elem);
		if ( ! str || str->isEmpty()) {
			return res;
		}

		if (str->compare(TRUE_STR, Qt::CaseInsensitive) == 0) {
			res = true;
		} else if (str->compare(FALSE_STR, Qt::CaseInsensitive) == 0) {
			res = false;
		} else {
			boost::optional<unsigned long> val;
			parse_integral_value(&QString::toULong, *str, val);
			if (val) {
			   	res = *val;
			}
		}
		return res;
	}


	boost::optional<double>
	create_double(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		boost::optional<double> res;
		boost::optional<QString> str = create_string(elem);
		if ( ! str || str->isEmpty()) {
			return res;
		}
		parse_decimal_value(&QString::toDouble, *str, res);
		return res;
	}


	boost::optional<unsigned long>
	create_ulong(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		boost::optional<unsigned long> res;
		boost::optional<QString> str = create_string(elem);
		if ( ! str || str->isEmpty()) {
			return res;
		}
		parse_integral_value(&QString::toULong, *str, res);
		return res;
	}


	boost::optional< GPlatesPropertyValues::TemplateTypeParameterType >
	create_template_type_parameter_type(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		boost::optional<QString> str = create_string(elem);
		if ( ! str || str->isEmpty()) {
			return boost::optional<GPlatesPropertyValues::TemplateTypeParameterType>();
		}
		QString alias = str->section(':', 0, 0);  // First chunk before a ':'.
		QString type = str->section(':', 1);  // The chunk after the ':'.
		boost::optional<QString> ns = elem->get_namespace_from_alias(alias);
		if ( ! ns) {
			return boost::optional<GPlatesPropertyValues::TemplateTypeParameterType>();
		}

		return GPlatesPropertyValues::TemplateTypeParameterType(*ns, alias, type);
	}


	boost::optional<int>
	create_int(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		boost::optional<int> res;
		boost::optional<QString> str = create_string(elem);
		if ( ! str || str->isEmpty()) {
			return res;
		}
		parse_integral_value(&QString::toInt, *str, res);
		return res;
	}


	boost::optional<unsigned int>
	create_uint(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		boost::optional<unsigned int> res;
		boost::optional<QString> str = create_string(elem);
		if ( ! str || str->isEmpty()) {
			return res;
		}
		parse_integral_value(&QString::toUInt, *str, res);
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


	boost::optional< GPlatesMaths::PointOnSphere >
	create_pos(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		typedef GPlatesMaths::PointOnSphere point_type;
		boost::optional<point_type> res;

		TextExtractionVisitor visitor;
		std::for_each(
				elem->children_begin(), elem->children_end(),
				boost::bind(&GPlatesModel::XmlNode::accept_visitor, _1, boost::ref(visitor)));
 
		if (visitor.encountered_subelement()) {
			return res;
		}

		QString str = visitor.get_text().trimmed();
		if (str.isEmpty()) {
			return res;
		}

		// XXX: Currently assuming srsDimension is 2!!

		QTextStream is(&str, QIODevice::ReadOnly);

		double lat = 0.0;
		double lon = 0.0;

		// FIXME: What should I do if one (or both) of these are screwed?
		is >> lat;
		// FIXME: Check is.status() here!
		is >> lon;

		if (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
				GPlatesMaths::LatLonPoint::is_valid_longitude(lon)) {
			res = GPlatesMaths::make_point_on_sphere(
						GPlatesMaths::LatLonPoint(lat, lon));
		}

		return res;
	}


	boost::optional< GPlatesMaths::PolylineOnSphere::non_null_ptr_type >
	create_polyline(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
	{
		typedef GPlatesMaths::PolylineOnSphere polyline_type;
		boost::optional<polyline_type::non_null_ptr_type> res;

		TextExtractionVisitor visitor;
		std::for_each(
				elem->children_begin(), elem->children_end(),
				boost::bind(&GPlatesModel::XmlNode::accept_visitor, _1, boost::ref(visitor)));
 
		if (visitor.encountered_subelement()) {
			return res;
		}

		QString str = visitor.get_text().trimmed();
		if (str.isEmpty()) {
			return res;
		}

		// XXX: Currently assuming srsDimension is 2!!

		std::vector<GPlatesMaths::PointOnSphere> points;
		points.reserve(estimate_number_of_points(str));

		QTextStream is(&str, QIODevice::ReadOnly);
		while ( ! is.atEnd() && (is.status() == QTextStream::Ok))
		{
			double lat = 0.0;
			double lon = 0.0;

			// FIXME: What should I do if one (or both) of these are screwed?
			is >> lat;
			// FIXME: Check is.status() here!
			is >> lon;

			if (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
					GPlatesMaths::LatLonPoint::is_valid_longitude(lon))
			{
				points.push_back(GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(lat,lon)));
			}
		}

		std::pair<
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator, 
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator>
				invalid_points;
		if (polyline_type::evaluate_construction_parameter_validity(points, invalid_points) 
				!= polyline_type::VALID)
		{
			// XXX: Report the problem!
			return res;
		}
		return polyline_type::create_on_heap(points);
	}


	/**
	 * This function will extract the single child of the given elem and return
	 * it.  If there is more than one child, the result will not be initialised.
	 */
	boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type>
	get_structural_type_element(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
			const GPlatesModel::PropertyName &prop_name)
	{
		if (elem->number_of_children() > 1) {
			return boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type>();
		}
		return elem->get_child_by_name(prop_name);
	}
}


boost::optional<GPlatesPropertyValues::XsBoolean::non_null_ptr_type>
GPlatesFileIO::PropertyCreationUtils::create_xs_boolean(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	boost::optional<bool> res = create_boolean(elem);
	boost::optional<GPlatesPropertyValues::XsBoolean::non_null_ptr_type> val;
	if (res) {
		val = GPlatesPropertyValues::XsBoolean::create(*res);
	}
	return val;
}


boost::optional<GPlatesPropertyValues::XsInteger::non_null_ptr_type>
GPlatesFileIO::PropertyCreationUtils::create_xs_integer(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	boost::optional<int> res = create_int(elem);
	boost::optional<GPlatesPropertyValues::XsInteger::non_null_ptr_type> val;
	if (res) {
		val = GPlatesPropertyValues::XsInteger::create(*res);
	}
	return val;
}


boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type>
GPlatesFileIO::PropertyCreationUtils::create_xs_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	boost::optional<QString> res = create_string(elem);
	boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type> val;
	if (res) {
		val = GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(*res));
	}
	return val;
}


boost::optional<GPlatesPropertyValues::XsDouble::non_null_ptr_type>
GPlatesFileIO::PropertyCreationUtils::create_xs_double(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	boost::optional<double> res = create_double(elem);
	boost::optional<GPlatesPropertyValues::XsDouble::non_null_ptr_type> val;
	if (res) {
		val = GPlatesPropertyValues::XsDouble::create(*res);
	}
	return val;
}


boost::optional<GPlatesModel::FeatureId>
GPlatesFileIO::PropertyCreationUtils::create_feature_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	boost::optional< GPlatesModel::FeatureId > id;
	boost::optional<QString> str = create_string(elem);

	if (str) {
		id = GPlatesModel::FeatureId(
			GPlatesUtils::make_icu_string_from_qstring(*str));
	}
	return id;
}


boost::optional<GPlatesModel::RevisionId>
GPlatesFileIO::PropertyCreationUtils::create_revision_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{	
	boost::optional<GPlatesModel::RevisionId> id;
	boost::optional<QString> str = create_string(elem);

	if (str) {
		id = GPlatesModel::RevisionId(
			GPlatesUtils::make_icu_string_from_qstring(*str));
	}
	return id;
}


boost::optional< GPlatesPropertyValues::GpmlRevisionId::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_gpml_revision_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	boost::optional<GPlatesModel::RevisionId> id = create_revision_id(elem);
	if ( ! id) {
		return boost::optional< GPlatesPropertyValues::GpmlRevisionId::non_null_ptr_type >();
	}

	return GPlatesPropertyValues::GpmlRevisionId::create(*id);
}


boost::optional< GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type > 
GPlatesFileIO::PropertyCreationUtils::create_plate_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	boost::optional<GPlatesModel::integer_plate_id_type> int_plate_id =
		create_ulong(elem);

	boost::optional< GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type > plate_id;
	if (int_plate_id && elem->attributes_empty()) {
		plate_id = GPlatesPropertyValues::GpmlPlateId::create(*int_plate_id);
	}
	return plate_id;
}


boost::optional< GPlatesPropertyValues::GeoTimeInstant >
GPlatesFileIO::PropertyCreationUtils::create_geo_time_instant(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	TextExtractionVisitor visitor;
	std::for_each(
			elem->children_begin(), elem->children_end(),
			boost::bind(&GPlatesModel::XmlNode::accept_visitor, _1, boost::ref(visitor)));

	boost::optional< GPlatesPropertyValues::GeoTimeInstant > time_instant;

	// FIXME:  Find and store the 'frame' attribute in the GeoTimeInstant.

	if (visitor.encountered_subelement()) {
		return time_instant;
	}

	QString text = visitor.get_text().trimmed();
	if (text.compare("http://gplates.org/times/distantFuture", Qt::CaseInsensitive) == 0) {
		return GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	}
	if (text.compare("http://gplates.org/times/distantPast", Qt::CaseInsensitive) == 0) {
		return GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
	}

	boost::optional<double> time;
	parse_decimal_value(&QString::toDouble, text, time);
	
	if (time) {
		time_instant = GPlatesPropertyValues::GeoTimeInstant(*time);
	}
	return time_instant;
}


boost::optional< GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type > 
GPlatesFileIO::PropertyCreationUtils::create_time_instant(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("TimeInstant"),
		TIME_POSITION = GPlatesModel::PropertyName::create_gml("timePosition");
	static const size_t N_PROPS = 1;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<GPlatesPropertyValues::GeoTimeInstant> time =
		find_and_create(elem, &create_geo_time_instant, TIME_POSITION);

	boost::optional< GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type > time_instant;

	// FIXME: The xml_attrs below should be read from the timePosition property.
	if (time && (elem->number_of_children() == N_PROPS) && elem->attributes_empty()) {
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
			xml_attrs(elem->attributes_begin(), elem->attributes_end());
		time_instant = GPlatesPropertyValues::GmlTimeInstant::create(*time, xml_attrs);
	}
	return time_instant;
}


boost::optional< GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type > 
GPlatesFileIO::PropertyCreationUtils::create_time_period(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("TimePeriod"),
		BEGIN_TIME = GPlatesModel::PropertyName::create_gml("begin"),
		END_TIME = GPlatesModel::PropertyName::create_gml("end");
	static const size_t N_PROPS = 2;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type>
		begin_time, end_time;

	begin_time = find_and_create(elem, &create_time_instant, BEGIN_TIME);
	end_time = find_and_create(elem, &create_time_instant, END_TIME);

	boost::optional< GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type > time_period;
	if (begin_time && end_time && (elem->number_of_children() == N_PROPS) 
			&& elem->attributes_empty()) {
		time_period = GPlatesPropertyValues::GmlTimePeriod::create(*begin_time, *end_time);
	}
	return time_period;
}


boost::optional< GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_constant_value(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("ConstantValue"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		VALUE = GPlatesModel::PropertyName::create_gpml("value"),
		DESCRIPTION = GPlatesModel::PropertyName::create_gpml("description");

	static const size_t N_PROPS = 3;
	static const size_t N_PROPS_WITHOUT_DESC = 2;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> type;
	boost::optional<QString> description;
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> value;

	description = find_and_create(elem, &create_string, DESCRIPTION);
	type = find_and_create(elem, &create_template_type_parameter_type, VALUE_TYPE);

	if (type) {
		StructurePropertyCreatorMap *map = StructurePropertyCreatorMap::instance();
		StructurePropertyCreatorMap::iterator iter = map->find(*type);

		// If we can create the given type...
		if (iter != map->end()) {
			boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> 
				value_element, target;

			// ... get the XML element for the type.
			target = elem->get_child_by_name(VALUE);
			// If the element is present...
			if (target && (*target)->attributes_empty() 
					&& ((*target)->number_of_children() == 1)) {
				value = (*iter->second)(*target);
			}
		}
	}

	boost::optional< GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type > const_val;
	if (value && type && description && (elem->number_of_children() == N_PROPS)
			&& elem->attributes_empty()) {
		const_val = GPlatesPropertyValues::GpmlConstantValue::create(*value, *type, 
				GPlatesUtils::make_icu_string_from_qstring(*description));
	} else if (value && type && (elem->number_of_children() == N_PROPS_WITHOUT_DESC)
			&& elem->attributes_empty()) {
		const_val = GPlatesPropertyValues::GpmlConstantValue::create(*value, *type);
	}
	return const_val;
}


boost::optional< GPlatesPropertyValues::GpmlTimeSample >
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

	static const size_t N_PROPS = 5;
	static const size_t N_PROPS_WITHOUT_DESC = 4;
	static const size_t N_PROPS_WITHOUT_DISABLED = 4;
	static const size_t N_PROPS_WITHOUT_DESC_OR_DISABLED = 3;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GpmlTimeSample >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> type;
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> value;
	boost::optional<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type> valid_time;
	boost::optional<QString> description;
	boost::optional<bool> is_disabled;

	type = find_and_create(elem, &create_template_type_parameter_type, VALUE_TYPE);
	valid_time = find_and_create(elem, &create_time_instant, VALID_TIME);
	description = find_and_create(elem, &create_string, DESCRIPTION);
	is_disabled = find_and_create(elem, &create_boolean, IS_DISABLED);

	if (type) {
		StructurePropertyCreatorMap *map = StructurePropertyCreatorMap::instance();
		StructurePropertyCreatorMap::iterator iter = map->find(*type);

		// If we can create the given type...
		if (iter != map->end()) {
			boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> 
				value_element, target;

			// ... get the XML element for the type.
			target = elem->get_child_by_name(VALUE);
			// If the element is present...
			if (target && (*target)->attributes_empty() 
					&& ((*target)->number_of_children() == 1)) {
				value = (*iter->second)(*target);
			}
		}
	}

	boost::intrusive_ptr<GPlatesPropertyValues::XsString> desc;
	if (description) {
		GPlatesPropertyValues::XsString::non_null_ptr_type tmp = 
			GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(*description));
		desc = GPlatesUtils::get_intrusive_ptr(tmp);
	}

	boost::optional< GPlatesPropertyValues::GpmlTimeSample > time_sample;
	if (value && type && description && valid_time && is_disabled 
			&& (elem->number_of_children() == N_PROPS) && elem->attributes_empty()) {
		time_sample = GPlatesPropertyValues::GpmlTimeSample(
				*value, *valid_time, desc, *type, *is_disabled);
	} else if (value && type && valid_time && is_disabled 
			&& (elem->number_of_children() == N_PROPS_WITHOUT_DESC) 
			&& elem->attributes_empty()) {
		time_sample = GPlatesPropertyValues::GpmlTimeSample(
				*value, *valid_time, desc, *type, *is_disabled);
	} else if (value && type && valid_time && description
			&& (elem->number_of_children() == N_PROPS_WITHOUT_DISABLED) 
			&& elem->attributes_empty()) {
		time_sample = GPlatesPropertyValues::GpmlTimeSample(
				*value, *valid_time, desc, *type);
	} else if (value && type && valid_time
			&& (elem->number_of_children() == N_PROPS_WITHOUT_DESC_OR_DISABLED)
			&& elem->attributes_empty()) {
		time_sample = GPlatesPropertyValues::GpmlTimeSample(
				*value, *valid_time, desc, *type);
	}
	return time_sample;
}


boost::optional< GPlatesPropertyValues::GpmlTimeWindow >
GPlatesFileIO::PropertyCreationUtils::create_time_window(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("TimeWindow"),
		TIME_DEPENDENT_PROPERTY_VALUE = 
			GPlatesModel::PropertyName::create_gpml("timeDependentPropertyValue"),
		VALID_TIME = GPlatesModel::PropertyName::create_gpml("validTime"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType");

	static const size_t N_PROPS = 3;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GpmlTimeWindow >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> time_dep_prop_val;
	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type> time_period;
	boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> type;

	time_dep_prop_val = find_and_create(elem, &create_time_dependent_property_value,
			TIME_DEPENDENT_PROPERTY_VALUE);
	time_period = find_and_create(elem, &create_time_period, VALID_TIME);
	type = find_and_create(elem, &create_template_type_parameter_type, VALUE_TYPE);

	boost::optional< GPlatesPropertyValues::GpmlTimeWindow > window;
	if (time_dep_prop_val && time_period && type
			&& elem->number_of_children() == N_PROPS && elem->attributes_empty()) {
		window = GPlatesPropertyValues::GpmlTimeWindow(
				*time_dep_prop_val, *time_period, *type);
	}
	return window;
}


boost::optional< GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type > 
GPlatesFileIO::PropertyCreationUtils::create_piecewise_aggregation(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("PiecewiseAggregation"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TIME_WINDOW = GPlatesModel::PropertyName::create_gpml("timeWindow");

	static const size_t MIN_N_PROPS = 1;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< 
			GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> type;
	boost::optional<GPlatesPropertyValues::GpmlTimeWindow> time_window;

	type = find_and_create(elem, &create_template_type_parameter_type, VALUE_TYPE);

	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows;

	GPlatesModel::XmlElementNode::child_const_iterator end = elem->children_end();
	std::pair<
		GPlatesModel::XmlElementNode::child_const_iterator,
		boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> >
			res = elem->get_next_child_by_name(TIME_WINDOW, elem->children_begin());

	boost::optional< GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type > 
		piecewise_aggr;

	while (res.first != end) 
	{
		time_window = create_time_window(*res.second);
		if ( ! time_window) {
			return piecewise_aggr;
		}
		time_windows.push_back(*time_window);
		res = elem->get_next_child_by_name(TIME_WINDOW, ++res.first);
	}

	if (type && (elem->number_of_children() >= (MIN_N_PROPS + time_windows.size())) 
			&& elem->attributes_empty()) {
		piecewise_aggr = GPlatesPropertyValues::GpmlPiecewiseAggregation::create(
				time_windows, *type);
	}
	return piecewise_aggr;
}


boost::optional< GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type > 
GPlatesFileIO::PropertyCreationUtils::create_irregular_sampling(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("IrregularSampling"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TIME_SAMPLE = GPlatesModel::PropertyName::create_gpml("timeSample"),
		INTERPOLATION_FUNCTION = GPlatesModel::PropertyName::create_gpml("interpolationFunction");

	static const size_t MIN_N_PROPS = 2;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> type;
	boost::optional<GPlatesPropertyValues::GpmlTimeSample> time_sample;
	boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type> 
		interp_func;

	type = find_and_create(elem, &create_template_type_parameter_type, VALUE_TYPE);
	interp_func = 
		find_and_create(elem, &create_interpolation_function, INTERPOLATION_FUNCTION);

	std::vector<GPlatesPropertyValues::GpmlTimeSample> time_samples;

	GPlatesModel::XmlElementNode::child_const_iterator end = elem->children_end();
	std::pair<
		GPlatesModel::XmlElementNode::child_const_iterator,
		boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> >
			res = elem->get_next_child_by_name(TIME_SAMPLE, elem->children_begin());

	boost::optional< GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type > 
		irreg_sampling;

	while (res.first != end) 
	{
		time_sample = create_time_sample(*res.second);
		if ( ! time_sample) {
			return irreg_sampling;
		}
		time_samples.push_back(*time_sample);
		res = elem->get_next_child_by_name(TIME_SAMPLE, ++res.first);
	}

	if (type && interp_func && ! time_samples.empty()
			&& (elem->number_of_children() >= MIN_N_PROPS) && elem->attributes_empty()) {
		irreg_sampling = GPlatesPropertyValues::GpmlIrregularSampling::create(
				time_samples, GPlatesUtils::get_intrusive_ptr(*interp_func), *type);
	} else if (type && ! time_samples.empty()
			&& (elem->number_of_children() >= MIN_N_PROPS) && elem->attributes_empty()) {
		irreg_sampling = GPlatesPropertyValues::GpmlIrregularSampling::create(
				time_samples, 
				GPlatesPropertyValues::GpmlInterpolationFunction::maybe_null_ptr_type(), 
				*type);
	}
	return irreg_sampling;
}


boost::optional< GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_hot_spot_trail_mark(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{	
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("HotSpotTrailMark"),
		POSITION = GPlatesModel::PropertyName::create_gpml("position"),
		TRAIL_WIDTH = GPlatesModel::PropertyName::create_gpml("trailWidth"),
		MEASURED_AGE = GPlatesModel::PropertyName::create_gpml("measuredAge"),
		MEASURED_AGE_RANGE = GPlatesModel::PropertyName::create_gpml("measuredAgeRange");
	static const size_t MIN_N_PROPS = 1;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< 
			GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional< GPlatesPropertyValues::GmlPoint::non_null_ptr_type > position;
	boost::optional< GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type > trail_width;
	boost::optional< GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type > measured_age;
	boost::optional< GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type > measured_age_range;

	position = find_and_create(elem, &create_point, POSITION);
	trail_width = find_and_create(elem, &create_measure, TRAIL_WIDTH);
	measured_age = find_and_create(elem, &create_time_instant, MEASURED_AGE);
	measured_age_range = find_and_create(elem, &create_time_period, MEASURED_AGE_RANGE);

	boost::optional< GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type > mark;
	if (position && (elem->number_of_children() >= MIN_N_PROPS)
			&& elem->attributes_empty()) {
		mark = GPlatesPropertyValues::GpmlHotSpotTrailMark::create(
				*position, trail_width, measured_age, measured_age_range);
	}
	return mark;
}


boost::optional< GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_measure(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{	
	boost::optional<double> quantity = create_double(elem);

	boost::optional< GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type > measure;
	if (quantity) {
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
			xml_attrs(elem->attributes_begin(), elem->attributes_end());
		measure = GPlatesPropertyValues::GpmlMeasure::create(*quantity, xml_attrs);
	}
	return measure;
}


boost::optional< GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_feature_reference(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("FeatureReference"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::PropertyName::create_gpml("targetFeature");
	static const size_t N_PROPS = 2;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> value_type;
	boost::optional<GPlatesModel::FeatureId> target_feature;

	value_type = find_and_create(elem, &create_template_type_parameter_type, VALUE_TYPE);
	target_feature = find_and_create(elem, &create_feature_id, TARGET_FEATURE);

	boost::optional< GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type > feature_ref;
	if (value_type && target_feature && (elem->number_of_children() == N_PROPS)
			&& elem->attributes_empty()) {
		feature_ref = 
			GPlatesPropertyValues::GpmlFeatureReference::create(*target_feature, *value_type);
	}
	return feature_ref;
}

boost::optional< GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type > 
GPlatesFileIO::PropertyCreationUtils::create_polarity_chron_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("PolarityChronId"),
		ERA   = GPlatesModel::PropertyName::create_gpml("era"),
		MAJOR = GPlatesModel::PropertyName::create_gpml("major"),
		MINOR = GPlatesModel::PropertyName::create_gpml("minor");
	
	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<QString> era;
	boost::optional<unsigned int> major_region;
	boost::optional<QString> minor_region;

	era = find_and_create(elem, &create_string, ERA);
	major_region = find_and_create(elem, &create_uint, MAJOR);
	minor_region = find_and_create(elem, &create_string, MINOR);

	boost::optional< GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type > 
		polarity_chron_id;
	if (elem->attributes_empty()) {
		polarity_chron_id =
		   	GPlatesPropertyValues::GpmlPolarityChronId::create(era, major_region, minor_region);
	}
	return polarity_chron_id;
}


boost::optional< GPlatesPropertyValues::GmlPoint::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_point(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("Point"),
		POS = GPlatesModel::PropertyName::create_gml("pos");
	static const size_t N_PROPS = 1;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GmlPoint::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional< GPlatesMaths::PointOnSphere > point;
	point = find_and_create(elem, &create_pos, POS);

	boost::optional< GPlatesPropertyValues::GmlPoint::non_null_ptr_type > gml_point;
	if (point && (elem->number_of_children() == N_PROPS)) {
		// FIXME: We need to give the srsName et al. attributes from the posList 
		// to the line string!
		gml_point = GPlatesPropertyValues::GmlPoint::create(*point);
	}
	return gml_point;
}


boost::optional< GPlatesPropertyValues::GmlLineString::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_line_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("LineString"),
		POS_LIST = GPlatesModel::PropertyName::create_gml("posList");
	static const size_t N_PROPS = 1;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GmlLineString::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional< GPlatesMaths::PolylineOnSphere::non_null_ptr_type > polyline;
	polyline = find_and_create(elem, &create_polyline, POS_LIST);

	boost::optional< GPlatesPropertyValues::GmlLineString::non_null_ptr_type > line_string;
	if (polyline && (elem->number_of_children() == N_PROPS)) {
		// FIXME: We need to give the srsName et al. attributes from the posList 
		// to the line string!
		line_string = GPlatesPropertyValues::GmlLineString::create(*polyline);
	}
	return line_string;
}


boost::optional< GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_orientable_curve(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName 
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gml("OrientableCurve"),
		BASE_CURVE = GPlatesModel::PropertyName::create_gml("baseCurve");
	static const size_t N_PROPS = 1;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional<GPlatesPropertyValues::GmlLineString::non_null_ptr_type> line_string;
	line_string = find_and_create(elem, &create_line_string, BASE_CURVE);

	boost::optional< GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type > curve;
	if (line_string && (elem->number_of_children() == N_PROPS)) {
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
			xml_attrs(elem->attributes_begin(), elem->attributes_end());
		curve = GPlatesPropertyValues::GmlOrientableCurve::create(*line_string, xml_attrs);
	}
	return curve;
}


boost::optional< GPlatesModel::PropertyValue::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_geometry(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static GPlatesModel::PropertyName
		POINT = GPlatesModel::PropertyName::create_gml("Point"),
		LINE_STRING = GPlatesModel::PropertyName::create_gml("LineString");
	
	if (parent->number_of_children() > 1) {
		return boost::optional< GPlatesModel::PropertyValue::non_null_ptr_type >();
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(POINT);
	if (structural_elem) {
		return boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>(
				create_point(*parent));
	}

	structural_elem = parent->get_child_by_name(LINE_STRING);
	if (structural_elem) {
		return boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>(
				create_line_string(*parent));
	}
	
	return boost::optional< GPlatesModel::PropertyValue::non_null_ptr_type >();
}


boost::optional< GPlatesModel::PropertyValue::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_time_dependent_property_value(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		CONSTANT_VALUE = GPlatesModel::PropertyName::create_gpml("ConstantValue"),
		IRREGULAR_SAMPLING = GPlatesModel::PropertyName::create_gpml("IrregularSampling"),
		PIECEWISE_AGGREGATION = GPlatesModel::PropertyName::create_gpml("PiecewiseAggregation");

	if (parent->number_of_children() > 1) {
		return boost::optional< GPlatesModel::PropertyValue::non_null_ptr_type >();
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(CONSTANT_VALUE);
	if (structural_elem) {
		return boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>(
				create_constant_value(*parent));
	}

	structural_elem = parent->get_child_by_name(IRREGULAR_SAMPLING);
	if (structural_elem) {
		return boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>(
				create_irregular_sampling(*parent));
	}

	structural_elem = parent->get_child_by_name(PIECEWISE_AGGREGATION);
	if (structural_elem) {
		return boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>(
				create_piecewise_aggregation(*parent));
	}
	return boost::optional< GPlatesModel::PropertyValue::non_null_ptr_type >();
}


boost::optional< GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_interpolation_function(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		FINITE_ROTATION_SLERP = GPlatesModel::PropertyName::create_gpml("FiniteRotationSlerp");

	if (parent->number_of_children() > 1) {
		return boost::optional< 
			GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type >();
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(FINITE_ROTATION_SLERP);
	if (structural_elem) {
		return boost::optional<
		   	GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type >(
				create_finite_rotation_slerp(*parent));
	}
	return boost::optional<
		GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type >();
}


boost::optional< GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_finite_rotation(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		AXIS_ANGLE_FINITE_ROTATION = 
			GPlatesModel::PropertyName::create_gpml("AxisAngleFiniteRotation"),
		ZERO_FINITE_ROTATION = GPlatesModel::PropertyName::create_gpml("ZeroFiniteRotation");

	if (parent->number_of_children() > 1) {
		return boost::optional< GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type >();
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;
   
	structural_elem = parent->get_child_by_name(AXIS_ANGLE_FINITE_ROTATION);
	if (structural_elem) {
		static const GPlatesModel::PropertyName
			EULER_POLE = GPlatesModel::PropertyName::create_gpml("eulerPole"),
			ANGLE = GPlatesModel::PropertyName::create_gpml("angle");
		static const size_t N_PROPS = 2;
		
		boost::optional< GPlatesPropertyValues::GmlPoint::non_null_ptr_type > euler_pole;
		boost::optional< GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type > angle;

		euler_pole = find_and_create(*structural_elem, &create_point, EULER_POLE);
		angle = find_and_create(*structural_elem, &create_measure, ANGLE);

		if (euler_pole && angle && ((*structural_elem)->number_of_children() == N_PROPS)
				&& (*structural_elem)->attributes_empty()) {
			return GPlatesPropertyValues::GpmlFiniteRotation::create(*euler_pole, *angle);
		} else {
			return boost::optional< 
				GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type >();
		}
	} 
	structural_elem = parent->get_child_by_name(ZERO_FINITE_ROTATION);
	if (structural_elem) {
		if (((*structural_elem)->number_of_children() == 0)
				&& ((*structural_elem)->attributes_empty())) {
			return GPlatesPropertyValues::GpmlFiniteRotation::create_zero_rotation();
		}
	}

	return boost::optional< 
		GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type >();
}


boost::optional< GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type >
GPlatesFileIO::PropertyCreationUtils::create_finite_rotation_slerp(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent)
{
	static const GPlatesModel::PropertyName
		STRUCTURAL_TYPE = GPlatesModel::PropertyName::create_gpml("FiniteRotationSlerp"),
		VALUE_TYPE = GPlatesModel::PropertyName::create_gpml("valueType");
	static const size_t N_PROPS = 1;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional<
			GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional< GPlatesPropertyValues::TemplateTypeParameterType > value_type;
	value_type = find_and_create(elem, &create_template_type_parameter_type, VALUE_TYPE);

	if (value_type && elem->number_of_children() == N_PROPS && elem->attributes_empty()) {
		return GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(*value_type);
	}
	return boost::optional< 
		GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type >();
}


boost::optional< GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type > 
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
	static const size_t N_PROPS = 13;

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > 
		structural_elem = get_structural_type_element(parent, STRUCTURAL_TYPE);
	if ( ! structural_elem) {
		return boost::optional< GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type >();
	}
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = *structural_elem;

	boost::optional< unsigned int > region_number;
	boost::optional< unsigned int > reference_number;
	boost::optional< unsigned int > string_number;
	boost::optional< QString > geographic_description;
	boost::optional< GPlatesModel::integer_plate_id_type > plate_id_number;
	boost::optional< double > age_of_appearance;
	boost::optional< double > age_of_disappearance;
	boost::optional< QString > data_type_code;
	boost::optional< unsigned int > data_type_code_number;
	boost::optional< QString > data_type_code_number_additional;
	boost::optional< GPlatesModel::integer_plate_id_type > conjugate_plate_id_number;
	boost::optional< unsigned int > colour_code;
	boost::optional< unsigned int > number_of_points;

	region_number = find_and_create(elem, &create_uint, REGION_NUMBER);
	reference_number = find_and_create(elem, &create_uint, REFERENCE_NUMBER);
	string_number = find_and_create(elem, &create_uint, STRING_NUMBER);
	geographic_description = find_and_create(elem, &create_string, GEOGRAPHIC_DESCRIPTION);
	plate_id_number = find_and_create(elem, &create_ulong, PLATE_ID_NUMBER);
	age_of_appearance = find_and_create(elem, &create_double, AGE_OF_APPEARANCE);
	age_of_disappearance = find_and_create(elem, &create_double, AGE_OF_DISAPPEARANCE);
	data_type_code = find_and_create(elem, &create_string, DATA_TYPE_CODE);
	data_type_code_number = find_and_create(elem, &create_uint, DATA_TYPE_CODE_NUMBER);
	data_type_code_number_additional =
		find_and_create(elem, &create_string, DATA_TYPE_CODE_NUMBER_ADDITIONAL);
	conjugate_plate_id_number = find_and_create(elem, &create_uint, CONJUGATE_PLATE_ID_NUMBER);
	colour_code = find_and_create(elem, &create_uint, COLOUR_CODE);
	number_of_points = find_and_create(elem, &create_uint, NUMBER_OF_POINTS);

	boost::optional< GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type > header;
	if (region_number && reference_number && string_number && geographic_description
				&& plate_id_number && age_of_appearance && age_of_disappearance
				&& data_type_code && data_type_code_number && data_type_code_number_additional
				&& conjugate_plate_id_number && colour_code && number_of_points
				&& elem->attributes_empty() && (elem->number_of_children() == N_PROPS)) {
		header = GPlatesPropertyValues::GpmlOldPlatesHeader::create(
				*region_number, *reference_number, *string_number, 
				GPlatesUtils::make_icu_string_from_qstring(*geographic_description),
				*plate_id_number, *age_of_appearance, *age_of_disappearance, 
				GPlatesUtils::make_icu_string_from_qstring(*data_type_code),
				*data_type_code_number, 
				GPlatesUtils::make_icu_string_from_qstring(*data_type_code_number_additional),
				*conjugate_plate_id_number, *colour_code, *number_of_points);
	}
	return header;
}

