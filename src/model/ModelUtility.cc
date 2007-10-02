/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include "ModelUtility.h"
#include "DummyTransactionHandle.h"
#include "FeatureHandle.h"
#include "FeatureRevision.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"


const GPlatesModel::InlinePropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::append_property_value_to_feature(
		PropertyValue::non_null_ptr_type property_value,
		const char *property_name_string,
		FeatureHandle::weak_ref &feature)
{
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	InlinePropertyContainer::non_null_ptr_type property_container =
			InlinePropertyContainer::create(property_name, property_value, xml_attributes);

	DummyTransactionHandle transaction(__FILE__, __LINE__);
	feature->append_property_container(property_container, transaction);
	transaction.commit();

	return property_container;
}


const GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
GPlatesModel::ModelUtility::create_gml_time_instant(
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	return GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant, xml_attributes);
}


const GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
GPlatesModel::ModelUtility::create_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlTimeSample &first_time_sample)
{
	GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type gpml_finite_rotation_slerp =
			GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(first_time_sample.value_type());

	return GPlatesPropertyValues::GpmlIrregularSampling::create(first_time_sample,
			GPlatesContrib::get_intrusive_ptr(gpml_finite_rotation_slerp),
			first_time_sample.value_type());
}


const GPlatesPropertyValues::XsString::non_null_ptr_type
GPlatesModel::ModelUtility::create_xs_string(
		const std::string &str)
{
	return GPlatesPropertyValues::XsString::create(UnicodeString(str.c_str()));
}


const GPlatesPropertyValues::XsBoolean::non_null_ptr_type
GPlatesModel::ModelUtility::create_xs_boolean(
		bool value)
{
	return GPlatesPropertyValues::XsBoolean::create(value);
}


