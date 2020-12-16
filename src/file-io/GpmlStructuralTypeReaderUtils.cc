/* $Id$ */

/**
 * @file 
 * This file contains the implementation of the functions in the
 * GpmlStructuralTypeReaderUtils namespace.  You should read the documentation
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
#include <set>
#include <utility>
#include <vector>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QStringList>
#include <QTextStream>

#include "GpmlStructuralTypeReaderUtils.h"

#include "GpmlReaderException.h"
#include "GpmlReaderUtils.h"
#include "GpmlPropertyStructuralTypeReader.h"
#include "GpmlPropertyStructuralTypeReaderUtils.h"
#include "ReadErrorAccumulation.h"

#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"

#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/GpgimEnumerationType.h"
#include "model/GpgimVersion.h"
#include "model/types.h"
#include "model/XmlElementName.h"
#include "model/XmlNodeUtils.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/StructuralType.h"

#include "utils/Parse.h"
#include "utils/UnicodeStringUtils.h"

#define EXCEPTION_SOURCE BOOST_CURRENT_FUNCTION

namespace
{
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
			GPlatesFileIO::GpmlStructuralTypeReaderUtils::xml_attributes_type xml_attributes(
					elem->attributes_begin(), elem->attributes_end());
			d_result = GPlatesFileIO::GpmlStructuralTypeReaderUtils::value_component_type(
					GPlatesPropertyValues::ValueObjectType(elem->get_name()),
					xml_attributes);
		}

		const boost::optional<GPlatesFileIO::GpmlStructuralTypeReaderUtils::value_component_type> &
		get_result() const
		{
			return d_result;
		}

	private:

		boost::optional<GPlatesFileIO::GpmlStructuralTypeReaderUtils::value_component_type> d_result;
	};


	/**
	 * Common code used by 'create_point_on_sphere()', 'create_lon_lat_point_on_sphere' and
	 * 'create_point_2d'.
	 */
	template <class PointType>
	std::pair<PointType, GPlatesPropertyValues::GmlPoint::GmlProperty>
	create_point(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
			PointType (*create_pos_fn)(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &,
					const GPlatesModel::GpgimVersion &,
					GPlatesFileIO::ReadErrorAccumulation &),
			PointType (*create_coordinates_fn)(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &,
					const GPlatesModel::GpgimVersion &,
					GPlatesFileIO::ReadErrorAccumulation &),
			const GPlatesModel::GpgimVersion &gpml_version,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		static const GPlatesModel::XmlElementName
			STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("Point"),
			POS = GPlatesModel::XmlElementName::create_gml("pos"),
			COORDINATES = GPlatesModel::XmlElementName::create_gml("coordinates");

		GPlatesModel::XmlElementNode::non_null_ptr_type elem =
				GPlatesFileIO::GpmlStructuralTypeReaderUtils::get_structural_type_element(
						parent,
						STRUCTURAL_TYPE);

		// FIXME: We need to give the srsName et al. attributes from the pos 
		// (or the gml:FeatureCollection tag?) to the GmlPoint or GmlMultiPoint.
		boost::optional<PointType> point_as_pos =
				GPlatesFileIO::GpmlStructuralTypeReaderUtils::find_and_create_optional(
						elem, create_pos_fn, POS, gpml_version, read_errors);
		boost::optional<PointType> point_as_coordinates =
				GPlatesFileIO::GpmlStructuralTypeReaderUtils::find_and_create_optional(
						elem, create_coordinates_fn, COORDINATES, gpml_version, read_errors);

		// The gml:Point needs one of gml:pos and gml:coordinates, but not both.
		if (point_as_pos && point_as_coordinates)
		{
			throw GPlatesFileIO::GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::DuplicateProperty,
					EXCEPTION_SOURCE);
		}
		if (!point_as_pos && !point_as_coordinates)
		{
			throw GPlatesFileIO::GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
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
}


GPlatesModel::FeatureId
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_feature_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesModel::FeatureId(
			GPlatesUtils::make_icu_string_from_qstring(
					create_nonempty_string(elem, gpml_version, read_errors)));
}


GPlatesModel::RevisionId
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_revision_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{	
	return GPlatesModel::RevisionId(
			GPlatesUtils::make_icu_string_from_qstring(
					create_nonempty_string(elem, gpml_version, read_errors)));
}


GPlatesFileIO::GpmlStructuralTypeReaderUtils::composite_value_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gml_composite_value(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("CompositeValue"),
		VALUE_COMPONENT = GPlatesModel::XmlElementName::create_gml("valueComponent");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	composite_value_type result;
	find_and_create_zero_or_more(
			elem, &create_gml_value_component,
					VALUE_COMPONENT, result, gpml_version, read_errors);

	return result;
}


