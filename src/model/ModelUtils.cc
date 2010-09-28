/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#include "Model.h"
#include "ModelUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"


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
	FeatureHandle::weak_ref feature = FeatureHandle::create(target_collection, feature_type);

	TopLevelProperty::non_null_ptr_type total_reconstruction_pole_container =
			create_total_reconstruction_pole(five_tuples);

	// DummyTransactionHandle pc1(__FILE__, __LINE__);
	feature->add(total_reconstruction_pole_container);
	// pc1.commit();

	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type fixed_ref_frame(
			GPlatesPropertyValues::GpmlPlateId::create(fixed_plate_id));
	feature->add(
			TopLevelPropertyInline::create(
				PropertyName::create_gpml("fixedReferenceFrame"),
				fixed_ref_frame));

	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type moving_ref_frame(
			GPlatesPropertyValues::GpmlPlateId::create(moving_plate_id));
	feature->add(
			TopLevelPropertyInline::create(
				PropertyName::create_gpml("movingReferenceFrame"),
				moving_ref_frame));

	return feature;
}

