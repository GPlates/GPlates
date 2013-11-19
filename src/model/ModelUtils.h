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

#include "FeatureCollectionHandle.h"
#include "FeatureHandle.h"
#include "FeatureId.h"
#include "FeatureType.h"
#include "ModelInterface.h"
#include "PropertyName.h"
#include "PropertyValue.h"
#include "TopLevelPropertyInline.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/StructuralType.h"
#include "property-values/GpmlKeyValueDictionary.h"

#include "global/InvalidFeatureCollectionException.h"
#include "global/InvalidParametersException.h"


namespace GPlatesModel
{
	class Gpgim;
	class GpgimProperty;

	namespace ModelUtils
	{

		//
		// Creation functions for individual property values.
		//


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


		//
		// Creation functions for total reconstruction poles.
		//


		// Note: Consider adding functions as member functions in one of the Handle classes instead.

		struct TotalReconstructionPole
		{
			double time;
			double lat_of_euler_pole;
			double lon_of_euler_pole;
			double rotation_angle;
			QString comment;
		};

		const TopLevelProperty::non_null_ptr_type
		create_total_reconstruction_pole(
				const std::vector<TotalReconstructionPole> &five_tuples,
				bool is_grot = false);


		const FeatureHandle::weak_ref
		create_total_recon_seq(
				ModelInterface &model,
				const FeatureCollectionHandle::weak_ref &target_collection,
				unsigned long fixed_plate_id,
				unsigned long moving_plate_id,
				const std::vector<TotalReconstructionPole> &five_tuples);


		GPlatesPropertyValues::GpmlTimeSample
		create_gml_time_sample(
				const GPlatesModel::ModelUtils::TotalReconstructionPole &trp,
				bool is_grot = false);
		//
		// Time-dependent property value wrapper functions.
		//


		namespace TimeDependentError
		{
			enum Type
			{
				COULD_NOT_WRAP_INTO_A_TIME_DEPENDENT_PROPERTY,
				COULD_NOT_UNWRAP_EXISTING_TIME_DEPENDENT_PROPERTY,
				COULD_NOT_CONVERT_FROM_ONE_TIME_DEPENDENT_WRAPPER_TO_ANOTHER,

				NUM_ERRORS // Must be last.
			};
		}

		/**
		 * Returns the error message string associated with the specified error code.
		 *
		 * NOTE: This might not normally go into app-logic code but it's easier to keep the error
		 * messages in-sync with the error codes. And it doesn't involve any presentation/view logic.
		 */
		const char *
		get_error_message(
				TimeDependentError::Type error_code);


		/**
		 * Returns the non-time-dependent structural type of the specified top-level property.
		 *
		 * If the specified property value is a time-dependent property then its *template* type is returned.
		 * If the specified property value is *not* a time-dependent property then its *direct* type is returned.
		 */
		GPlatesPropertyValues::StructuralType
		get_non_time_dependent_property_structural_type(
				const PropertyValue &property_value);


		/**
		 * Attempts to add, remove or convert a time-dependent wrapper for the specified
		 * property value as dictated by the GPGIM (@a gpgim_property).
		 *
		 * Returns boost::none if unable do any of the following (as dictated by GPGIM):
		 *  (1) wrap in a GPGIM-mandated time-dependent property,
		 *  (2) unwrap from an existing time-dependent wrapper, or
		 *  (3) convert from existing time-dependent wrapper to a GPGIM-mandated time-dependent wrapper.
		 *
		 * For example, if the property value is a 'gpml:IrregularSampling' and the GPGIM mandates
		 * that there should be no time-dependent wrapper (for the type wrapped inside the
		 * 'gpml:IrregularSampling') then a (constant) property value cannot be unwrapped from
		 * an irregular sampling.
		 *
		 * If no conversion is necessary then the specified property value is returned.
		 */
		boost::optional<PropertyValue::non_null_ptr_type>
		add_remove_or_convert_time_dependent_wrapper(
				const PropertyValue::non_null_ptr_type &property_value,
				const GpgimProperty &gpgim_property,
				TimeDependentError::Type *error_code = NULL);


		/**
		 * Wraps a regular property value in a 'gpml:GpmlConstantValue' property value.
		 */
		const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
		create_gpml_constant_value(
				const PropertyValue::non_null_ptr_type &property_value);