GPlatesPropertyValues::GmlGridEnvelope::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gml_grid_envelope(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("GridEnvelope"),
		LOW = GPlatesModel::XmlElementName::create_gml("low"),
		HIGH = GPlatesModel::XmlElementName::create_gml("high");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<int> low = find_and_create_one(elem, &create_int_list, LOW, gpml_version, read_errors);
	std::vector<int> high = find_and_create_one(elem, &create_int_list, HIGH, gpml_version, read_errors);

	return GPlatesPropertyValues::GmlGridEnvelope::create(low, high);
}


GPlatesFileIO::GpmlStructuralTypeReaderUtils::value_component_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gml_value_component(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	if (parent->number_of_children() > 1)
	{
		// Properties with multiple inline structural elements are not (yet) handled!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::NonUniqueStructuralElement,
				EXCEPTION_SOURCE);
	}
	else if (parent->number_of_children() == 0)
	{
		// Could not locate structural element template!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::StructuralElementNotFound,
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
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::StructuralElementNotFound,
				EXCEPTION_SOURCE);
	}
}


GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_finite_rotation_slerp(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("FiniteRotationSlerp"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::StructuralType value_type =
			find_and_create_one(elem, &create_template_type_parameter_type,
					VALUE_TYPE, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(value_type);
}


GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_interpolation_function(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		FINITE_ROTATION_SLERP = GPlatesModel::XmlElementName::create_gpml("FiniteRotationSlerp");

	if (parent->number_of_children() > 1) {
		// Too many children!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				EXCEPTION_SOURCE);
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(FINITE_ROTATION_SLERP);
	if (structural_elem) {
		return GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type(
				create_gpml_finite_rotation_slerp(parent, gpml_version, read_errors));
	}

	// Invalid child!
	throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
			parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesPropertyValues::GpmlKeyValueDictionaryElement
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_key_value_dictionary_element(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("KeyValueDictionaryElement"),
		KEY = GPlatesModel::XmlElementName::create_gpml("key"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType"),
		VALUE = GPlatesModel::XmlElementName::create_gpml("value");


	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::StructuralType
		type = find_and_create_one(elem, &create_template_type_parameter_type,
				VALUE_TYPE, gpml_version, read_errors);
	GPlatesModel::PropertyValue::non_null_ptr_type 
		value = find_and_create_from_type(elem, type,
				VALUE, structural_type_reader, gpml_version, read_errors);
	GPlatesPropertyValues::XsString::non_null_ptr_type
		key = find_and_create_one(elem,
				&GpmlPropertyStructuralTypeReaderUtils::create_xs_string,
				KEY, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlKeyValueDictionaryElement(key, value, type);
}


GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_property_delegate(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("PropertyDelegate"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::XmlElementName::create_gpml("targetFeature"),
		TARGET_PROPERTY = GPlatesModel::XmlElementName::create_gpml("targetProperty");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::StructuralType
		value_type = find_and_create_one(elem, &create_template_type_parameter_type,
				VALUE_TYPE, gpml_version, read_errors);
	GPlatesModel::FeatureId
		target_feature = find_and_create_one(elem, &create_feature_id,
				TARGET_FEATURE, gpml_version, read_errors);
	GPlatesPropertyValues::StructuralType
		target_property = find_and_create_one(elem, &create_template_type_parameter_type, 
				TARGET_PROPERTY, gpml_version, read_errors);

	GPlatesModel::PropertyName prop_name(target_property);
	return GPlatesPropertyValues::GpmlPropertyDelegate::create(
			target_feature, prop_name, value_type);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_time_dependent_property_value(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		CONSTANT_VALUE = GPlatesModel::XmlElementName::create_gpml("ConstantValue"),
		IRREGULAR_SAMPLING = GPlatesModel::XmlElementName::create_gpml("IrregularSampling"),
		PIECEWISE_AGGREGATION = GPlatesModel::XmlElementName::create_gpml("PiecewiseAggregation");

	if (parent->number_of_children() > 1) {
		// Too many children!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				EXCEPTION_SOURCE);
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(CONSTANT_VALUE);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(
				GpmlPropertyStructuralTypeReaderUtils::create_gpml_constant_value(
						parent, structural_type_reader, gpml_version, read_errors));
	}

	structural_elem = parent->get_child_by_name(IRREGULAR_SAMPLING);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(
				GpmlPropertyStructuralTypeReaderUtils::create_gpml_irregular_sampling(
						parent, structural_type_reader, gpml_version, read_errors));
	}

	structural_elem = parent->get_child_by_name(PIECEWISE_AGGREGATION);
	if (structural_elem) {
		return GPlatesModel::PropertyValue::non_null_ptr_type(
				GpmlPropertyStructuralTypeReaderUtils::create_gpml_piecewise_aggregation(
						parent, structural_type_reader, gpml_version, read_errors));
	}

	// Invalid child!
	throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
			parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesPropertyValues::GpmlTimeSample
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_time_sample(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("TimeSample"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType"),
		VALUE = GPlatesModel::XmlElementName::create_gpml("value"),
		VALID_TIME = GPlatesModel::XmlElementName::create_gpml("validTime"),
		DESCRIPTION = GPlatesModel::XmlElementName::create_gml("description"),
		IS_DISABLED = GPlatesModel::XmlElementName::create_gpml("isDisabled");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::StructuralType
		type = find_and_create_one(elem, &create_template_type_parameter_type,
				VALUE_TYPE, gpml_version, read_errors);
	GPlatesModel::PropertyValue::non_null_ptr_type 
		value = find_and_create_from_type(elem, type, VALUE,
				structural_type_reader, gpml_version, read_errors);
	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		valid_time = find_and_create_one(elem,
				&GpmlPropertyStructuralTypeReaderUtils::create_gml_time_instant,
				VALID_TIME, gpml_version, read_errors);
	boost::optional<QString>
		description = find_and_create_optional(elem, &create_string_without_trimming,
				DESCRIPTION, gpml_version, read_errors);
	boost::optional<bool>
		is_disabled = find_and_create_optional(elem, &create_boolean,
				IS_DISABLED, gpml_version, read_errors);

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
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_time_window(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("TimeWindow"),
		TIME_DEPENDENT_PROPERTY_VALUE = 
			GPlatesModel::XmlElementName::create_gpml("timeDependentPropertyValue"),
		VALID_TIME = GPlatesModel::XmlElementName::create_gpml("validTime"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesModel::PropertyValue::non_null_ptr_type
		time_dep_prop_val = find_and_create_one(elem, &create_gpml_time_dependent_property_value,
				TIME_DEPENDENT_PROPERTY_VALUE, structural_type_reader, gpml_version, read_errors);
	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
		time_period = find_and_create_one(elem,
				&GpmlPropertyStructuralTypeReaderUtils::create_gml_time_period,
				VALID_TIME, gpml_version, read_errors);
	GPlatesPropertyValues::StructuralType
		type = find_and_create_one(elem, &create_template_type_parameter_type,
				VALUE_TYPE, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlTimeWindow(time_dep_prop_val, time_period, type);
}


GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_topological_network_interior(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = 
			GPlatesModel::XmlElementName::create_gpml("TopologicalNetworkInterior"),
		SOURCE_GEOMETRY = 
			GPlatesModel::XmlElementName::create_gpml("sourceGeometry");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type source_geometry = 
		find_and_create_one(elem, &create_gpml_property_delegate,
				SOURCE_GEOMETRY, gpml_version, read_errors);

	return source_geometry;
}


GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_topological_line_section(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = 
			GPlatesModel::XmlElementName::create_gpml("TopologicalLineSection"),
		SOURCE_GEOMETRY = 
			GPlatesModel::XmlElementName::create_gpml("sourceGeometry"),
		REVERSE_ORDER = 
			GPlatesModel::XmlElementName::create_gpml("reverseOrder");


	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type source_geometry = 
		find_and_create_one(elem, &create_gpml_property_delegate,
				SOURCE_GEOMETRY, gpml_version, read_errors);

	bool reverse_order = 
		find_and_create_one(elem, &create_boolean, REVERSE_ORDER, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlTopologicalLineSection::create(
			source_geometry, 
			reverse_order);
}


GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_topological_point(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = 
			GPlatesModel::XmlElementName::create_gpml("TopologicalPoint"),
		SOURCE_GEOMETRY = 
			GPlatesModel::XmlElementName::create_gpml("sourceGeometry");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type source_geometry = 
		find_and_create_one(elem, &create_gpml_property_delegate,
				SOURCE_GEOMETRY, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlTopologicalPoint::create(
		source_geometry);
}


GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_topological_section(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		TOPOLOGICAL_LINE_SECTION = 
			GPlatesModel::XmlElementName::create_gpml("TopologicalLineSection"),
		TOPOLOGICAL_POINT = 
			GPlatesModel::XmlElementName::create_gpml("TopologicalPoint");

	if (parent->number_of_children() > 1) {
		// Too many children!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				EXCEPTION_SOURCE);
	}

	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem;

	structural_elem = parent->get_child_by_name(TOPOLOGICAL_LINE_SECTION);
	if (structural_elem) {
		return GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type(
				create_gpml_topological_line_section(parent, gpml_version, read_errors));
	}

	structural_elem = parent->get_child_by_name(TOPOLOGICAL_POINT);
	if (structural_elem) {
		return GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type(
				create_gpml_topological_point(parent, gpml_version, read_errors));
	}

	// Invalid child!
	throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
			parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
			EXCEPTION_SOURCE);
}


GPlatesModel::XmlElementNode::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::get_structural_type_element(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::XmlElementName &xml_element_name)
{
	if (elem->number_of_children() > 1)
	{
		// Properties with multiple inline structural elements are not (yet) handled!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::NonUniqueStructuralElement,
				EXCEPTION_SOURCE);
	}
	else if (elem->number_of_children() == 0)
	{
		// Could not locate a structural element.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::StructuralElementNotFound,
				EXCEPTION_SOURCE);
	}

	// Look for the structural type...
	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type >
		structural_elem = elem->get_child_by_name(xml_element_name);
	if (!structural_elem)
	{
		// Could not locate expected structural element!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::UnexpectedStructuralElement,
				EXCEPTION_SOURCE);
	}

	return structural_elem.get();
}


GPlatesFileIO::GpmlStructuralTypeReaderUtils::xml_attributes_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::get_xml_attributes_from_child(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::XmlElementName &xml_element_name)
{
	GPlatesModel::XmlElementNode::named_child_const_iterator
		iter = elem->get_next_child_by_name(xml_element_name, elem->children_begin());

	if (iter.first == elem->children_end())
	{
		// We didn't find the property, return empty map.
		return xml_attributes_type();
	}

	GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;
	return xml_attributes_type(target->attributes_begin(), target->attributes_end());
}


GPlatesPropertyValues::StructuralType 
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_template_type_parameter_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	QString str = create_nonempty_string(elem, gpml_version, read_errors);

	QString alias = str.section(':', 0, 0);  // First chunk before a ':'.
	QString type = str.section(':', 1);  // The chunk after the ':'.
	boost::optional<QString> ns = elem->get_namespace_from_alias(alias);
	if ( ! ns) {
		// Couldn't find the namespace alias.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, ReadErrors::MissingNamespaceAlias,
				EXCEPTION_SOURCE);
	}

	return GPlatesPropertyValues::StructuralType(*ns, alias, type);
}


boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::find_optional(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::XmlElementName &xml_element_name,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	GPlatesModel::XmlElementNode::named_child_const_iterator 
		iter = elem->get_next_child_by_name(xml_element_name, elem->children_begin());

	if (iter.first == elem->children_end())
	{
		// We didn't find the property, but that's okay here.
		return boost::none;
	}

	GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;

	// Increment iter:
	iter = elem->get_next_child_by_name(xml_element_name, ++iter.first);
	if (iter.first != elem->children_end())
	{
		// Found duplicate!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::DuplicateProperty,
				EXCEPTION_SOURCE);
	}

	return target;
}


GPlatesModel::XmlElementNode::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::find_one(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::XmlElementName &xml_element_name,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	GPlatesModel::XmlElementNode::named_child_const_iterator 
		iter = elem->get_next_child_by_name(xml_element_name, elem->children_begin());

	if (iter.first == elem->children_end())
	{
		// Couldn't find the property!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
				EXCEPTION_SOURCE);
	}

	GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;

	// Increment iter:
	iter = elem->get_next_child_by_name(xml_element_name, ++iter.first);
	if (iter.first != elem->children_end())
	{
		// Found duplicate!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::DuplicateProperty,
				EXCEPTION_SOURCE);
	}

	return target;
}


void
GPlatesFileIO::GpmlStructuralTypeReaderUtils::find_zero_or_more(
		std::vector<GPlatesModel::XmlElementNode::non_null_ptr_type> &targets,
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::XmlElementName &xml_element_name,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	GPlatesModel::XmlElementNode::named_child_const_iterator 
                    iter = elem->get_next_child_by_name(xml_element_name, elem->children_begin());

	while (iter.first != elem->children_end())
	{
		GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;

		targets.push_back(target);
		
		// Increment iter:
		iter = elem->get_next_child_by_name(xml_element_name, ++iter.first);
	}
}


void
GPlatesFileIO::GpmlStructuralTypeReaderUtils::find_one_or_more(
		std::vector<GPlatesModel::XmlElementNode::non_null_ptr_type> &targets,
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::XmlElementName &xml_element_name,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	GPlatesModel::XmlElementNode::named_child_const_iterator 
                    iter = elem->get_next_child_by_name(xml_element_name, elem->children_begin());

	if (iter.first == elem->children_end())
	{
		// Require at least one element!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
				EXCEPTION_SOURCE);
	}

	do
	{
		GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;

		targets.push_back(target);
		
		// Increment iter:
		iter = elem->get_next_child_by_name(xml_element_name, ++iter.first);
	}
	while (iter.first != elem->children_end());
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::find_and_create_from_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesPropertyValues::StructuralType &type,
		const GPlatesModel::XmlElementName &xml_element_name,
		const GPlatesFileIO::GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	boost::optional<GPlatesFileIO::GpmlPropertyStructuralTypeReader::structural_type_reader_function_type>
			structural_type_reader_function =
					structural_type_reader.get_structural_type_reader_function(type);
	if (!structural_type_reader_function)
	{
		// We can't create the given type!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::UnknownValueType,
				EXCEPTION_SOURCE);
	}

	boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> 
		target = elem->get_child_by_name(xml_element_name);


	// Allow any number of children for string-types.
	static const GPlatesPropertyValues::StructuralType string_type =
		GPlatesPropertyValues::StructuralType::create_xsi("string");

	if ( ! (target 
			&& (*target)->attributes_empty() 
			&& ( (*target)->number_of_children() == 1 || type == string_type)
			)) {

		// Can't find target value!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::BadOrMissingTargetForValueType,
				EXCEPTION_SOURCE);
	}

	return structural_type_reader_function.get()(*target, gpml_version, read_errors);
}


void
GPlatesFileIO::GpmlStructuralTypeReaderUtils::find_and_create_one_or_more_from_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesPropertyValues::StructuralType &type,
		const GPlatesModel::XmlElementName &xml_element_name,
		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &members,
		const GPlatesFileIO::GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	boost::optional<GPlatesFileIO::GpmlPropertyStructuralTypeReader::structural_type_reader_function_type>
			structural_type_reader_function =
					structural_type_reader.get_structural_type_reader_function(type);
	if (!structural_type_reader_function)
	{
		// We can't create the given type!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::UnknownValueType,
				EXCEPTION_SOURCE);
	}

	GPlatesModel::XmlElementNode::named_child_const_iterator
		iter = elem->get_next_child_by_name(xml_element_name, elem->children_begin());

	while (iter.first != elem->children_end()) {
		GPlatesModel::XmlElementNode::non_null_ptr_type target = *iter.second;

		//May need to check for attributes and number of children before adding to vector.
		// Note: creation function can throw.
		members.push_back(
				structural_type_reader_function.get()(target, gpml_version, read_errors));

		// Increment iter:
		iter = elem->get_next_child_by_name(xml_element_name, ++iter.first);
	}

}


