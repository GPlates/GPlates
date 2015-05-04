/* $Id$ */

/**
 * @file 
 * This file contains the implementation of the functions in the
 * GpmlPropertyStructuralTypeReaderUtils namespace.  You should read the documentation
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

#include "GpmlPropertyStructuralTypeReaderUtils.h"

#include "GpmlReaderException.h"
#include "GpmlReaderUtils.h"
#include "GpmlPropertyStructuralTypeReader.h"
#include "GpmlStructuralTypeReaderUtils.h"
#include "ReadErrorAccumulation.h"

#include "global/GPlatesException.h"

#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureType.h"
#include "model/GpgimEnumerationType.h"
#include "model/GpgimVersion.h"
#include "model/Metadata.h"
#include "model/types.h"
#include "model/XmlElementName.h"
#include "model/XmlNodeUtils.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlDataBlockCoordinateList.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlTotalReconstructionPole.h"
#include "property-values/StructuralType.h"
#include "property-values/ValueObjectType.h"

#include "utils/Parse.h"
#include "utils/UnicodeStringUtils.h"

#define EXCEPTION_SOURCE BOOST_CURRENT_FUNCTION


using namespace GPlatesFileIO::GpmlStructuralTypeReaderUtils;


//
// Please keep these ordered alphabetically (within the XSI, GML and GPML groups)...
//


GPlatesPropertyValues::XsBoolean::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_xs_boolean(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::XsBoolean::create(create_boolean(elem, gpml_version, read_errors));
}


GPlatesPropertyValues::XsDouble::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_xs_double(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::XsDouble::create(create_double(elem, gpml_version, read_errors));
}


GPlatesPropertyValues::XsInteger::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_xs_integer(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::XsInteger::create(create_int(elem, gpml_version, read_errors));
}


GPlatesPropertyValues::XsString::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_xs_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::XsString::create(
			GPlatesUtils::make_icu_string_from_qstring(create_string(elem, gpml_version, read_errors)));
}


GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_data_block(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("DataBlock"),
		RANGE_PARAMETERS = GPlatesModel::XmlElementName::create_gml("rangeParameters"),
		TUPLE_LIST = GPlatesModel::XmlElementName::create_gml("tupleList");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	// <gml:rangeParameters>
	composite_value_type range_parameters =
		find_and_create_one(elem, &create_gml_composite_value,
				RANGE_PARAMETERS, gpml_version, read_errors);

	// <gml:tupleList>
	std::vector<coordinate_list_type> tuple_lists =
			find_and_create_one(
					elem, &create_tuple_list, TUPLE_LIST, gpml_version, read_errors);

	if (range_parameters.size() != tuple_lists.size())
	{
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::MismatchingRangeParametersSizeAndTupleSize,
				EXCEPTION_SOURCE);
	}

	GmlDataBlock::non_null_ptr_type gml_data_block = GmlDataBlock::create();

	for (unsigned int tuple_list_index = 0; tuple_list_index < tuple_lists.size(); ++tuple_list_index)
	{
		coordinate_list_type &tuple_list = tuple_lists[tuple_list_index];
		const value_component_type &value_component = range_parameters[tuple_list_index];

		GmlDataBlockCoordinateList::non_null_ptr_type gml_data_block_coordinate_list =
				GmlDataBlockCoordinateList::create_swap(
						value_component.first,
						value_component.second,
						tuple_list);

		gml_data_block->tuple_list_push_back(gml_data_block_coordinate_list);
	}

	return gml_data_block;
}


GPlatesPropertyValues::GmlFile::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_file(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("File"),
		RANGE_PARAMETERS = GPlatesModel::XmlElementName::create_gml("rangeParameters"),
		FILE_NAME = GPlatesModel::XmlElementName::create_gml("fileName"),
		FILE_STRUCTURE = GPlatesModel::XmlElementName::create_gml("fileStructure"),
		MIME_TYPE = GPlatesModel::XmlElementName::create_gml("mimeType"),
		COMPRESSION = GPlatesModel::XmlElementName::create_gml("compression");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	// <gml:rangeParameters>
	composite_value_type range_parameters =
		find_and_create_one(elem, &create_gml_composite_value,
				RANGE_PARAMETERS, gpml_version, read_errors);
	
	// <gml:fileName>
	XsString::non_null_ptr_type file_name =
			find_and_create_one(elem, &create_xs_string, FILE_NAME, gpml_version, read_errors);

	// <gml:fileStructure>
	XsString::non_null_ptr_type file_structure =
			find_and_create_one(elem, &create_xs_string, FILE_STRUCTURE, gpml_version, read_errors);

	// <gml:mimeType>
	boost::optional<XsString::non_null_ptr_type> mime_type =
			find_and_create_optional(elem, &create_xs_string, MIME_TYPE, gpml_version, read_errors);

	// <gml:compression>
	boost::optional<XsString::non_null_ptr_type> compression =
			find_and_create_optional(elem, &create_xs_string, COMPRESSION, gpml_version, read_errors);

	return GmlFile::create(range_parameters, file_name, file_structure, mime_type, compression, &read_errors);
}


GPlatesPropertyValues::GmlLineString::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_line_string(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("LineString"),
		POS_LIST = GPlatesModel::XmlElementName::create_gml("posList");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		polyline = find_and_create_one(elem, &create_polyline, POS_LIST, gpml_version, read_errors);

	// FIXME: We need to give the srsName et al. attributes from the posList 
	// to the line string!
	return GPlatesPropertyValues::GmlLineString::create(polyline);
}


GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_multi_point(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("MultiPoint"),
		POINT_MEMBER = GPlatesModel::XmlElementName::create_gml("pointMember");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	// GmlMultiPoint has multiple gml:pointMember properties each containing a
	// single gml:Point.
	typedef std::pair<GPlatesMaths::PointOnSphere, GPlatesPropertyValues::GmlPoint::GmlProperty> point_and_property_type;
	std::vector<point_and_property_type> points_and_properties;
	find_and_create_one_or_more(
			elem, &create_point_on_sphere, POINT_MEMBER, points_and_properties, gpml_version, read_errors);

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
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_orientable_curve(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("OrientableCurve"),
		BASE_CURVE = GPlatesModel::XmlElementName::create_gml("baseCurve");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GmlLineString::non_null_ptr_type
		line_string = find_and_create_one(elem, &create_gml_line_string,
				BASE_CURVE, gpml_version, read_errors);

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
		xml_attrs(elem->attributes_begin(), elem->attributes_end());
	return GPlatesPropertyValues::GmlOrientableCurve::create(line_string, xml_attrs);
}


GPlatesPropertyValues::GmlPoint::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_point(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	// Note: We call 'create_point_2d' instead of 'create_lon_lat_point_on_sphere' because
	// the former does not check for valid latitude/longitude ranges. This is important because
	// not all points read from GML are within valid lat/lon ranges - an example is the origin
	// of a rectified grid (georeferencing) where the georeferenced coordinates are in a
	// *projection* coordinate system (which is generally not specified in lat/lon).
	// Unfortunately this also means that regular points that are lat/lon points won't get
	// checked for valid lat/lon ranges - this will have to be delayed to when the point is
	// extracted from the GmlPoint property.
	std::pair<std::pair<double, double>, GPlatesPropertyValues::GmlPoint::GmlProperty> point =
			create_point_2d(parent, gpml_version, read_errors);

	// FIXME: We need to give the srsName et al. attributes from the posList 
	// to the line string!
	return GPlatesPropertyValues::GmlPoint::create_from_pos_2d(point.first, point.second);
}


GPlatesPropertyValues::GmlPolygon::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_polygon(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("Polygon"),
		INTERIOR = GPlatesModel::XmlElementName::create_gml("interior"),
		EXTERIOR = GPlatesModel::XmlElementName::create_gml("exterior");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	// GmlPolygon has exactly one exterior gml:LinearRing
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		exterior = find_and_create_one(elem, &create_linear_ring, EXTERIOR, gpml_version, read_errors);

	// GmlPolygon has zero or more interior gml:LinearRing
	std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> interiors;
	find_and_create_zero_or_more(
			elem, &create_linear_ring, INTERIOR, interiors, gpml_version, read_errors);

	// FIXME: We need to give the srsName et al. attributes from the posList 
	// (or the gml:FeatureCollection tag?) to the GmlPolygon (or the FeatureCollection)!
	return GPlatesPropertyValues::GmlPolygon::create(exterior, interiors);
}


GPlatesPropertyValues::GmlRectifiedGrid::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_rectified_grid(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("RectifiedGrid"),
		LIMITS = GPlatesModel::XmlElementName::create_gml("limits"),
		AXIS_NAME = GPlatesModel::XmlElementName::create_gml("axisName"),
		ORIGIN = GPlatesModel::XmlElementName::create_gml("origin"),
		OFFSET_VECTOR = GPlatesModel::XmlElementName::create_gml("offsetVector");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes(
			elem->attributes_begin(), elem->attributes_end());

	// <gml:limits>
	GmlGridEnvelope::non_null_ptr_type limits =
			find_and_create_one(elem, &create_gml_grid_envelope, LIMITS, gpml_version, read_errors);

	// <gml:axisName>
	std::vector<XsString::non_null_ptr_type> non_const_axes;
	find_and_create_one_or_more(elem, &create_xs_string,
			AXIS_NAME, non_const_axes, gpml_version, read_errors);
	GmlRectifiedGrid::axes_list_type axes(non_const_axes.begin(), non_const_axes.end());

	// <gml:origin>
	GmlPoint::non_null_ptr_type origin =
			find_and_create_one(elem, &create_gml_point, ORIGIN, gpml_version, read_errors);

	// <gml:offsetVector>
	GmlRectifiedGrid::offset_vector_list_type offset_vectors;
	find_and_create_one_or_more(
			elem, &create_double_list, OFFSET_VECTOR, offset_vectors, gpml_version, read_errors);

	return GmlRectifiedGrid::create(limits, axes, origin, offset_vectors, xml_attributes);
}


GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_time_instant(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("TimeInstant"),
		TIME_POSITION = GPlatesModel::XmlElementName::create_gml("timePosition");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GeoTimeInstant
		time = find_and_create_one(elem, &create_geo_time_instant,
				TIME_POSITION, gpml_version, read_errors);

	// The XML attributes are read from the timePosition property, not the TimeInstant property.
	return GPlatesPropertyValues::GmlTimeInstant::create(
			time,
			get_xml_attributes_from_child(elem, TIME_POSITION));
}


GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gml_time_period(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gml("TimePeriod"),
		BEGIN_TIME = GPlatesModel::XmlElementName::create_gml("begin"),
		END_TIME = GPlatesModel::XmlElementName::create_gml("end");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		begin_time = find_and_create_one(elem, &create_gml_time_instant,
				BEGIN_TIME, gpml_version, read_errors), 
		end_time = find_and_create_one(elem, &create_gml_time_instant,
				END_TIME, gpml_version, read_errors);

	return GPlatesPropertyValues::GmlTimePeriod::create(begin_time, end_time);
}


GPlatesPropertyValues::GpmlArray::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_array(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("Array"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType"),
		MEMBER = GPlatesModel::XmlElementName::create_gpml("member");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		mem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::StructuralType
		type = find_and_create_one(mem, &create_template_type_parameter_type,
				VALUE_TYPE, gpml_version, read_errors);

	std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> members;
	find_and_create_one_or_more_from_type(mem, type, MEMBER, members,
			structural_type_reader, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlArray::create(members, type);
}


GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_constant_value(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("ConstantValue"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType"),
		VALUE = GPlatesModel::XmlElementName::create_gpml("value"),
		DESCRIPTION = GPlatesModel::XmlElementName::create_gpml("description");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	boost::optional<QString> 
		description_string = find_and_create_optional(elem, &create_string,
				DESCRIPTION, gpml_version, read_errors);
	GPlatesPropertyValues::StructuralType 
		type = find_and_create_one(elem, &create_template_type_parameter_type,
				VALUE_TYPE, gpml_version, read_errors);
	GPlatesModel::PropertyValue::non_null_ptr_type value = 
		find_and_create_from_type(elem, type, VALUE,
				structural_type_reader, gpml_version, read_errors);

	boost::optional<GPlatesUtils::UnicodeString> description;
	if (description_string)
	{
		description = GPlatesUtils::make_icu_string_from_qstring(*description_string);
	}

	return GPlatesPropertyValues::GpmlConstantValue::create(value, type, description);
}


GPlatesPropertyValues::Enumeration::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_enumeration(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimEnumerationType &gpgim_property_enumeration_type,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	const GPlatesPropertyValues::EnumerationType enum_type(
			gpgim_property_enumeration_type.get_structural_type());

	const QString enum_value = create_nonempty_string(elem, gpml_version, read_errors);

	// Ensure the enumeration value is allowed, by the GPGIM, for the enumeration type.
	const GPlatesModel::GpgimEnumerationType::content_seq_type &enum_contents =
			gpgim_property_enumeration_type.get_contents();
	BOOST_FOREACH(const GPlatesModel::GpgimEnumerationType::Content &enum_content, enum_contents)
	{
		if (enum_value == enum_content.value)
		{
			return GPlatesPropertyValues::Enumeration::create(
					enum_type,
					GPlatesUtils::make_icu_string_from_qstring(enum_value));
		}
	}

	// The read enumeration value is not allowed by the GPGIM.
	throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
			elem, GPlatesFileIO::ReadErrors::InvalidEnumerationValue, EXCEPTION_SOURCE);
}


GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_feature_reference(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("FeatureReference"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::XmlElementName::create_gpml("targetFeature");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::StructuralType value_type =
			find_and_create_one(elem, &create_template_type_parameter_type,
					VALUE_TYPE, gpml_version, read_errors);
	GPlatesModel::FeatureId target_feature =
			find_and_create_one(elem, &create_feature_id, TARGET_FEATURE, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlFeatureReference::create(
			target_feature,
			GPlatesModel::FeatureType(value_type));
}


GPlatesPropertyValues::GpmlFeatureSnapshotReference::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_feature_snapshot_reference(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("FeatureSnapshotReference"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType"),
		TARGET_FEATURE = GPlatesModel::XmlElementName::create_gpml("targetFeature"),
		TARGET_REVISION = GPlatesModel::XmlElementName::create_gpml("targetRevision");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::StructuralType value_type =
			find_and_create_one(elem, &create_template_type_parameter_type,
					VALUE_TYPE, gpml_version, read_errors);
	GPlatesModel::FeatureId target_feature =
			find_and_create_one(elem, &create_feature_id, TARGET_FEATURE, gpml_version, read_errors);
	GPlatesModel::RevisionId target_revision =
			find_and_create_one(elem, &create_revision_id, TARGET_REVISION, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlFeatureSnapshotReference::create(
			target_feature,
			target_revision,
			GPlatesModel::FeatureType(value_type));
}

GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_finite_rotation(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		AXIS_ANGLE_FINITE_ROTATION = 
			GPlatesModel::XmlElementName::create_gpml("AxisAngleFiniteRotation"),
		ZERO_FINITE_ROTATION = 
			GPlatesModel::XmlElementName::create_gpml("ZeroFiniteRotation"),
		TOTAL_RECONSTRUCTION_POLE = 
			GPlatesModel::XmlElementName::create_gpml("TotalReconstructionPole");
	
	if (parent->number_of_children() > 1) {
		// Too many children!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				EXCEPTION_SOURCE);
	}

	bool is_trp = false;
	GPlatesModel::XmlElementNode::non_null_ptr_type xml_elem = parent; 
	boost::optional< GPlatesModel::XmlElementNode::non_null_ptr_type > structural_elem = 
		parent->get_child_by_name(TOTAL_RECONSTRUCTION_POLE);
	if (structural_elem) {
		is_trp = true;
		xml_elem = *structural_elem;
	}

	boost::optional<GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type> finite_rotation;

	structural_elem = xml_elem->get_child_by_name(ZERO_FINITE_ROTATION);
	if (structural_elem)
	{
		finite_rotation = GPlatesPropertyValues::GpmlFiniteRotation::create_zero_rotation();
	}
	else
	{
		structural_elem = xml_elem->get_child_by_name(AXIS_ANGLE_FINITE_ROTATION);
		if (structural_elem)
		{
			static const GPlatesModel::XmlElementName
					EULER_POLE = GPlatesModel::XmlElementName::create_gpml("eulerPole"),
					ANGLE = GPlatesModel::XmlElementName::create_gpml("angle");
			
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type euler_pole =
					find_and_create_one(structural_elem.get(), &create_gml_point,
							EULER_POLE, gpml_version, read_errors);
			GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type angle =
					find_and_create_one(structural_elem.get(), &create_gpml_measure,
							ANGLE, gpml_version, read_errors);
			finite_rotation = GPlatesPropertyValues::GpmlFiniteRotation::create(euler_pole, angle);
		}
	}

	if (!finite_rotation)
	{
		// Invalid child!
		throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
				parent, GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
				EXCEPTION_SOURCE);
	}
	else
	{
		if (is_trp)
		{
			return GPlatesPropertyValues::GpmlTotalReconstructionPole::create(
					(*finite_rotation)->get_finite_rotation(),
					*parent->get_child_by_name(TOTAL_RECONSTRUCTION_POLE));
		}
		else
		{
			return *finite_rotation;
		}
	}
}


GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_hot_spot_trail_mark(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{	
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("HotSpotTrailMark"),
		POSITION = GPlatesModel::XmlElementName::create_gpml("position"),
		TRAIL_WIDTH = GPlatesModel::XmlElementName::create_gpml("trailWidth"),
		MEASURED_AGE = GPlatesModel::XmlElementName::create_gpml("measuredAge"),
		MEASURED_AGE_RANGE = GPlatesModel::XmlElementName::create_gpml("measuredAgeRange");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::GmlPoint::non_null_ptr_type
		position = find_and_create_one(elem, &create_gml_point, POSITION, gpml_version, read_errors);
	boost::optional< GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type >
		trail_width = find_and_create_optional(elem, &create_gpml_measure,
				TRAIL_WIDTH, gpml_version, read_errors);
	boost::optional< GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type >
		measured_age = find_and_create_optional(elem, &create_gml_time_instant,
				MEASURED_AGE, gpml_version, read_errors);
	boost::optional< GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type >
		measured_age_range = find_and_create_optional(elem, &create_gml_time_period, 
				MEASURED_AGE_RANGE, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlHotSpotTrailMark::create(
			position, trail_width, measured_age, measured_age_range);
}


GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_irregular_sampling(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("IrregularSampling"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType"),
		TIME_SAMPLE = GPlatesModel::XmlElementName::create_gpml("timeSample"),
		INTERPOLATION_FUNCTION = GPlatesModel::XmlElementName::create_gpml("interpolationFunction");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::StructuralType
		type = find_and_create_one(elem, &create_template_type_parameter_type,
				VALUE_TYPE, gpml_version, read_errors);
	boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
		interp_func = find_and_create_optional(elem, &create_gpml_interpolation_function, 
				INTERPOLATION_FUNCTION, gpml_version, read_errors);

	std::vector<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type> time_samples;
	find_and_create_one_or_more(
			elem, &create_gpml_time_sample, TIME_SAMPLE, time_samples,
			structural_type_reader, gpml_version, read_errors);

	if (interp_func) {
		return GPlatesPropertyValues::GpmlIrregularSampling::create(
				time_samples, *interp_func, type);
	}
	return GPlatesPropertyValues::GpmlIrregularSampling::create(time_samples, boost::none, type);
}


GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_key_value_dictionary(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("KeyValueDictionary"),
		ELEMENT = GPlatesModel::XmlElementName::create_gpml("element");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type> elements;
	find_and_create_one_or_more(
			elem, &create_gpml_key_value_dictionary_element, ELEMENT, elements,
			structural_type_reader, gpml_version, read_errors);
	return GPlatesPropertyValues::GpmlKeyValueDictionary::create(elements);
}


GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_measure(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{	
	double quantity = create_double(elem, gpml_version, read_errors);

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
		xml_attrs(elem->attributes_begin(), elem->attributes_end());
	return GPlatesPropertyValues::GpmlMeasure::create(quantity, xml_attrs);
}


GPlatesPropertyValues::GpmlMetadata::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_metadata(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	GPlatesModel::FeatureCollectionMetadata meta(elem);
	return GPlatesPropertyValues::GpmlMetadata::create(meta);
}


GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_old_plates_header(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("OldPlatesHeader"),
		REGION_NUMBER = GPlatesModel::XmlElementName::create_gpml("regionNumber"),
		REFERENCE_NUMBER = GPlatesModel::XmlElementName::create_gpml("referenceNumber"),
		STRING_NUMBER = GPlatesModel::XmlElementName::create_gpml("stringNumber"),
		GEOGRAPHIC_DESCRIPTION = GPlatesModel::XmlElementName::create_gpml("geographicDescription"),
		PLATE_ID_NUMBER = GPlatesModel::XmlElementName::create_gpml("plateIdNumber"),
		AGE_OF_APPEARANCE = GPlatesModel::XmlElementName::create_gpml("ageOfAppearance"),
		AGE_OF_DISAPPEARANCE = GPlatesModel::XmlElementName::create_gpml("ageOfDisappearance"),
		DATA_TYPE_CODE = GPlatesModel::XmlElementName::create_gpml("dataTypeCode"),
		DATA_TYPE_CODE_NUMBER = GPlatesModel::XmlElementName::create_gpml("dataTypeCodeNumber"),
		DATA_TYPE_CODE_NUMBER_ADDITIONAL = GPlatesModel::XmlElementName::create_gpml("dataTypeCodeNumberAdditional"),
		CONJUGATE_PLATE_ID_NUMBER = GPlatesModel::XmlElementName::create_gpml("conjugatePlateIdNumber"),
		COLOUR_CODE = GPlatesModel::XmlElementName::create_gpml("colourCode"),
		NUMBER_OF_POINTS = GPlatesModel::XmlElementName::create_gpml("numberOfPoints");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	unsigned int region_number =
			find_and_create_one(elem, &create_uint, REGION_NUMBER, gpml_version, read_errors);
	unsigned int reference_number =
			find_and_create_one(elem, &create_uint, REFERENCE_NUMBER, gpml_version, read_errors);
	unsigned int string_number =
			find_and_create_one(elem, &create_uint, STRING_NUMBER, gpml_version, read_errors);
	QString geographic_description = 
			find_and_create_one(elem, &create_string, GEOGRAPHIC_DESCRIPTION, gpml_version, read_errors);
	GPlatesModel::integer_plate_id_type plate_id_number =
			find_and_create_one(elem, &create_ulong, PLATE_ID_NUMBER, gpml_version, read_errors);
	double age_of_appearance =
			find_and_create_one(elem, &create_double, AGE_OF_APPEARANCE, gpml_version, read_errors);
	double age_of_disappearance =
			find_and_create_one(elem, &create_double, AGE_OF_DISAPPEARANCE, gpml_version, read_errors);
	QString data_type_code =
			find_and_create_one(elem, &create_string, DATA_TYPE_CODE, gpml_version, read_errors);
	unsigned int data_type_code_number = 
			find_and_create_one(elem, &create_uint, DATA_TYPE_CODE_NUMBER, gpml_version, read_errors);
	QString data_type_code_number_additional =
			find_and_create_one(elem, &create_string, DATA_TYPE_CODE_NUMBER_ADDITIONAL, gpml_version, read_errors);
	GPlatesModel::integer_plate_id_type conjugate_plate_id_number = 
			find_and_create_one(elem, &create_uint, CONJUGATE_PLATE_ID_NUMBER, gpml_version, read_errors);
	unsigned int colour_code =
			find_and_create_one(elem, &create_uint, COLOUR_CODE, gpml_version, read_errors);
	unsigned int number_of_points =
			find_and_create_one(elem, &create_uint, NUMBER_OF_POINTS, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlOldPlatesHeader::create(
			region_number, reference_number, string_number, 
			GPlatesUtils::make_icu_string_from_qstring(geographic_description),
			plate_id_number, age_of_appearance, age_of_disappearance, 
			GPlatesUtils::make_icu_string_from_qstring(data_type_code),
			data_type_code_number, 
			GPlatesUtils::make_icu_string_from_qstring(data_type_code_number_additional),
			conjugate_plate_id_number, colour_code, number_of_points);
}


GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_piecewise_aggregation(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GpmlPropertyStructuralTypeReader &structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("PiecewiseAggregation"),
		VALUE_TYPE = GPlatesModel::XmlElementName::create_gpml("valueType"),
		TIME_WINDOW = GPlatesModel::XmlElementName::create_gpml("timeWindow");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	GPlatesPropertyValues::StructuralType
		type = find_and_create_one(elem, &create_template_type_parameter_type,
				VALUE_TYPE, gpml_version, read_errors);

	std::vector<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type> time_windows;

	find_and_create_zero_or_more(
			elem, &create_gpml_time_window, TIME_WINDOW, time_windows,
					structural_type_reader, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlPiecewiseAggregation::create(time_windows, type);
}


GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_plate_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::GpmlPlateId::create(create_ulong(elem, gpml_version, read_errors));
}


GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_polarity_chron_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("PolarityChronId"),
		ERA   = GPlatesModel::XmlElementName::create_gpml("era"),
		MAJOR = GPlatesModel::XmlElementName::create_gpml("major"),
		MINOR = GPlatesModel::XmlElementName::create_gpml("minor");
	
	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	boost::optional<QString>
		era = find_and_create_optional(elem, &create_string, ERA, gpml_version, read_errors);
	boost::optional<unsigned int>
		major_region = find_and_create_optional(elem, &create_uint, MAJOR, gpml_version, read_errors);
	boost::optional<QString>
		minor_region = find_and_create_optional(elem, &create_string, MINOR, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlPolarityChronId::create(era, major_region, minor_region);
}


GPlatesPropertyValues::GpmlRasterBandNames::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_raster_band_names(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("RasterBandNames"),
		BAND_NAME = GPlatesModel::XmlElementName::create_gpml("bandName");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<XsString::non_null_ptr_type> band_names;
        find_and_create_zero_or_more(elem, &create_xs_string,
				BAND_NAME, band_names, gpml_version, read_errors);

	// Check for uniqueness of band names.
	std::set<GPlatesUtils::UnicodeString> band_name_set;
	BOOST_FOREACH(const XsString::non_null_ptr_type &band_name, band_names)
	{
		if (!band_name_set.insert(band_name->get_value().get()).second)
		{
			throw GpmlReaderException(GPLATES_EXCEPTION_SOURCE,
					elem, GPlatesFileIO::ReadErrors::DuplicateRasterBandName,
					EXCEPTION_SOURCE);
		}
	}

	return GpmlRasterBandNames::create(band_names.begin(), band_names.end());
}


GPlatesPropertyValues::GpmlRevisionId::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_revision_id(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	return GPlatesPropertyValues::GpmlRevisionId::create(create_revision_id(elem, gpml_version, read_errors));
}


GPlatesPropertyValues::GpmlScalarField3DFile::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_scalar_field_3d_file(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	using namespace GPlatesPropertyValues;

	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("ScalarField3DFile"),
		FILE_NAME = GPlatesModel::XmlElementName::create_gpml("fileName");

	GPlatesModel::XmlElementNode::non_null_ptr_type elem =
		get_structural_type_element(parent, STRUCTURAL_TYPE);

	const XsString::non_null_ptr_type filename =
        find_and_create_one(elem, &create_xs_string, FILE_NAME, gpml_version, read_errors);

	return GpmlScalarField3DFile::create(filename);
}


GPlatesPropertyValues::GpmlStringList::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_string_list(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
			ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName
		STRUCTURAL_TYPE = GPlatesModel::XmlElementName::create_gpml("StringList"),
		ELEMENT = GPlatesModel::XmlElementName::create_gpml("element");

	GPlatesModel::XmlElementNode::non_null_ptr_type 
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesUtils::UnicodeString> elements;
	find_and_create_zero_or_more(elem, &create_unicode_string, ELEMENT, elements, gpml_version, read_errors);
	return GPlatesPropertyValues::GpmlStringList::create_copy(elements);
}


GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_topological_line(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = 
			GPlatesModel::XmlElementName::create_gpml("TopologicalLine"),
		SECTION = 
			GPlatesModel::XmlElementName::create_gpml("section");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> sections;

	find_and_create_one_or_more(elem, &create_gpml_topological_section,
			SECTION, sections, gpml_version, read_errors);

	return GPlatesPropertyValues::GpmlTopologicalLine::create(sections.begin(), sections.end());
}


GPlatesPropertyValues::GpmlTopologicalNetwork::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_topological_network(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = 
			GPlatesModel::XmlElementName::create_gpml("TopologicalNetwork"),
		BOUNDARY = 
			GPlatesModel::XmlElementName::create_gpml("boundary"),
		INTERIOR = 
			GPlatesModel::XmlElementName::create_gpml("interior");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	// GpmlTopologicalNetwork has exactly one boundary.
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> boundary_sections =
			find_and_create_one(elem, &create_topological_sections,
					BOUNDARY, gpml_version, read_errors);

	// GpmlTopologicalNetwork has zero or more interiors.
	std::vector<GPlatesPropertyValues::GpmlTopologicalNetwork::Interior> interiors;
	find_and_create_zero_or_more(elem, &create_gpml_topological_network_interior,
				INTERIOR, interiors, gpml_version, read_errors);

	if (interiors.empty())
	{
		return GPlatesPropertyValues::GpmlTopologicalNetwork::create(
				boundary_sections.begin(),
				boundary_sections.end());
	}

	return GPlatesPropertyValues::GpmlTopologicalNetwork::create(
			boundary_sections.begin(),
			boundary_sections.end(),
			interiors.begin(),
			interiors.end());
}


GPlatesPropertyValues::GpmlTopologicalPolygon::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReaderUtils::create_gpml_topological_polygon(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
		const GPlatesModel::GpgimVersion &gpml_version,
		ReadErrorAccumulation &read_errors)
{
	static const GPlatesModel::XmlElementName 
		STRUCTURAL_TYPE = 
			GPlatesModel::XmlElementName::create_gpml("TopologicalPolygon");

	GPlatesModel::XmlElementNode::non_null_ptr_type
		elem = get_structural_type_element(parent, STRUCTURAL_TYPE);

	// Prior to GPGIM version 1.6.319 there was no 'exterior' element.
	static const GPlatesModel::GpgimVersion GPGIM_VERSION_1_6_319(1, 6, 319);
	if (gpml_version < GPGIM_VERSION_1_6_319)
	{
		static const GPlatesModel::XmlElementName 
			SECTION = 
				GPlatesModel::XmlElementName::create_gpml("section");

		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> sections;
		find_and_create_one_or_more(
				elem, &create_gpml_topological_section, SECTION, sections, gpml_version, read_errors);

		return GPlatesPropertyValues::GpmlTopologicalPolygon::create(
				sections.begin(),
				sections.end());
	}

	static const GPlatesModel::XmlElementName 
		EXTERIOR = 
			GPlatesModel::XmlElementName::create_gpml("exterior");

	// GpmlTopologicalBoundary has exactly one exterior.
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> exterior_sections =
			find_and_create_one(elem, &create_topological_sections, EXTERIOR, gpml_version, read_errors);

	// TODO: Add 'interior' polygons.

	return GPlatesPropertyValues::GpmlTopologicalPolygon::create(
			exterior_sections.begin(),
			exterior_sections.end());
}
