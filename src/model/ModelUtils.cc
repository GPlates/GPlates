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

#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/static_assert.hpp>
#include <QtGlobal>

#include "ModelUtils.h"

#include "FeatureHandleWeakRefBackInserter.h"
#include "Gpgim.h"
#include "GpgimProperty.h"
#include "Model.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "global/LogException.h"

#include "presentation/Application.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/StructuralType.h"

namespace
{
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
	add_remove_or_convert_time_dependent_wrapper(
			const GPlatesModel::PropertyValue::non_null_ptr_type &property_value,
			const GPlatesModel::GpgimProperty &gpgim_property,
			GPlatesModel::ModelUtils::TopLevelPropertyError::Type *error_code)
	{
		// Add or remove the time-dependent wrapper as dictated by the GPGIM.
		GPlatesModel::ModelUtils::TimeDependentError::Type time_dependent_error_code;
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> converted_property_value =
				GPlatesModel::ModelUtils::add_remove_or_convert_time_dependent_wrapper(
						property_value, gpgim_property, &time_dependent_error_code);
		if (converted_property_value)
		{
			return converted_property_value.get();
		}

		if (error_code)
		{
			if (time_dependent_error_code ==
				GPlatesModel::ModelUtils::TimeDependentError::COULD_NOT_WRAP_INTO_A_TIME_DEPENDENT_PROPERTY)
			{
				*error_code = GPlatesModel::ModelUtils::TopLevelPropertyError::COULD_NOT_WRAP_INTO_A_TIME_DEPENDENT_PROPERTY;
			}
			else if (time_dependent_error_code ==
				GPlatesModel::ModelUtils::TimeDependentError::COULD_NOT_UNWRAP_EXISTING_TIME_DEPENDENT_PROPERTY)
			{
				*error_code = GPlatesModel::ModelUtils::TopLevelPropertyError::COULD_NOT_UNWRAP_EXISTING_TIME_DEPENDENT_PROPERTY;
			}
			else
			{
				*error_code = GPlatesModel::ModelUtils::TopLevelPropertyError::COULD_NOT_CONVERT_FROM_ONE_TIME_DEPENDENT_WRAPPER_TO_ANOTHER;
			}
		}

		return boost::none;
	}


	boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type>
	get_gpgim_property(
			boost::optional<GPlatesModel::FeatureType> feature_type,
			const GPlatesModel::PropertyName& property_name,
			const GPlatesModel::Gpgim &gpgim,
			GPlatesModel::ModelUtils::TopLevelPropertyError::Type *error_code)
	{
		// Get the GPGIM property using the property name (and optionally the feature type).
		// Using the feature type results in stricter conformance to the GPGIM.
		boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> gpgim_property =
				feature_type
				? gpgim.get_feature_property(feature_type.get(), property_name)
				: gpgim.get_property(property_name);
		if (gpgim_property)
		{
			return gpgim_property.get();
		}

		if (error_code)
		{
			if (feature_type)
			{
				// If we checked against the feature type then the failure could just be that
				// the property name wasn't a name recognised for *any* feature type - we give
				// preference to that error message (if that's the case here).
				*error_code = gpgim.get_property(property_name)
						? GPlatesModel::ModelUtils::TopLevelPropertyError::PROPERTY_NAME_NOT_RECOGNISED
						// Property name was recognised, but not supported by the feature type...
						: GPlatesModel::ModelUtils::TopLevelPropertyError::PROPERTY_NAME_NOT_SUPPORTED_BY_FEATURE_TYPE;
			}
			else
			{
				*error_code = GPlatesModel::ModelUtils::TopLevelPropertyError::PROPERTY_NAME_NOT_RECOGNISED;
			}
		}

		return boost::none;
	}
}