QString
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_string_without_trimming(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	boost::optional<QString> text = GPlatesModel::XmlNodeUtils::get_text_without_trimming(elem);
	if (!text)
	{
		// String is wrong.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidString,
				EXCEPTION_SOURCE);
	}

	return text.get();
}


QString
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	return create_string_without_trimming(elem, gpml_version, read_errors).trimmed();
}


QString
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_nonempty_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	QString str = create_string(elem, gpml_version, read_errors);
	if (str.isEmpty()) {
		// Unexpected empty string.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::UnexpectedEmptyString,
				EXCEPTION_SOURCE);
	}
	return str;
}


GPlatesUtils::UnicodeString
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_unicode_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	return GPlatesUtils::make_icu_string_from_qstring(create_string(elem, gpml_version, read_errors));
}


bool
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_boolean(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	static const QString TRUE_STR = "true";
	static const QString FALSE_STR = "false";

	QString str = create_nonempty_string(elem, gpml_version, read_errors);

	if (str.compare(TRUE_STR, Qt::CaseInsensitive) == 0) {
		return true;
	} else if (str.compare(FALSE_STR, Qt::CaseInsensitive) == 0) {
		return false;
	}

	unsigned long val = 0;
	if ( ! parse_integral_value(&QString::toULong, str, val)) {
		// Can't convert the string to an integer.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidBoolean,
				EXCEPTION_SOURCE);
	}
	return val;
}