const GPlatesPropertyValues::GpmlStrikeSlipEnumeration::non_null_ptr_type
GPlatesModel::ModelUtility::create_gpml_strike_slip_enumeration(
		const std::string &value)
{
	return GPlatesPropertyValues::GpmlStrikeSlipEnumeration::create(UnicodeString(value.c_str()));
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_reconstruction_plate_id(
		unsigned long plate_id)
{
	PropertyValue::non_null_ptr_type gpml_plate_id =
			GPlatesPropertyValues::GpmlPlateId::create(plate_id);

	UnicodeString template_type_parameter_type_string("gpml:plateId");
	GPlatesPropertyValues::TemplateTypeParameterType template_type_parameter_type(template_type_parameter_type_string);
	PropertyValue::non_null_ptr_type gpml_plate_id_constant_value =
			GPlatesPropertyValues::GpmlConstantValue::create(gpml_plate_id, template_type_parameter_type);

	UnicodeString property_name_string("gpml:reconstructionPlateId");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gpml_plate_id_constant_value, xml_attributes);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_reference_frame_plate_id(
		unsigned long plate_id,
		const char *which_reference_frame)
{
	PropertyValue::non_null_ptr_type gpml_plate_id =
			GPlatesPropertyValues::GpmlPlateId::create(plate_id);

	UnicodeString property_name_string(which_reference_frame);
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gpml_plate_id, xml_attributes);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_centre_line_of(
		const std::vector<double> &gml_pos_list)
{
	PropertyValue::non_null_ptr_type gml_line_string =
			GPlatesPropertyValues::GmlLineString::create(gml_pos_list);

	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name("orientation");
	XmlAttributeValue xml_attribute_value("+");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	PropertyValue::non_null_ptr_type gml_orientable_curve =
			GPlatesPropertyValues::GmlOrientableCurve::create(gml_line_string, xml_attributes);

	UnicodeString template_type_parameter_type_string("gml:OrientableCurve");
	GPlatesPropertyValues::TemplateTypeParameterType template_type_parameter_type(template_type_parameter_type_string);
	PropertyValue::non_null_ptr_type gml_orientable_curve_constant_value =
			GPlatesPropertyValues::GpmlConstantValue::create(gml_orientable_curve, template_type_parameter_type);

	UnicodeString property_name_string("gpml:centreLineOf");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes2;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gml_orientable_curve_constant_value, xml_attributes2);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_valid_time(
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_begin,
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_end)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type gml_time_instant_begin =
			GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant_begin, xml_attributes);

	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type gml_time_instant_end =
			GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant_end, xml_attributes);

	PropertyValue::non_null_ptr_type gml_time_period =
			GPlatesPropertyValues::GmlTimePeriod::create(gml_time_instant_begin, gml_time_instant_end);

	UnicodeString property_name_string("gml:validTime");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes2;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gml_time_period, xml_attributes2);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_name(
		const UnicodeString &name,
		const UnicodeString &codespace)
{
	PropertyValue::non_null_ptr_type gml_name = GPlatesPropertyValues::XsString::create(name);

	UnicodeString property_name_string("gml:name");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name("codeSpace");
	XmlAttributeValue xml_attribute_value(codespace);
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gml_name, xml_attributes);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_total_reconstruction_pole(
		const std::vector<TotalReconstructionPoleData> &five_tuples)
{
	std::vector<GPlatesPropertyValues::GpmlTimeSample> time_samples;
	GPlatesPropertyValues::TemplateTypeParameterType value_type("gpml:FiniteRotation");

	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	for (std::vector<TotalReconstructionPoleData>::const_iterator iter = five_tuples.begin(); 
				iter != five_tuples.end(); ++iter) 
	{
		std::pair<double, double> gpml_euler_pole =
				std::make_pair(iter->lon_of_euler_pole, iter->lat_of_euler_pole);
		GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type gpml_finite_rotation =
				GPlatesPropertyValues::GpmlFiniteRotation::create(gpml_euler_pole,
				iter->rotation_angle);

		GPlatesPropertyValues::GeoTimeInstant geo_time_instant(iter->time);
		GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type gml_time_instant =
				GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant, xml_attributes);

		UnicodeString comment_string(iter->comment);
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_description =
				GPlatesPropertyValues::XsString::create(comment_string);

		time_samples.push_back(GPlatesPropertyValues::GpmlTimeSample(gpml_finite_rotation, gml_time_instant,
				get_intrusive_ptr(gml_description), value_type));
	}

	GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type gpml_finite_rotation_slerp =
			GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(value_type);

	PropertyValue::non_null_ptr_type gpml_irregular_sampling =
			GPlatesPropertyValues::GpmlIrregularSampling::create(time_samples,
			GPlatesContrib::get_intrusive_ptr(gpml_finite_rotation_slerp), value_type);

	UnicodeString property_name_string("gpml:totalReconstructionPole");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes2;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gpml_irregular_sampling, xml_attributes2);

	return inline_property_container;
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::ModelUtility::create_total_recon_seq(
		ModelInterface &model,
		FeatureCollectionHandle::weak_ref &target_collection,
		unsigned long fixed_plate_id,
		unsigned long moving_plate_id,
		const std::vector<TotalReconstructionPoleData> &five_tuples)
{
	UnicodeString feature_type_string("gpml:TotalReconstructionSequence");
	FeatureType feature_type(feature_type_string);
	FeatureHandle::weak_ref feature_handle =
			model.create_feature(feature_type, target_collection);

	PropertyContainer::non_null_ptr_type total_reconstruction_pole_container =
			create_total_reconstruction_pole(five_tuples);
	PropertyContainer::non_null_ptr_type fixed_reference_frame_container =
			create_reference_frame_plate_id(fixed_plate_id, "gpml:fixedReferenceFrame");
	PropertyContainer::non_null_ptr_type moving_reference_frame_container =
			create_reference_frame_plate_id(moving_plate_id, "gpml:movingReferenceFrame");

	DummyTransactionHandle pc1(__FILE__, __LINE__);
	feature_handle->append_property_container(total_reconstruction_pole_container, pc1);
	pc1.commit();

	DummyTransactionHandle pc2(__FILE__, __LINE__);
	feature_handle->append_property_container(fixed_reference_frame_container, pc2);
	pc2.commit();

	DummyTransactionHandle pc3(__FILE__, __LINE__);
	feature_handle->append_property_container(moving_reference_frame_container, pc3);
	pc3.commit();

	return feature_handle;
}
