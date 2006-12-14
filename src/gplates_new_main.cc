/* $Id$ */

/**
 * @file
 * Contains the main function of GPlates.
 *
 * This file used to be "model/compilation_test.cc".  It was used during the implementation of the
 * new GPGIM-1.5-based Model, to check that the new Model code compiled without error and ran
 * correctly.  The code in this file constructs some hard-coded GPGIM features (which are
 * minimalist but otherwise structurally accurate) and outputs them as GPML.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006 The University of Sydney, Australia
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

#include "model/FeatureHandle.h"
#include "model/FeatureRevision.h"
#include "model/GmlLineString.h"
#include "model/GmlOrientableCurve.h"
#include "model/GmlTimePeriod.h"
#include "model/GpmlConstantValue.h"
#include "model/GpmlFiniteRotationSlerp.h"
#include "model/GpmlIrregularSampling.h"
#include "model/GpmlPlateId.h"
#include "model/GpmlTimeSample.h"
#include "model/SingleValuedPropertyContainer.h"
#include "model/XsString.h"
#include "model/GpmlOnePointFiveOutputVisitor.h"
#include "model/XmlOutputInterface.h"


boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_reconstruction_plate_id(
		const unsigned long &plate_id) {

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_plate_id =
			GPlatesModel::GpmlPlateId::create(plate_id);

	UnicodeString template_type_parameter_type_string("gpml:plateId");
	GPlatesModel::TemplateTypeParameterType template_type_parameter_type(template_type_parameter_type_string);
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_plate_id_constant_value =
			GPlatesModel::GpmlConstantValue::create(gpml_plate_id, template_type_parameter_type);

	UnicodeString property_name_string("gpml:reconstructionPlateId");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> single_valued_property_container =
			GPlatesModel::SingleValuedPropertyContainer::create(property_name,
			gpml_plate_id_constant_value, xml_attributes, false);

	return single_valued_property_container;
}


boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_reference_frame_plate_id(
		const unsigned long &plate_id,
		const char *which_reference_frame) {

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_plate_id =
			GPlatesModel::GpmlPlateId::create(plate_id);

	UnicodeString property_name_string(which_reference_frame);
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> single_valued_property_container =
			GPlatesModel::SingleValuedPropertyContainer::create(property_name,
			gpml_plate_id, xml_attributes, false);

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

	UnicodeString template_type_parameter_type_string("gml:OrientableCurve");
	GPlatesModel::TemplateTypeParameterType template_type_parameter_type(template_type_parameter_type_string);
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_orientable_curve_constant_value =
			GPlatesModel::GpmlConstantValue::create(gml_orientable_curve, template_type_parameter_type);

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
		const unsigned long &plate_id,
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
	feature_handle.set_current_revision(revision);

	return feature_handle;
}


struct RotationFileFiveTuple {
	double time;
	double lat_of_euler_pole;
	double lon_of_euler_pole;
	double rotation_angle;
	const char *comment;
};


boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_total_reconstruction_pole(
		const RotationFileFiveTuple *five_tuples,
		unsigned num_five_tuples) {

	std::vector<GPlatesModel::GpmlTimeSample> time_samples;
	GPlatesModel::TemplateTypeParameterType value_type("gpml:FiniteRotation");

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	GPlatesModel::XmlAttributeName xml_attribute_name("frame");
	GPlatesModel::XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	for (unsigned i = 0; i < num_five_tuples; ++i) {
		GPlatesModel::GeoTimeInstant geo_time_instant(five_tuples[i].time);
		boost::intrusive_ptr<GPlatesModel::GmlTimeInstant> gml_time_instant =
				GPlatesModel::GmlTimeInstant::create(geo_time_instant, xml_attributes);

		UnicodeString comment_string(five_tuples[i].comment);
		boost::intrusive_ptr<GPlatesModel::XsString> gml_description =
				GPlatesModel::XsString::create(comment_string);

		time_samples.push_back(GPlatesModel::GpmlTimeSample(NULL, gml_time_instant,
				gml_description, value_type));
	}

	boost::intrusive_ptr<GPlatesModel::GpmlInterpolationFunction> gpml_finite_rotation_slerp =
			GPlatesModel::GpmlFiniteRotationSlerp::create(value_type);

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_irregular_sampling =
			GPlatesModel::GpmlIrregularSampling::create(time_samples,
			gpml_finite_rotation_slerp, value_type);

	UnicodeString property_name_string("gpml:totalReconstructionPole");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes2;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> single_valued_property_container =
			GPlatesModel::SingleValuedPropertyContainer::create(property_name,
			gpml_irregular_sampling, xml_attributes2, false);

	return single_valued_property_container;
}


GPlatesModel::FeatureHandle
create_total_recon_seq(
		const unsigned long &fixed_plate_id,
		const unsigned long &moving_plate_id,
		const RotationFileFiveTuple *five_tuples,
		unsigned num_five_tuples) {

	boost::intrusive_ptr<GPlatesModel::PropertyContainer> total_reconstruction_pole_container =
			create_total_reconstruction_pole(five_tuples, num_five_tuples);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> fixed_reference_frame_container =
			create_reference_frame_plate_id(fixed_plate_id, "gpml:fixedReferenceFrame");
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> moving_reference_frame_container =
			create_reference_frame_plate_id(moving_plate_id, "gpml:movingReferenceFrame");

	boost::intrusive_ptr<GPlatesModel::FeatureRevision> revision = GPlatesModel::FeatureRevision::create();
	revision->properties().push_back(total_reconstruction_pole_container);
	revision->properties().push_back(fixed_reference_frame_container);
	revision->properties().push_back(moving_reference_frame_container);

	GPlatesModel::FeatureId feature_id;
	UnicodeString feature_type_string("gpml:TotalReconstructionSequence");
	GPlatesModel::FeatureType feature_type(feature_type_string);
	GPlatesModel::FeatureHandle feature_handle(feature_id, feature_type);
	feature_handle.set_current_revision(revision);

	return feature_handle;
}


int
main() {

	static const unsigned long plate_id = 501;
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
			create_isochron(plate_id, points, num_points, geo_time_instant_begin,
			geo_time_instant_end, description, name, codespace_of_name);

	static const unsigned long fixed_plate_id = 511;
	static const unsigned long moving_plate_id = 501;
	static const RotationFileFiveTuple five_tuples[] = {
		{	0.0,	90.0,	0.0,	0.0,	"IND-CIB India-Central Indian Basin"	},
		{	9.9,	-8.7,	76.9,	2.75,	"IND-CIB AN 5 JYR 7/4/89"	},
		{	20.2,	-5.2,	74.3,	5.93,	"IND-CIB Royer & Chang 1991"	},
		{	83.5,	-5.2,	74.3,	5.93,	"IND-CIB switchover"	},
	};
	static const unsigned num_five_tuples = sizeof(five_tuples) / sizeof(five_tuples[0]);

	GPlatesModel::FeatureHandle total_recon_seq =
			create_total_recon_seq(fixed_plate_id, moving_plate_id, five_tuples,
			num_five_tuples);

	GPlatesFileIO::XmlOutputInterface xoi =
			GPlatesFileIO::XmlOutputInterface::create_for_stdout();
	GPlatesFileIO::GpmlOnePointFiveOutputVisitor v(xoi);
	isochron.accept_visitor(v);
	total_recon_seq.accept_visitor(v);

	return 0;
}