double
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_double(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	QString str = create_nonempty_string(elem, gpml_version, read_errors);

	double res = 0.0;
	if ( ! parse_decimal_value(&QString::toDouble, str, res)) {
		// Can't convert the string to a double.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidDouble,
				EXCEPTION_SOURCE);
	}
	return res;
}


std::vector<double>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_double_list(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	QStringList tokens =
			create_string(elem, gpml_version, read_errors).split(" ", QString::SkipEmptyParts);

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
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidDouble,
				EXCEPTION_SOURCE);
	}
}


unsigned long
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_ulong(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	QString str = create_nonempty_string(elem, gpml_version, read_errors);

	unsigned long res = 0;
	if ( ! parse_integral_value(&QString::toULong, str, res)) {
		// Can't convert the string to an unsigned long.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidUnsignedLong,
				EXCEPTION_SOURCE);
	}
	return res;
}


int
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_int(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	QString str = create_nonempty_string(elem, gpml_version, read_errors);

	int res = 0;
	if ( ! parse_integral_value(&QString::toInt, str, res)) {
		// Can't convert the string to an int.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidInt,
				EXCEPTION_SOURCE);
	}
	return res;
}


std::vector<int>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_int_list(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	QStringList tokens =
			create_string(elem, gpml_version, read_errors).split(" ", QString::SkipEmptyParts);

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
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidInt,
				EXCEPTION_SOURCE);
	}
}