		/**
		 * Wraps a 'gpml:ConstantValue' property value in a 'gpml:PiecewiseAggregation' property value.
		 *
		 * The piecewise aggregation will contain a single time period that spans distant past to distant future.
		 */
		const GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
		create_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type &constant_value_property_value);


		//
		// Top-level property functions.
		//


		namespace TopLevelPropertyError
		{
			enum Type
			{
				NOT_ONE_PROPERTY_VALUE,
				NOT_TOP_LEVEL_PROPERTY_INLINE,
				PROPERTY_NAME_NOT_RECOGNISED, // Not recognised by GPGIM.
				PROPERTY_NAME_NOT_SUPPORTED_BY_FEATURE_TYPE,
				COULD_NOT_WRAP_INTO_A_TIME_DEPENDENT_PROPERTY,
				COULD_NOT_UNWRAP_EXISTING_TIME_DEPENDENT_PROPERTY,
				COULD_NOT_CONVERT_FROM_ONE_TIME_DEPENDENT_WRAPPER_TO_ANOTHER,

				NUM_ERRORS // Must be last.
			};
		}

		/**
		 * Returns the error message string associated with the specified error code.
		 *
		 * NOTE: This might not normally go into app-logic code but it's easier to keep the error
		 * messages in-sync with the error codes. And it doesn't involve any presentation/view logic.
		 */
		const char *
		get_error_message(
				TopLevelPropertyError::Type error_code);


		/**
		 * Returns the specified top-level property as an *inline* top-level property.
		 *
		 * NOTE: Currently the only type of top-level property is inline so this should not fail.
		 */
		boost::optional<const TopLevelPropertyInline &>
		get_top_level_property_inline(
				const TopLevelProperty &top_level_property,
				TopLevelPropertyError::Type *error_code = NULL);


		/**
		 * Returns the property value of the specified top-level property.
		 *
		 * NOTE: Currently the only type of top-level property is inline so this should not fail.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type>
		get_property_value(
				const TopLevelProperty &top_level_property,
				TopLevelPropertyError::Type *error_code = NULL);

		/**
		 * Returns the TopLevelPropertyRef(s) of the given name in a feature.
		 */
		std::vector<FeatureHandle::iterator>
		get_top_level_property_ref(
				const PropertyName& name,
				GPlatesModel::FeatureHandle::weak_ref feature);


		/**
		 * Creates a TopLevelPropertyInline from the specified property value.
		 *
		 * The property value is also converted, to a new property value if necessary, that has the
		 * correct time-dependent wrapper (or none) as dictated by the GPGIM for the specified
		 * property name (this is done via @a add_remove_or_convert_time_dependent_wrapper).
		 *
		 * If a feature type is specified then the property name is also checked to see if it's valid
		 * for the specified feature type. This ensures a stricter level of conformance to the GPGIM.
		 *
		 * Returns boost::none if any error is encountered (such as an unrecognised property name or inability
		 * to convert time-dependent wrapper).
		 * The error code can optionally be returned via @a error_code.
		 */
		boost::optional<TopLevelProperty::non_null_ptr_type>
		create_top_level_property(
				const PropertyName& property_name,
				const PropertyValue::non_null_ptr_type &property_value,
				const Gpgim &gpgim,
				boost::optional<FeatureType> feature_type = boost::none,
				TopLevelPropertyError::Type *error_code = NULL);


		/**
		 * An overload of @a create_top_level_property for when the GPGIM property has already been
		 * determined by the caller (from the property name).
		 *
		 * Note that the caller is responsible for ensuring that the specified GPGIM property is allowed
		 * for the feature type of the feature it will subsequently be added to (according to the GPGIM).
		 */
		boost::optional<TopLevelProperty::non_null_ptr_type>
		create_top_level_property(
				const GpgimProperty &gpgim_property,
				const PropertyValue::non_null_ptr_type &property_value,
				TopLevelPropertyError::Type *error_code = NULL);


		/**
		 * Creates a TopLevelPropertyInline from the specified property value and
		 * adds it into the specified feature.
		 *
		 * The property value is also converted, if necessary, to have the correct time-dependent
		 * wrapper (or none) as dictated by the GPGIM for the specified property name
		 * (this is done via @a add_remove_or_convert_time_dependent_wrapper).
		 *
		 * NOTE: If @a check_property_name_with_feature_type is true then the property name is also
		 * checked to see if it's valid for the specified feature's type (and only added if it is).
		 * This ensures a stricter level of conformance to the GPGIM.
		 *
		 * Returns false if any error is encountered (such as an unrecognised property name or inability
		 * to convert time-dependent wrapper) in which case the property value is not added to the feature.
		 * The error code can optionally be returned via @a error_code.
		 */
		bool
		add_property(
				const FeatureHandle::weak_ref &feature,
				const PropertyName& property_name,
				const PropertyValue::non_null_ptr_type &property_value,
				const Gpgim &gpgim,
				bool check_property_name_allowed_for_feature_type = true,
				TopLevelPropertyError::Type *error_code = NULL);


