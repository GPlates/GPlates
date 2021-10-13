/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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

#ifndef GPLATES_MODEL_MODELUTILS_H
#define GPLATES_MODEL_MODELUTILS_H

#include <boost/optional.hpp>

#include "FeatureHandle.h"
#include "FeatureCollectionHandle.h"
#include "ModelInterface.h"
#include "PropertyName.h"
#include "PropertyValue.h"
#include "TopLevelPropertyInline.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/TemplateTypeParameterType.h"

#include "global/InvalidFeatureCollectionException.h"
#include "global/InvalidParametersException.h"


namespace GPlatesModel
{
	namespace ModelUtils
	{
		// Note: Consider adding functions as member functions in one of the Handle classes instead.

		struct TotalReconstructionPoleData
		{
			double time;
			double lat_of_euler_pole;
			double lon_of_euler_pole;
			double rotation_angle;
			const char *comment;
		};


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


		namespace RenameGeometricPropertyError
		{
			enum Type
			{
				NOT_ONE_PROPERTY_VALUE,
				NOT_GEOMETRY,
				COULD_NOT_UNWRAP_TIME_DEPENDENT_PROPERTY,
				NOT_TOP_LEVEL_PROPERTY_INLINE,

				NUM_ERRORS // Must be last.
			};
		}


		/**
		 * Takes an existing geometric @a top_level_property and returns a new top-level
		 * property object with the same geometry as @a top_level_property but with
		 * the @a new_property_name.
		 */
		boost::optional<TopLevelProperty::non_null_ptr_type>
		rename_geometric_property(
				const TopLevelProperty &top_level_property,
				const PropertyName &new_property_name,
				RenameGeometricPropertyError::Type *error_code = NULL);


		// Before this line are the new, hopefully-better-designed functions; after this
		// line are the old, arbitrary functions which should probably be reviewed (and
		// should quite possibly be refactored).
		// FIXME:  Review the following functions and refactor if necessary.

		const TopLevelProperty::non_null_ptr_type
		create_total_reconstruction_pole(
				const std::vector<TotalReconstructionPoleData> &five_tuples);
	
		const FeatureHandle::weak_ref
		create_total_recon_seq(
				ModelInterface &model,
				const FeatureCollectionHandle::weak_ref &target_collection,
				unsigned long fixed_plate_id,
				unsigned long moving_plate_id,
				const std::vector<TotalReconstructionPoleData> &five_tuples);

		/*
		* Convenient function to create TopLevelPropertyInline 
		*/
		template<class PropertyValueType, class DataType>
		inline
		TopLevelPropertyInline::non_null_ptr_type
		create_toplevel_property(
				const PropertyName& property_name,
				const DataType& data)
		{
			return TopLevelPropertyInline::create(
					property_name,
					PropertyValueType::create(data));
		}

		/*
		* Convenient function to add TopLevelPropertyInline into feature. 
		*/
		template<class PropertyValueType, class DataType>
		inline
		void
		add_property(
				FeatureHandle::weak_ref feature,
				const PropertyName& property_name,
				const DataType& data)
		{
			feature->add(create_toplevel_property<PropertyValueType>(property_name, data));
		}

		/*
		* Convenient function to add TopLevelPropertyInline into feature. 
		*/
		inline
		void
		add_property(
				FeatureHandle::weak_ref feature,
				const PropertyName& property_name,
				GPlatesModel::PropertyValue::non_null_ptr_type value)
		{
			feature->add(TopLevelPropertyInline::create(property_name, value));
		}
	}
}

#endif  // GPLATES_MODEL_MODELUTILS_H