unsigned int
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_uint(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	QString str = create_nonempty_string(elem, gpml_version, read_errors);

	unsigned int res = 0;
	if ( ! parse_integral_value(&QString::toUInt, str, res)) {
		// Can't convert the string to an unsigned int.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidUnsignedInt,
				EXCEPTION_SOURCE);
	}
	return res;
}


GPlatesMaths::PointOnSphere
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_pos(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	const std::pair<double, double> lon_lat_pos =
			create_lon_lat_pos(elem, gpml_version, read_errors);

	return GPlatesMaths::make_point_on_sphere(
			GPlatesMaths::LatLonPoint(
					lon_lat_pos.second/*lat*/,
					lon_lat_pos.first/*lon*/));
}


std::pair<double, double>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_lon_lat_pos(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	const std::pair<double, double> pos_2d =
			create_pos_2d(elem, gpml_version, read_errors);

	// FIXME: What should I do if one (or both) of these are screwed?
	// NOTE: We are assuming GPML is using (lat,lon) ordering.
	// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
	const double lat = pos_2d.first;
	// FIXME: Check is.status() here!
	const double lon = pos_2d.second;

	if ( ! (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
			GPlatesMaths::LatLonPoint::is_valid_longitude(lon))) {
		// Bad coordinates.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
				EXCEPTION_SOURCE);
	}
	return std::make_pair(lon, lat);
}