const char *
GPlatesModel::ModelUtils::get_error_message(
		TopLevelPropertyError::Type error_code)
{
	static const char *const ERROR_MESSAGES[] =
	{
		QT_TR_NOOP("GPlates cannot change the property name of a top-level property that does not have exactly one property value."),
		QT_TR_NOOP("GPlates cannot change the property name of a top-level property that is not inline."),
		QT_TR_NOOP("The property name was not recognised as a valid name by the GPGIM."),
		QT_TR_NOOP("The property name is not in the feature type's list of valid names."),
		QT_TR_NOOP("GPlates was unable to wrap into a time-dependent property."),
		QT_TR_NOOP("GPlates was unable to unwrap the existing time-dependent property."),
		QT_TR_NOOP("GPlates was unable to convert from one time-dependent wrapper to another.")
	};
	BOOST_STATIC_ASSERT(sizeof(ERROR_MESSAGES) / sizeof(const char *) == TopLevelPropertyError::NUM_ERRORS);

	return ERROR_MESSAGES[static_cast<unsigned int>(error_code)];
}


boost::optional<const GPlatesModel::TopLevelPropertyInline &>
GPlatesModel::ModelUtils::get_top_level_property_inline(
		const TopLevelProperty &top_level_property,
		TopLevelPropertyError::Type *error_code)
{
	try
	{
		const TopLevelPropertyInline &tlpi =
				dynamic_cast<const TopLevelPropertyInline &>(top_level_property);
		if (tlpi.size() != 1)
		{
			if (error_code)
			{
				*error_code = TopLevelPropertyError::NOT_ONE_PROPERTY_VALUE;
			}
			return boost::none;
		}

		return tlpi;
	}
	catch (const std::bad_cast &)
	{
		if (error_code)
		{
			*error_code = TopLevelPropertyError::NOT_TOP_LEVEL_PROPERTY_INLINE;
		}
	}

	return boost::none;
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type>
GPlatesModel::ModelUtils::get_property_value(
		const TopLevelProperty &top_level_property,
		TopLevelPropertyError::Type *error_code)
{
	boost::optional<const TopLevelPropertyInline &> tlpi =
			get_top_level_property_inline(top_level_property, error_code);
	if (!tlpi)
	{
		return boost::none;
	}

	const PropertyValue::non_null_ptr_to_const_type property_value = *tlpi->begin();

	return property_value;
}


std::vector<GPlatesModel::FeatureHandle::iterator>
GPlatesModel::ModelUtils::get_top_level_property_ref(
		const PropertyName& name,
		FeatureHandle::weak_ref feature)
{
	std::vector<FeatureHandle::iterator> ret;
	if(feature.is_valid())
	{
		FeatureHandle::iterator it = feature->begin();
		for(;it != feature->end(); it++)
		{
			if((*it)->property_name() == name)
			{
				ret.push_back(it);
			}
		}
	}
	return ret;
}


boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
GPlatesModel::ModelUtils::create_top_level_property(
		const PropertyName& property_name,
		const PropertyValue::non_null_ptr_type &property_value,
		const Gpgim &gpgim,
		boost::optional<FeatureType> feature_type,
		TopLevelPropertyError::Type *error_code)
{
	// Get the GPGIM property using the property name (and optionally the feature type).
	// Using the feature type results in stricter conformance to the GPGIM.
	boost::optional<GpgimProperty::non_null_ptr_to_const_type> gpgim_property =
			get_gpgim_property(
					feature_type,
					property_name,
					gpgim,
					error_code);
	if (!gpgim_property)
	{
		return boost::none;
	}

	return create_top_level_property(
			*gpgim_property.get(),
			property_value,
			error_code);
}


boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
GPlatesModel::ModelUtils::create_top_level_property(
		const GpgimProperty &gpgim_property,
		const PropertyValue::non_null_ptr_type &property_value,
		TopLevelPropertyError::Type *error_code)
{
	// Make sure property value has correct time-dependent wrapper (or none).
	boost::optional<PropertyValue::non_null_ptr_type> converted_property_value =
			::add_remove_or_convert_time_dependent_wrapper(
					property_value, gpgim_property, error_code);
	if (!converted_property_value)
	{
		return boost::none;
	}

	return TopLevelProperty::non_null_ptr_type(
			TopLevelPropertyInline::create(
					gpgim_property.get_property_name(),
					converted_property_value.get()));
}


bool
GPlatesModel::ModelUtils::add_property(
		const FeatureHandle::weak_ref &feature,
		const PropertyName& property_name,
		const PropertyValue::non_null_ptr_type &property_value,
		const Gpgim &gpgim,
		bool check_property_name_allowed_for_feature_type,
		TopLevelPropertyError::Type *error_code)
{
	boost::optional<FeatureType> feature_type;
	if (check_property_name_allowed_for_feature_type)
	{
		feature_type = feature->feature_type();
	}

	boost::optional<TopLevelProperty::non_null_ptr_type> top_level_property =
			create_top_level_property(
					property_name,
					property_value,
					gpgim,
					feature_type,
					error_code);
	if (!top_level_property)
	{
		return false;
	}

	// Add the converted property value to the feature.
	feature->add(top_level_property.get());

	return true;
}


bool
GPlatesModel::ModelUtils::add_property(
		const FeatureHandle::weak_ref &feature,
		const GpgimProperty &gpgim_property,
		const PropertyValue::non_null_ptr_type &property_value,
		TopLevelPropertyError::Type *error_code)
{
	boost::optional<TopLevelProperty::non_null_ptr_type> top_level_property =
			create_top_level_property(
					gpgim_property,
					property_value,
					error_code);
	if (!top_level_property)
	{
		return false;
	}

	// Add the converted property value to the feature.
	feature->add(top_level_property.get());

	return true;
}


bool
GPlatesModel::ModelUtils::rename_feature_properties(
		FeatureHandle &feature,
		const PropertyName &old_property_name,
		const PropertyName &new_property_name,
		const Gpgim &gpgim,
		bool check_new_property_name_allowed_for_feature_type,
		boost::optional< std::vector<FeatureHandle::iterator> &> renamed_feature_properties,
		TopLevelPropertyError::Type *error_code)
{
	boost::optional<FeatureType> feature_type;
	if (check_new_property_name_allowed_for_feature_type)
	{
		feature_type = feature.feature_type();
	}

	// Get the new GPGIM property using the new property name (and optionally the feature type).
	// Using the feature type results in stricter conformance to the GPGIM.
	boost::optional<GpgimProperty::non_null_ptr_to_const_type> new_gpgim_property =
			get_gpgim_property(
					feature_type,
					new_property_name,
					gpgim,
					error_code);
	if (!new_gpgim_property)
	{
		return false;
	}

	typedef std::pair<FeatureHandle::iterator/*old*/, TopLevelProperty::non_null_ptr_type/*new*/>
			renamed_property_type;
	std::vector<renamed_property_type> renamed_top_level_properties;

	// Iterate over the feature properties and create a renamed property for each matching property name.
	FeatureHandle::iterator properties_iter = feature.begin();
	FeatureHandle::iterator properties_end = feature.end();
	for ( ; properties_iter != properties_end; ++properties_iter)
	{
		if ((*properties_iter)->property_name() == old_property_name)
		{
			// We can't actually rename a (top-level) property of a feature.
			// So we need to create a new top-level property and remove the existing one.

			// Get existing top-level property.
			const TopLevelPropertyInline &top_level_property =
					dynamic_cast<const TopLevelPropertyInline &>(**properties_iter);

			// Create the renamed top-level property.
			boost::optional<TopLevelProperty::non_null_ptr_type> renamed_top_level_property =
					rename_property(
							top_level_property,
							*new_gpgim_property.get(),
							error_code);
			if (!renamed_top_level_property)
			{
				// Return without having renamed any feature properties.
				return false;
			}

			// Add it to the list of renamed top-level properties.
			renamed_top_level_properties.push_back(
					renamed_property_type(properties_iter, renamed_top_level_property.get()));
		}
	}

	// Add the renamed properties to the feature (and remove the old properties).
	BOOST_FOREACH(
			const renamed_property_type &renamed_top_level_property,
			renamed_top_level_properties)
	{
		// Remove old property.
		feature.remove(renamed_top_level_property.first);

		// Add renamed property.
		const FeatureHandle::iterator renamed_feature_property =
				feature.add(renamed_top_level_property.second);

		// Notify caller of renamed properties if requested.
		if (renamed_feature_properties)
		{
			renamed_feature_properties->push_back(renamed_feature_property);
		}
	}

	return true;
}


boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
GPlatesModel::ModelUtils::rename_property(
		const TopLevelProperty &top_level_property,
		const PropertyName &new_property_name,
		const Gpgim &gpgim,
		TopLevelPropertyError::Type *error_code)
{
	boost::optional<GpgimProperty::non_null_ptr_to_const_type> new_gpgim_property =
			gpgim.get_property(new_property_name);
	if (!new_gpgim_property)
	{
		if (error_code)
		{
			*error_code = TopLevelPropertyError::PROPERTY_NAME_NOT_RECOGNISED;
		}
		return boost::none;
	}

	return rename_property(top_level_property, *new_gpgim_property.get(), error_code);
}


boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
GPlatesModel::ModelUtils::rename_property(
		const TopLevelProperty &top_level_property,
		const GpgimProperty &new_gpgim_property,
		TopLevelPropertyError::Type *error_code)
{
	boost::optional<const TopLevelPropertyInline &> tlpi =
			get_top_level_property_inline(top_level_property, error_code);
	if (!tlpi)
	{
		return boost::none;
	}

	// Clone property value to convert from 'const' to 'non-const' which we need
	// when creating a new top-level property.
	PropertyValue::non_null_ptr_type property_value = (*tlpi->begin())->deep_clone_as_prop_val();

	// Add or remove the time-dependent wrapper as dictated by the GPGIM.
	boost::optional<PropertyValue::non_null_ptr_type> converted_property_value =
			::add_remove_or_convert_time_dependent_wrapper(
					property_value, new_gpgim_property, error_code);
	if (!converted_property_value)
	{
		return boost::none;
	}

	return TopLevelProperty::non_null_ptr_type(
			TopLevelPropertyInline::create(
					new_gpgim_property.get_property_name(),
					converted_property_value.get(),
					tlpi->xml_attributes()));
}


const char *
GPlatesModel::ModelUtils::get_error_message(
		TimeDependentError::Type error_code)
{
	static const char *const ERROR_MESSAGES[] =
	{
		QT_TR_NOOP("GPlates was unable to wrap into a time-dependent property."),
		QT_TR_NOOP("GPlates was unable to unwrap the existing time-dependent property."),
		QT_TR_NOOP("GPlates was unable to convert from one time-dependent wrapper to another.")
	};
	BOOST_STATIC_ASSERT(sizeof(ERROR_MESSAGES) / sizeof(const char *) == TimeDependentError::NUM_ERRORS);

	return ERROR_MESSAGES[static_cast<unsigned int>(error_code)];
}


GPlatesPropertyValues::StructuralType
GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(
		const PropertyValue &property_value)
{
	// The time-dependent property values are template types.
	static const GPlatesPropertyValues::StructuralType CONSTANT_VALUE_TYPE =
			GPlatesPropertyValues::StructuralType::create_gpml("ConstantValue");
	static const GPlatesPropertyValues::StructuralType IRREGULAR_SAMPLING_TYPE =
			GPlatesPropertyValues::StructuralType::create_gpml("IrregularSampling");
	static const GPlatesPropertyValues::StructuralType PIECEWISE_AGGREGATION_TYPE =
			GPlatesPropertyValues::StructuralType::create_gpml("PiecewiseAggregation");

	const GPlatesPropertyValues::StructuralType structural_type = property_value.get_structural_type();

	if (structural_type == CONSTANT_VALUE_TYPE)
	{
		return dynamic_cast<const GPlatesPropertyValues::GpmlConstantValue &>(property_value).value_type();
	}
	if (structural_type == IRREGULAR_SAMPLING_TYPE)
	{
		return dynamic_cast<const GPlatesPropertyValues::GpmlIrregularSampling &>(property_value).value_type();
	}
	if (structural_type == PIECEWISE_AGGREGATION_TYPE)
	{
		return dynamic_cast<const GPlatesPropertyValues::GpmlPiecewiseAggregation &>(property_value).value_type();
	}

	return structural_type;
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesModel::ModelUtils::add_remove_or_convert_time_dependent_wrapper(
		const PropertyValue::non_null_ptr_type &property_value,
		const GpgimProperty &gpgim_property,
		TimeDependentError::Type *error_code)
{
	// The time-dependent property value types.
	static const GPlatesPropertyValues::StructuralType CONSTANT_VALUE_TYPE =
			GPlatesPropertyValues::StructuralType::create_gpml("ConstantValue");
	static const GPlatesPropertyValues::StructuralType IRREGULAR_SAMPLING_TYPE =
			GPlatesPropertyValues::StructuralType::create_gpml("IrregularSampling");
	static const GPlatesPropertyValues::StructuralType PIECEWISE_AGGREGATION_TYPE =
			GPlatesPropertyValues::StructuralType::create_gpml("PiecewiseAggregation");

	const GpgimProperty::time_dependent_flags_type &time_dependent_flags =
			gpgim_property.get_time_dependent_types();

	const GPlatesPropertyValues::StructuralType structural_type = property_value->get_structural_type();

	if (structural_type == CONSTANT_VALUE_TYPE)
	{
		// If the GPGIM specifies a constant-value wrapped property then just return
		// the property value since it's already constant-value wrapped.
		if (time_dependent_flags.test(GpgimProperty::CONSTANT_VALUE))
		{
			return property_value;
		}

		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type gpml_constant_value =
				GPlatesUtils::dynamic_pointer_cast<GPlatesPropertyValues::GpmlConstantValue>(
						property_value);

		// Wrap it in a piecewise-aggregation if the GPGIM allows this.
		if (time_dependent_flags.test(GpgimProperty::PIECEWISE_AGGREGATION))
		{
			return PropertyValue::non_null_ptr_type(create_gpml_piecewise_aggregation(gpml_constant_value));
		}

		// If the GPGIM specifies a non-time-dependent property then unwrap the property value.
		if (!time_dependent_flags.any())
		{
			return gpml_constant_value->value();
		}

		// ...else we cannot convert a constant-value property to an irregularly-sampled property.
		if (error_code)
		{
			*error_code = TimeDependentError::COULD_NOT_CONVERT_FROM_ONE_TIME_DEPENDENT_WRAPPER_TO_ANOTHER;
		}
	}
	else if (structural_type == IRREGULAR_SAMPLING_TYPE)
	{
		// If the GPGIM specifies an irregular-sampled property then just return
		// the property value since it's already irregular-sampled.
		if (time_dependent_flags.test(GpgimProperty::IRREGULAR_SAMPLING))
		{
			return property_value;
		}

		// ...else we cannot convert an irregularly-sampled property to any other time-dependent
		// wrapper, or to an unwrapped property.
		if (error_code)
		{
			*error_code = time_dependent_flags.any()
					? TimeDependentError::COULD_NOT_CONVERT_FROM_ONE_TIME_DEPENDENT_WRAPPER_TO_ANOTHER
					: TimeDependentError::COULD_NOT_UNWRAP_EXISTING_TIME_DEPENDENT_PROPERTY;
		}
	}
	else if (structural_type == PIECEWISE_AGGREGATION_TYPE)
	{
		// If the GPGIM specifies a piecewise-aggregation property then just return
		// the property value since it's already piecewise-aggregated.
		if (time_dependent_flags.test(GpgimProperty::PIECEWISE_AGGREGATION))
		{
			return property_value;
		}

		GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type gpml_piecewise_aggregation =
				GPlatesUtils::dynamic_pointer_cast<GPlatesPropertyValues::GpmlPiecewiseAggregation>(
						property_value);

		// If the GPGIM specifies a constant value then see if the piecewise aggregation actually
		// contains a single time window with a constant value.
		if (time_dependent_flags.test(GpgimProperty::CONSTANT_VALUE) ||
			!time_dependent_flags.any())
		{
			std::vector<GPlatesPropertyValues::GpmlTimeWindow> &time_windows =
					gpml_piecewise_aggregation->time_windows();

			// If the there's a single time window that covers all time and it's a constant-value...
			if (time_windows.size() == 1 &&
				time_windows.front().valid_time()->begin()->time_position().is_distant_past() &&
				time_windows.front().valid_time()->end()->time_position().is_distant_future() &&
				time_windows.front().time_dependent_value()->get_structural_type() == CONSTANT_VALUE_TYPE)
			{
				GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type gpml_constant_value =
						GPlatesUtils::dynamic_pointer_cast<GPlatesPropertyValues::GpmlConstantValue>(
								time_windows.front().time_dependent_value());

				// Return the constant-value wrapped property value if the GPGIM allows this.
				if (time_dependent_flags.test(GpgimProperty::CONSTANT_VALUE))
				{
					return PropertyValue::non_null_ptr_type(gpml_constant_value);
				}

				// If the GPGIM specifies a non-time-dependent property then unwrap the property value.
				if (!time_dependent_flags.any())
				{
					return gpml_constant_value->value();
				}
			}
		}

		// ...else we cannot convert a piecewise-aggregated property to any other time-dependent
		// wrapper, or to an unwrapped property.
		if (error_code)
		{
			*error_code = time_dependent_flags.any()
					? TimeDependentError::COULD_NOT_CONVERT_FROM_ONE_TIME_DEPENDENT_WRAPPER_TO_ANOTHER
					: TimeDependentError::COULD_NOT_UNWRAP_EXISTING_TIME_DEPENDENT_PROPERTY;
		}
	}
	else // Not a time-dependent property value type...
	{
		// If the GPGIM specifies a non-time-dependent property then just return the property value.
		if (!time_dependent_flags.any())
		{
			return property_value;
		}

		// Wrap it in a constant-value if the GPGIM allows this.
		if (time_dependent_flags.test(GpgimProperty::CONSTANT_VALUE))
		{
			return PropertyValue::non_null_ptr_type(create_gpml_constant_value(property_value));
		}

		// Wrap it in a piecewise-aggregation if the GPGIM allows this.
		if (time_dependent_flags.test(GpgimProperty::PIECEWISE_AGGREGATION))
		{
			return PropertyValue::non_null_ptr_type(
					create_gpml_piecewise_aggregation(
							create_gpml_constant_value(property_value)));
		}

		// Else it's an irregular sampling and we can't wrap a property in that.
		if (error_code)
		{
			*error_code = TimeDependentError::COULD_NOT_WRAP_INTO_A_TIME_DEPENDENT_PROPERTY;
		}
	}

	// Unable to either add a time-dependent wrapper or remove one.
	return boost::none;
}


const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
GPlatesModel::ModelUtils::create_gpml_constant_value(
		const PropertyValue::non_null_ptr_type &property_value)
{
	const GPlatesPropertyValues::StructuralType structural_type = property_value->get_structural_type();

	return GPlatesPropertyValues::GpmlConstantValue::create(property_value, structural_type);
}


const GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
GPlatesModel::ModelUtils::create_gpml_piecewise_aggregation(
		const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type &constant_value_property_value)
{
	const GPlatesPropertyValues::StructuralType structural_type = constant_value_property_value->value_type();

	// Create a time period property that spans *all* time (distant past to distant future).
	const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type time_period =
			create_gml_time_period(
					GPlatesPropertyValues::GeoTimeInstant::create_distant_past(),
					GPlatesPropertyValues::GeoTimeInstant::create_distant_future());

	// Create the TimeWindow.
	const GPlatesPropertyValues::GpmlTimeWindow time_window =
			GPlatesPropertyValues::GpmlTimeWindow(
					constant_value_property_value,
					time_period,
					structural_type);

	// Create the TimeWindow sequence.
	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows;
	time_windows.push_back(time_window);

	// Final wrapping of the 'gpml:ConstantValue' in a 'gpml:PiecewiseAggregation'.
	return GPlatesPropertyValues::GpmlPiecewiseAggregation::create(time_windows, structural_type);
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


const GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesModel::ModelUtils::create_total_reconstruction_pole(
		const std::vector<TotalReconstructionPole> &five_tuples,
		bool is_grot)
{
	using namespace GPlatesPropertyValues;
	std::vector<GPlatesPropertyValues::GpmlTimeSample> time_samples;
	boost::optional<StructuralType> value_type;
	if(is_grot)
	{
		value_type = StructuralType::create_gpml("TotalReconstructionPole");
	}
	else
	{
		value_type = StructuralType::create_gpml("FiniteRotation");
	}

	for (std::vector<TotalReconstructionPole>::const_iterator iter = five_tuples.begin(); 
		iter != five_tuples.end(); ++iter) 
	{
		time_samples.push_back(create_gml_time_sample(*iter, is_grot));
	}

	PropertyValue::non_null_ptr_type gpml_irregular_sampling =
		GpmlIrregularSampling::create(
				time_samples,
				GPlatesUtils::get_intrusive_ptr(
						GpmlFiniteRotationSlerp::create(*value_type)), 
				*value_type);

	TopLevelProperty::non_null_ptr_type top_level_property_inline =
		TopLevelPropertyInline::create(
				PropertyName::create_gpml("totalReconstructionPole"),
				gpml_irregular_sampling, 
				std::map<XmlAttributeName, XmlAttributeValue>());

	return top_level_property_inline;
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::ModelUtils::create_total_recon_seq(
		ModelInterface &model,
		const FeatureCollectionHandle::weak_ref &target_collection,
		unsigned long fixed_plate_id,
		unsigned long moving_plate_id,
		const std::vector<TotalReconstructionPole> &five_tuples)
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


GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
GPlatesModel::ModelUtils::get_mprs_attributes(
		FeatureHandle::const_weak_ref f)
{

	static const  PropertyName mprs_attrs = PropertyName::create_gpml(QString("mprsAttributes"));
	if(f.is_valid())
	{
		const GPlatesPropertyValues::GpmlKeyValueDictionary* const_dictionary = NULL;
		FeatureHandle::const_iterator	it = f->begin();
		for(;it != f->end(); it++)
		{
			if((*it)->property_name() == mprs_attrs)
			{
				const TopLevelPropertyInline *p_inline = dynamic_cast<const TopLevelPropertyInline*>((*it).get());
				if(p_inline && p_inline->size() >= 1)
				{
					const_dictionary = 
						dynamic_cast<const GPlatesPropertyValues::GpmlKeyValueDictionary*>((*p_inline->begin()).get());
				}
			}
		}
		
		if(const_dictionary)
		{
			GPlatesPropertyValues::GpmlKeyValueDictionary* dictionary = 
				const_cast<GPlatesPropertyValues::GpmlKeyValueDictionary*>(const_dictionary);
			return GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type(dictionary);
		}
		
	}
	throw GPlatesGlobal::LogException(
		GPLATES_EXCEPTION_SOURCE,
		"Cannot find MPRS attributes.");
}


GPlatesPropertyValues::GpmlTimeSample
GPlatesModel::ModelUtils::create_gml_time_sample(
		const TotalReconstructionPole &trp,
		bool is_grot)
{
	using namespace GPlatesModel;
	using namespace GPlatesPropertyValues;

	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	std::pair<double, double> gpml_euler_pole =
		std::make_pair(trp.lon_of_euler_pole, trp.lat_of_euler_pole);
	GpmlFiniteRotation::non_null_ptr_type gpml_finite_rotation =
		GpmlFiniteRotation::create(gpml_euler_pole, trp.rotation_angle);

	GmlTimeInstant::non_null_ptr_type gml_time_instant =
		GmlTimeInstant::create(GeoTimeInstant(trp.time), xml_attributes);

	XsString::non_null_ptr_type gml_description = 
		XsString::create(GPlatesUtils::make_icu_string_from_qstring(trp.comment));

	if(is_grot)
	{
		return GpmlTimeSample(
				GpmlTotalReconstructionPole::non_null_ptr_type(
						new GpmlTotalReconstructionPole(
								gpml_finite_rotation->finite_rotation())), 
				gml_time_instant,
				get_intrusive_ptr(gml_description), 
				StructuralType::create_gpml("TotalReconstructionPole"));
	}
	else
	{
		return GpmlTimeSample(
				gpml_finite_rotation, 
				gml_time_instant,
				get_intrusive_ptr(gml_description), 
				StructuralType::create_gpml("FiniteRotation"));
	}
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::ModelUtils::find_feature(
		const FeatureId &id)
{
	std::vector<FeatureHandle::weak_ref> back_ref_targets;
	id.find_back_ref_targets(append_as_weak_refs(back_ref_targets));
	
	if (back_ref_targets.size() != 1)
	{
		// We didn't get exactly one feature with the feature id so something is
		// not right (user loaded same file twice or didn't load at all)
		// so print debug message and return null feature reference.
		if ( back_ref_targets.empty() )
		{
			qWarning() 
				<< "Missing feature for feature-id = "
				<< GPlatesUtils::make_qstring_from_icu_string( id.get() );
		}
		else
		{
			qWarning() 
				<< "Multiple features for feature-id = "
				<< GPlatesUtils::make_qstring_from_icu_string( id.get() );
		}

		// Return null feature reference.
		return FeatureHandle::weak_ref();
	}

	return back_ref_targets.front();
}











