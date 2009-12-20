/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009 The University of Sydney, Australia
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

#include "ModelUtils.h"
#include "Model.h"
#include "DummyTransactionHandle.h"
#include "FeatureHandle.h"
#include "FeatureRevision.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"


const GPlatesModel::TopLevelPropertyInline::non_null_ptr_type
GPlatesModel::ModelUtils::append_property_value_to_feature(
		PropertyValue::non_null_ptr_type property_value,
		const PropertyName &property_name,
		const FeatureHandle::weak_ref &feature)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	TopLevelPropertyInline::non_null_ptr_type top_level_property =
			TopLevelPropertyInline::create(property_name, property_value, xml_attributes);

	DummyTransactionHandle transaction(__FILE__, __LINE__);
	feature->append_top_level_property(top_level_property, transaction);
	transaction.commit();

	return top_level_property;
}


const GPlatesModel::TopLevelPropertyInline::non_null_ptr_type
GPlatesModel::ModelUtils::append_property_value_to_feature(
		PropertyValue::non_null_ptr_type property_value,
		const PropertyName &property_name,
		const UnicodeString &attribute_name_string,
		const UnicodeString &attribute_value_string,
		const FeatureHandle::weak_ref &feature)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	// FIXME: xml_attribute_name is not always GPML!
	XmlAttributeName xml_attribute_name =
		XmlAttributeName::create_gpml(
				GPlatesUtils::make_qstring_from_icu_string(attribute_name_string));
	XmlAttributeValue xml_attribute_value(attribute_value_string);
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	TopLevelPropertyInline::non_null_ptr_type top_level_property =
			TopLevelPropertyInline::create(property_name, property_value, xml_attributes);

	DummyTransactionHandle transaction(__FILE__, __LINE__);
	feature->append_top_level_property(top_level_property, transaction);
	transaction.commit();

	return top_level_property;
}


const GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesModel::ModelUtils::append_property_value_to_feature(
		TopLevelProperty::non_null_ptr_type top_level_property,
		const FeatureHandle::weak_ref &feature)
{
	DummyTransactionHandle transaction(__FILE__, __LINE__);
	feature->append_top_level_property(top_level_property, transaction);
	transaction.commit();

	return top_level_property;
}


void
GPlatesModel::ModelUtils::remove_property_value_from_feature(
		FeatureHandle::properties_iterator properties_iterator,
		const FeatureHandle::weak_ref &feature)
{
	DummyTransactionHandle transaction(__FILE__, __LINE__);
	feature->remove_top_level_property(properties_iterator, transaction);
	transaction.commit();
}


void
GPlatesModel::ModelUtils::remove_property_values_from_feature(
		const PropertyName &property_name,
		const FeatureHandle::weak_ref &feature)
{
	FeatureHandle::properties_iterator feature_properties_iter = feature->properties_begin();
	FeatureHandle::properties_iterator feature_properties_end = feature->properties_end();
	while (feature_properties_iter != feature_properties_end)
	{
		// Increment iterator before we remove property.
		// I don't think this is currently necessary but it doesn't hurt.
		FeatureHandle::properties_iterator current_feature_properties_iter =
				feature_properties_iter;
		++feature_properties_iter;

		if (current_feature_properties_iter.is_valid() &&
			(*current_feature_properties_iter)->property_name() == property_name)
		{
			remove_property_value_from_feature(
					current_feature_properties_iter, feature);
		}
	}
}


const GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
GPlatesModel::ModelUtils::create_gml_orientable_curve(
		const GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string,
		bool reverse_orientation)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("orientation");
	XmlAttributeValue xml_attribute_value(reverse_orientation ? "-" : "+");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	return GPlatesPropertyValues::GmlOrientableCurve::create(gml_line_string, xml_attributes);
}


const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
GPlatesModel::ModelUtils::create_gml_time_period(
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_begin,
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_end)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type gml_time_instant_begin =
			GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant_begin, xml_attributes);

	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type gml_time_instant_end =
			GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant_end, xml_attributes);

	return GPlatesPropertyValues::GmlTimePeriod::create(gml_time_instant_begin, gml_time_instant_end);
}


const GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
GPlatesModel::ModelUtils::create_gml_time_instant(
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	return GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant, xml_attributes);
}


const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
GPlatesModel::ModelUtils::create_gpml_constant_value(
		const PropertyValue::non_null_ptr_type property_value,
		const GPlatesPropertyValues::TemplateTypeParameterType &template_type_parameter_type)
{
	return GPlatesPropertyValues::GpmlConstantValue::create(property_value,
			template_type_parameter_type);
}


const GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesModel::ModelUtils::create_total_reconstruction_pole(
		const std::vector<TotalReconstructionPoleData> &five_tuples)
{
	std::vector<GPlatesPropertyValues::GpmlTimeSample> time_samples;
	GPlatesPropertyValues::TemplateTypeParameterType value_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("FiniteRotation");

	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("frame");
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
			GPlatesUtils::get_intrusive_ptr(gpml_finite_rotation_slerp), value_type);

	PropertyName property_name =
		PropertyName::create_gpml("totalReconstructionPole");
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes2;
	TopLevelProperty::non_null_ptr_type top_level_property_inline =
			TopLevelPropertyInline::create(property_name,
			gpml_irregular_sampling, xml_attributes2);

	return top_level_property_inline;
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::ModelUtils::create_total_recon_seq(
		ModelInterface &model,
		const FeatureCollectionHandle::weak_ref &target_collection,
		unsigned long fixed_plate_id,
		unsigned long moving_plate_id,
		const std::vector<TotalReconstructionPoleData> &five_tuples)
{
	FeatureType feature_type = FeatureType::create_gpml("TotalReconstructionSequence");
	FeatureHandle::weak_ref feature =
			model->create_feature(feature_type, target_collection);

	TopLevelProperty::non_null_ptr_type total_reconstruction_pole_container =
			create_total_reconstruction_pole(five_tuples);

	DummyTransactionHandle pc1(__FILE__, __LINE__);
	feature->append_top_level_property(total_reconstruction_pole_container, pc1);
	pc1.commit();

	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type fixed_ref_frame(
			GPlatesPropertyValues::GpmlPlateId::create(fixed_plate_id));
	ModelUtils::append_property_value_to_feature(
			fixed_ref_frame,
			PropertyName::create_gpml("fixedReferenceFrame"), 
			feature);

	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type moving_ref_frame(
			GPlatesPropertyValues::GpmlPlateId::create(moving_plate_id));
	ModelUtils::append_property_value_to_feature(
			moving_ref_frame,
			PropertyName::create_gpml("movingReferenceFrame"), 
			feature);

	return feature;
}