std::pair<double, double>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_pos_2d(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	QString str = create_nonempty_string(elem, gpml_version, read_errors);

	// XXX: Currently assuming srsDimension is 2!!

	QTextStream is(&str, QIODevice::ReadOnly);

	double x = 0.0;
	double y = 0.0;

	is >> x;
	is >> y;

	return std::make_pair(x, y);
}


GPlatesMaths::PointOnSphere
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_coordinates(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	const std::pair<double, double> lon_lat_coordinates =
			create_lon_lat_coordinates(elem, gpml_version, read_errors);

	return GPlatesMaths::make_point_on_sphere(
			GPlatesMaths::LatLonPoint(
					lon_lat_coordinates.second/*lat*/,
					lon_lat_coordinates.first/*lon*/));
}


std::pair<double, double>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_lon_lat_coordinates(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	const std::pair<double, double> coordinates_2d =
			create_coordinates_2d(elem, gpml_version, read_errors);

	// FIXME: What should I do if one (or both) of these are screwed?
	// NOTE: We are assuming GPML is using (lat,lon) ordering.
	// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
	const double lat = coordinates_2d.first;
	// FIXME: Check is.status() here!
	const double lon = coordinates_2d.second;

	if ( ! (GPlatesMaths::LatLonPoint::is_valid_latitude(lat) &&
			GPlatesMaths::LatLonPoint::is_valid_longitude(lon))) {
		// Bad coordinates.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
				EXCEPTION_SOURCE);
	}
	return std::make_pair(lon, lat);
}


std::pair<double, double>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_coordinates_2d(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	QString str = create_nonempty_string(elem, gpml_version, read_errors);

	// XXX: Currently assuming srsDimension is 2!!

	QStringList tokens = str.split(",");

	double x = 0.0;
	double y = 0.0;

	if (tokens.count() == 2)
	{
		try
		{
			GPlatesUtils::Parse<double> parse;
			x = parse(tokens.at(0));
			y = parse(tokens.at(1));
		}
		catch (...)
		{
			// Do nothing, fall through.
		}
	}

	return std::make_pair(x, y);
}


GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_polyline(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	typedef GPlatesMaths::PolylineOnSphere polyline_type;

	QString str = create_nonempty_string(elem, gpml_version, read_errors);

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
			throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
					EXCEPTION_SOURCE);
		}
		points.push_back(GPlatesMaths::make_point_on_sphere(
					GPlatesMaths::LatLonPoint(lat,lon)));
	}

	// We want to return a different ReadError Description for each possible return
	// value of evaluate_construction_parameter_validity().
	polyline_type::ConstructionParameterValidity polyline_validity =
			polyline_type::evaluate_construction_parameter_validity(points);
	switch (polyline_validity)
	{
	case polyline_type::VALID:
			// All good.
			break;
	
	case polyline_type::INVALID_INSUFFICIENT_DISTINCT_POINTS:
			// Not enough points to make even a single (valid) line segment.
			throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::InsufficientDistinctPointsInPolyline,
					EXCEPTION_SOURCE);
			break;
			
	case polyline_type::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
			// Segments of a polyline cannot be defined between two points which are antipodal.
			throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::AntipodalAdjacentPointsInPolyline,
					EXCEPTION_SOURCE);
			break;

	default:
			// Incompatible points encountered! For no defined reason!
			throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::InvalidPointsInPolyline,
					EXCEPTION_SOURCE);
			break;
	}
	return polyline_type::create(points);
}