		/**
		 * An overload of @a add_property for when the GPGIM property has already been
		 * determined by the caller (from the property name).
		 *
		 * Note that the caller is responsible for ensuring that the specified GPGIM property
		 * is allowed for the feature's type (according to the GPGIM).
		 */
		bool
		add_property(
				const FeatureHandle::weak_ref &feature,
				const GpgimProperty &gpgim_property,
				const PropertyValue::non_null_ptr_type &property_value,
				TopLevelPropertyError::Type *error_code = NULL);


		/**
		 * Creates a TopLevelPropertyInline from the specified property value and
		 * adds it into the specified feature.
		 *
		 * NOTE: This does not check against the GPGIM or convert the time-dependent wrapper (if any)
		 * by querying the GPGIM. See the other overload of @a add_property for that.
		 */
		inline
		void
		add_property(
				const FeatureHandle::weak_ref &feature,
				const PropertyName& property_name,
				const PropertyValue::non_null_ptr_type &property_value)
		{
			feature->add(TopLevelPropertyInline::create(property_name, property_value));
		}


		/*
		* Given the feature reference, 
		* return the MPRS(Moving Plate Rotation Sequence) metadata as a GpmlKeyValueDictionary.
		*/
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
		get_mprs_attributes(
				FeatureHandle::const_weak_ref f);


		/**
		 * Renames all properties of @a feature, with property name matching @a old_property_name,
		 * to property name @a new_property_name.
		 *
		 * Properties with a name not matching @a old_property_name are left alone.
		 *
		 * The GPGIM, @a gpgim, is used to determine the time-dependent wrapper (if any) required
		 * for the property name @a new_property_name.
		 *
		 * NOTE: If @a check_new_property_name_allowed_for_feature_type is true then the new property
		 * name is also checked to see if it's valid for the specified feature's type.
		 * This ensures a stricter level of conformance to the GPGIM.
		 *
		 * Returns false if any error is encountered in which case *none* of the properties are renamed.
		 * The error can optionally be returned via @a error_code.
		 */
		bool
		rename_feature_properties(
				FeatureHandle &feature,
				const PropertyName &old_property_name,
				const PropertyName &new_property_name,
				const Gpgim &gpgim,
				bool check_new_property_name_allowed_for_feature_type = true,
				TopLevelPropertyError::Type *error_code = NULL);


		/**
		 * Takes an existing @a top_level_property and returns a new top-level
		 * property object with the same property value (aside from time-dependent differences)
		 * as @a top_level_property but with the @a new_property_name.
		 *
		 * The GPGIM, @a gpgim, is used to determine the time-dependent wrapper (if any) required
		 * for the specified property name.
		 */
		boost::optional<TopLevelProperty::non_null_ptr_type>
		rename_property(
				const TopLevelProperty &top_level_property,
				const PropertyName &new_property_name,
				const Gpgim &gpgim,
				TopLevelPropertyError::Type *error_code = NULL);

		/**
		 * An overload of @a rename_property for when the GPGIM property has already been
		 * determined by the caller (from the new property name).
		 */
		boost::optional<TopLevelProperty::non_null_ptr_type>
		rename_property(
				const TopLevelProperty &top_level_property,
				const GpgimProperty &new_gpgim_property,
				TopLevelPropertyError::Type *error_code = NULL);
		
		/*
		* Find the FeatureHandle weak ref given the feature id as GPlatesModel::FeatureId.
		*/
		GPlatesModel::FeatureHandle::weak_ref
		find_feature(
				const GPlatesModel::FeatureId &id);

		/*
		* Find the FeatureHandle weak ref given the feature id as QString.
		*/
		inline
		GPlatesModel::FeatureHandle::weak_ref
		find_feature(
				const QString &id)
		{
			return find_feature(GPlatesModel::FeatureId(GPlatesUtils::UnicodeString(id)));
		}
	}
}

#endif  // GPLATES_MODEL_MODELUTILS_H





