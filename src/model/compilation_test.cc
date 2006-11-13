/* $Id$ */

/**
 * @file
 * This file has been used during the implementation of the new GPGIM-1.5-based Model, to check
 * that the new Model code compiles without error and runs correctly.
 *
 * This file (and the accompanying Makefile, "Makefile.compilation_test") enabled me to compile and
 * run the Model code without needing to wire it into the rest of the program.  The code in this
 * file constructs some hard-coded GPGIM features (which are minimalist but otherwise structurally
 * accurate) and outputs them as GPML.
 *
 * This file is not intended to be part of the GPlates program, but is committed to the repository
 * for the benefit of posterity.  (Also, in case I accidentally delete it when I'm tired.)
 *
 * This file should not be compiled into the GPlates executable.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "FeatureHandle.h"
#include "FeatureRevision.h"
#include "GmlLineString.h"
#include "GmlOrientableCurve.h"
#include "GmlTimePeriod.h"
#include "GpmlConstantValue.h"
#include "GpmlPlateId.h"
#include "SingleValuedPropertyContainer.h"
#include "XsString.h"
#include "GpmlOnePointFiveOutputVisitor.h"
#include "XmlOutputInterface.h"


boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_reconstruction_plate_id(
		unsigned long plate_id) {

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_plate_id =
			GPlatesModel::GpmlPlateId::create(plate_id);

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_plate_id_constant_value =
			GPlatesModel::GpmlConstantValue::create(gpml_plate_id);

	UnicodeString property_name_string("gpml:reconstructionPlateId");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> single_valued_property_container =
			GPlatesModel::SingleValuedPropertyContainer::create(property_name,
			gpml_plate_id_constant_value, xml_attributes, false);

	return single_valued_property_container;
}


boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_centre_line_of(
		const double *points,
		unsigned num_points) {

	std::vector<double> gml_pos_list(points, points + num_points);
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_line_string =
			GPlatesModel::GmlLineString::create(gml_pos_list);

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	GPlatesModel::XmlAttributeName xml_attribute_name("orientation");
	GPlatesModel::XmlAttributeValue xml_attribute_value("+");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_orientable_curve =
			GPlatesModel::GmlOrientableCurve::create(gml_line_string, xml_attributes);

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_orientable_curve_constant_value =
			GPlatesModel::GpmlConstantValue::create(gml_orientable_curve);

	UnicodeString property_name_string("gpml:centreLineOf");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes2;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> single_valued_property_container =
			GPlatesModel::SingleValuedPropertyContainer::create(property_name,
			gml_orientable_curve_constant_value, xml_attributes2, false);

	return single_valued_property_container;
}


boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_valid_time(
		const GPlatesModel::GeoTimeInstant &geo_time_instant_begin,
		const GPlatesModel::GeoTimeInstant &geo_time_instant_end) {

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	GPlatesModel::XmlAttributeName xml_attribute_name("frame");
	GPlatesModel::XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	boost::intrusive_ptr<GPlatesModel::GmlTimeInstant> gml_time_instant_begin =
			GPlatesModel::GmlTimeInstant::create(geo_time_instant_begin, xml_attributes);

	boost::intrusive_ptr<GPlatesModel::GmlTimeInstant> gml_time_instant_end =
			GPlatesModel::GmlTimeInstant::create(geo_time_instant_end, xml_attributes);

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_time_period =
			GPlatesModel::GmlTimePeriod::create(gml_time_instant_begin, gml_time_instant_end);

	UnicodeString property_name_string("gml:validTime");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes2;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> single_valued_property_container =
			GPlatesModel::SingleValuedPropertyContainer::create(property_name,
			gml_time_period, xml_attributes2, false);

	return single_valued_property_container;
}


boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_description(
		const UnicodeString &description) {

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_description = GPlatesModel::XsString::create(description);

	UnicodeString property_name_string("gml:description");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> single_valued_property_container =
			GPlatesModel::SingleValuedPropertyContainer::create(property_name,
			gml_description, xml_attributes, false);

	return single_valued_property_container;
}


boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_name(
		const UnicodeString &name,
		const UnicodeString &codespace) {

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_name = GPlatesModel::XsString::create(name);

	UnicodeString property_name_string("gml:name");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	GPlatesModel::XmlAttributeName xml_attribute_name("codeSpace");
	GPlatesModel::XmlAttributeValue xml_attribute_value(codespace);
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> single_valued_property_container =
			GPlatesModel::SingleValuedPropertyContainer::create(property_name,
			gml_name, xml_attributes, false);

	return single_valued_property_container;
}


GPlatesModel::FeatureHandle
create_isochron(
		unsigned long plate_id,
		const double *points,
		unsigned num_points,
		const GPlatesModel::GeoTimeInstant &geo_time_instant_begin,
		const GPlatesModel::GeoTimeInstant &geo_time_instant_end,
		const UnicodeString &description,
		const UnicodeString &name,
		const UnicodeString &codespace_of_name) {

	boost::intrusive_ptr<GPlatesModel::PropertyContainer> reconstruction_plate_id_container =
			create_reconstruction_plate_id(plate_id);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> centre_line_of_container =
			create_centre_line_of(points, num_points);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> valid_time_container =
			create_valid_time(geo_time_instant_begin, geo_time_instant_end);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> description_container =
			create_description(description);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> name_container =
			create_name(name, codespace_of_name);

	boost::intrusive_ptr<GPlatesModel::FeatureRevision> revision = GPlatesModel::FeatureRevision::create();
	revision->properties().push_back(reconstruction_plate_id_container);
	revision->properties().push_back(centre_line_of_container);
	revision->properties().push_back(valid_time_container);
	revision->properties().push_back(description_container);
	revision->properties().push_back(name_container);

	GPlatesModel::FeatureId feature_id;
	UnicodeString feature_type_string("gpml:Isochron");
	GPlatesModel::FeatureType feature_type(feature_type_string);
	GPlatesModel::FeatureHandle feature_handle(feature_id, feature_type);
	feature_handle.swap_revision(revision);

	return feature_handle;
}


int
main() {

	unsigned long plate_id = 501;
	static const double points[] = {
		-5.5765,
		69.2877,
		-4.8556,
		69.1323
	};
	static const unsigned num_points = sizeof(points) / sizeof(points[0]);
	GPlatesModel::GeoTimeInstant geo_time_instant_begin(10.9);
	GPlatesModel::GeoTimeInstant geo_time_instant_end =
			GPlatesModel::GeoTimeInstant::create_distant_future();
	UnicodeString description("CARLSBERG RIDGE, INDIA-AFRICA ANOMALY 5 ISOCHRON");
	UnicodeString name("Izzy the Isochron");
	UnicodeString codespace_of_name("EarthByte");

	GPlatesModel::FeatureHandle isochron =
			create_isochron(plate_id, points, num_points, geo_time_instant_begin, geo_time_instant_end,
			description, name, codespace_of_name);

	GPlatesFileIO::XmlOutputInterface xoi = GPlatesFileIO::XmlOutputInterface::create_for_stdout();
	GPlatesFileIO::GpmlOnePointFiveOutputVisitor v(xoi);
	isochron.accept_visitor(v);

	return 0;
}