boost::shared_ptr< std::vector<GPlatesMaths::PointOnSphere> >
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_polygon_ring(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	typedef GPlatesMaths::PolygonOnSphere polygon_type;

	QString str = create_nonempty_string(elem, gpml_version, read_errors);

	// XXX: Currently assuming srsDimension is 2!!

	boost::shared_ptr< std::vector<GPlatesMaths::PointOnSphere> > ring_points(
			new std::vector<GPlatesMaths::PointOnSphere>());
	ring_points->reserve(estimate_number_of_points(str));

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
			throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
					EXCEPTION_SOURCE);
		}
		ring_points->push_back(
				GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(lat,lon)));
	}

	// There should be at least 3 points in a polygon.
	if (ring_points->size() < 3)
	{
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InsufficientPointsInPolygon,
				EXCEPTION_SOURCE);
	}

	// GML Polygons require the first and last points of a polygon to be identical,
	// because the format wasn't verbose enough. GPlates expects that the first
	// and last points of a PolygonOnSphere are implicitly joined.
	// If the first and last points are the same then we'll remove the last point
	// (provided that leaves us with at least 3 points for the polygon).
	if (ring_points->size() >= 4)
	{
		const GPlatesMaths::PointOnSphere &p1 = *(ring_points->begin());
		const GPlatesMaths::PointOnSphere &p2 = *(--ring_points->end());
		if (p1 == p2)
		{
			ring_points->pop_back();
		}
	}

	// We want to return a different ReadError Description for each possible return
	// value of evaluate_construction_parameter_validity().
	polygon_type::ConstructionParameterValidity polygon_validity =
			polygon_type::evaluate_construction_parameter_validity(*ring_points);
	switch (polygon_validity)
	{
	case polygon_type::VALID:
			// All good.
			break;
	
	case polygon_type::INVALID_INSUFFICIENT_DISTINCT_POINTS:
			// Less good - not enough points, although we have already checked for
			// this earlier in the function. So it must be a problem with coincident points.
			throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::InsufficientDistinctPointsInPolygon,
					EXCEPTION_SOURCE);
			break;
			
	case polygon_type::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
			// Segments of a polygon cannot be defined between two points which are antipodal.
			throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::AntipodalAdjacentPointsInPolygon,
					EXCEPTION_SOURCE);
			break;

	default:
			// Incompatible points encountered! For no defined reason!
			throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::InvalidPointsInPolygon,
					EXCEPTION_SOURCE);
			break;
	}

	return ring_points;
}


boost::shared_ptr< std::vector<GPlatesMaths::PointOnSphere> >
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_linear_ring(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("LinearRing"),
		POS_LIST = GPlatesModel::XmlElementName::create_gml("posList");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	boost::shared_ptr< std::vector<GPlatesMaths::PointOnSphere> >
		ring_points = find_and_create_one(elem, &create_polygon_ring, POS_LIST, gpml_version, read_errors);

	return ring_points;
}


std::pair<GPlatesMaths::PointOnSphere, GPlatesPropertyValues::GmlPoint::GmlProperty>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_point_on_sphere(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	return create_point(
			parent,
			&create_pos,
			&create_coordinates,
			gpml_version,
			read_errors);
}


std::pair<std::pair<double, double>, GPlatesPropertyValues::GmlPoint::GmlProperty>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_lon_lat_point_on_sphere(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	return create_point(
			parent,
			&create_lon_lat_pos,
			&create_lon_lat_coordinates,
			gpml_version,
			read_errors);
}


std::pair<std::pair<double, double>, GPlatesPropertyValues::GmlPoint::GmlProperty>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_point_2d(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	return create_point(
			parent,
			&create_pos_2d,
			&create_coordinates_2d,
			gpml_version,
			read_errors);
}


