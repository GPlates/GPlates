/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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

#ifndef GPLATES_MODEL_MODELUTILS_H
#define GPLATES_MODEL_MODELUTILS_H

#include "ModelInterface.h"
#include "FeatureCollectionHandle.h"
#include "FeatureHandle.h"
#include "InlinePropertyContainer.h"
#include "DummyTransactionHandle.h"
#include "PropertyName.h"
#include "PropertyValue.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/TemplateTypeParameterType.h"

namespace GPlatesModel
{
	namespace ModelUtils
	{
		struct TotalReconstructionPoleData
		{
			double time;
			double lat_of_euler_pole;
			double lon_of_euler_pole;
			double rotation_angle;
			const char *comment;
		};


		const InlinePropertyContainer::non_null_ptr_type
		append_property_value_to_feature(
				PropertyValue::non_null_ptr_type property_value,
				const PropertyName &property_name,
				FeatureHandle::weak_ref &feature);


		const InlinePropertyContainer::non_null_ptr_type
		append_property_value_to_feature(
				PropertyValue::non_null_ptr_type property_value,
				const PropertyName &property_name,
				const UnicodeString &attribute_name_string,
				const UnicodeString &attribute_value_string,
				FeatureHandle::weak_ref &feature);


		template< typename AttributeIterator >
		const InlinePropertyContainer::non_null_ptr_type
		append_property_value_to_feature(
				PropertyValue::non_null_ptr_type property_value,
				const PropertyName &property_name,
				const AttributeIterator &attributes_begin,
				const AttributeIterator &attributes_end,
				FeatureHandle::weak_ref &feature);


		const GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
		create_gml_orientable_curve(
				const GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string,
				bool reverse_orientation = false);


		const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
		create_gml_time_period(
				const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_begin,
				const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_end);


		const GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		create_gml_time_instant(
				const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant);


		const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
		create_gpml_constant_value(
				const PropertyValue::non_null_ptr_type property_value,
				const GPlatesPropertyValues::TemplateTypeParameterType &template_type_parameter_type);


		// Before this line are the new, hopefully-better-designed functions; after this
		// line are the old, arbitrary functions which should probably be reviewed (and
		// should quite possibly be refactored).
		// FIXME:  Review the following functions and refactor if necessary.

		const PropertyContainer::non_null_ptr_type
		create_total_reconstruction_pole(
				const std::vector<TotalReconstructionPoleData> &five_tuples);
	
		const FeatureHandle::weak_ref
		create_total_recon_seq(
				ModelInterface &model,
				FeatureCollectionHandle::weak_ref &target_collection,
				unsigned long fixed_plate_id,
				unsigned long moving_plate_id,
				const std::vector<TotalReconstructionPoleData> &five_tuples);
	}


	template< typename AttributeIterator >
	const InlinePropertyContainer::non_null_ptr_type
	ModelUtils::append_property_value_to_feature(
			PropertyValue::non_null_ptr_type property_value,
			const PropertyName &property_name,
			const AttributeIterator &attributes_begin,
			const AttributeIterator &attributes_end,
			FeatureHandle::weak_ref &feature)
	{
		std::map<XmlAttributeName, XmlAttributeValue> xml_attributes(
				attributes_begin, attributes_end);
		
		InlinePropertyContainer::non_null_ptr_type property_container =
				InlinePropertyContainer::create(property_name, property_value, xml_attributes);

		DummyTransactionHandle transaction(__FILE__, __LINE__);
		feature->append_property_container(property_container, transaction);
		transaction.commit();

		return property_container;
	}
}

#endif  // GPLATES_MODEL_MODELUTILS_H