GPlatesPropertyValues::GeoTimeInstant
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_geo_time_instant(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	// FIXME:  Find and store the 'frame' attribute in the GeoTimeInstant.

	QString text = create_nonempty_string(elem, gpml_version, read_errors);
	if (text.compare("http://gplates.org/times/distantFuture", Qt::CaseInsensitive) == 0) {
		return GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	}
	if (text.compare("http://gplates.org/times/distantPast", Qt::CaseInsensitive) == 0) {
		return GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
	}

	double time = 0.0;
	if ( ! parse_decimal_value(&QString::toDouble, text, time)) {
		// Can't convert the string to a geo time.
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				elem, GPlatesFileIO::ReadErrors::InvalidGeoTime,
				EXCEPTION_SOURCE);
	}
	return GPlatesPropertyValues::GeoTimeInstant(time);
}


std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_topological_sections(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("TopologicalSections"),
		SECTION = GPlatesModel::XmlElementName::create_gpml("section");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> topological_sections;
	find_and_create_one_or_more(
			elem,
			&GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_gpml_topological_section,
			SECTION,
			topological_sections,
			gpml_version,
			read_errors);

	return topological_sections;
}


std::vector<GPlatesFileIO::GpmlStructuralTypeReaderUtils::coordinate_list_type>
GPlatesFileIO::GpmlStructuralTypeReaderUtils::create_tuple_list(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	const QStringList comma_separated_tokens = create_string(parent, gpml_version, read_errors).split(",");

	// If there are no commas then there is only one list in the tuple.
	const unsigned int num_comma_separated_tokens = comma_separated_tokens.size();
	if (num_comma_separated_tokens == 1)
	{
		const std::vector<double> double_list = create_double_list(parent, gpml_version, read_errors);
		return std::vector<coordinate_list_type>(1, double_list);
	}

	// Determine the number of lists in the tuple.
	unsigned int num_lists = 1;
	for ( ; num_lists < num_comma_separated_tokens; ++num_lists)
	{
		QStringList current_token_numbers =
				comma_separated_tokens[num_lists].trimmed().split(" ", QString::SkipEmptyParts);
		if (current_token_numbers.size() != 1)
		{
			++num_lists;
			break;
		}
	}

	// The number of elements in each list - all lists (should) have same number of elements.
	// Division-by-zero is not possible since 'num_lists' will always be greater than one.
	const unsigned int list_size = (num_comma_separated_tokens - 1) / (num_lists - 1);

	// The total number of comma-separated tokens must be appropriate for the number of lists.
	if (list_size * (num_lists - 1) + 1 != num_comma_separated_tokens)
	{
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::InvalidTupleList,
				EXCEPTION_SOURCE);
	}

	// Reserve space in the lists.
	std::vector<coordinate_list_type> tuple_list(num_lists);
	for (unsigned int list_index = 0; list_index < num_lists; ++list_index)
	{
		tuple_list.reserve(list_size);
	}

	// Parse the lists.
	GPlatesUtils::Parse<double> parse;
	unsigned int comma_separated_token_index = 0;
	boost::optional<QString> next_element_in_first_list;
	for (unsigned int list_element_index = 0; list_element_index < list_size; ++list_element_index)
	{
		for (unsigned int list_index = 0; list_index < num_lists; ++list_index)
		{
			if (list_index == 0 &&
				next_element_in_first_list)
			{
				// Second, or third, etc, element in first list (not first element).
				// This was parsed as part of the previous element in the last list.
				const QString &list_element = next_element_in_first_list.get();
				try
				{
					tuple_list[0].push_back(parse(list_element));
				}
				catch (const GPlatesUtils::ParseError &)
				{
					throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
							parent, GPlatesFileIO::ReadErrors::InvalidTupleList,
							EXCEPTION_SOURCE);
				}

				// We're not parsing the current comma-separated token.
				continue;
			}

			QStringList current_token_numbers =
					comma_separated_tokens[comma_separated_token_index].trimmed().split(" ", QString::SkipEmptyParts);
			++comma_separated_token_index;

			const QString list_element = current_token_numbers[0];

			// The comma-separated tokens associated with elements of the last list
			// always have two tokens (separated by whitespace) except for the last element.
			if (list_index == num_lists - 1 &&
				list_element_index < list_size - 1)
			{
				if (current_token_numbers.size() != 2)
				{
					throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
							parent, GPlatesFileIO::ReadErrors::InvalidTupleList,
							EXCEPTION_SOURCE);
				}
				next_element_in_first_list = current_token_numbers[1];
			}
			else
			{
				if (current_token_numbers.size() != 1)
				{
					throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
							parent, GPlatesFileIO::ReadErrors::InvalidTupleList,
							EXCEPTION_SOURCE);
				}
			}

			try
			{
				tuple_list[list_index].push_back(parse(list_element));
			}
			catch (const GPlatesUtils::ParseError &)
			{
				throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
						parent, GPlatesFileIO::ReadErrors::InvalidTupleList,
						EXCEPTION_SOURCE);
			}
		}
	}

	return tuple_list;
}
