/* $Id$ */

/**
 * \file
 * Finds property values of a feature that satisfy specified property names and property value types.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_PROPERTYVALUEFINDER_H
#define GPLATES_FEATUREVISITORS_PROPERTYVALUEFINDER_H

#include <vector>
#include <algorithm>  // std::find
#include <utility>

#include "model/ConstFeatureVisitor.h"
#include "model/InlinePropertyContainer.h"
#include "model/PropertyName.h"
#include "model/types.h"
#include "property-values/Enumeration.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * Returns true if @a feature_handle has a property value of type @a PropertyValueType and
	 * property name @a property_name and 
	 * If true then property value is returned in @a property_value.
	 * If has more than one property matching criteria then only first is returned.
	 * For example:
	 *    const GPlatesPropertyValues::Enumeration *property_value;
	 *    if (get_property_value(feature_handle, property_name, &property_value))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType>
	bool
	get_property_value(
			const GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			const PropertyValueType **property_value);

	/**
	 * Returns true if @a feature_handle has any property values of type @a PropertyValueType
	 * with property name @a property_name.
	 * If so then the property values are returned in @a property_values.
	 * For example:
	 *    std::vector<const GPlatesPropertyValues::Enumeration *> property_values;
	 *    if (get_property_values(feature_handle, property_name, &property_values))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType>
	bool
	get_property_values(
			const GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			std::vector<const PropertyValueType *> *property_values);

	/**
	 * Base implementation class for const feature visitor that finds all PropertyValue's of
	 * type PropertyValueType and with property name included in a specified list that are
	 * contained within a feature.
	 */
	class PropertyValueFinderBase :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		virtual
		~PropertyValueFinderBase() {  }

		void
		add_property_name_to_allow(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			// Now visit each of the properties in turn.
			visit_feature_properties(feature_handle);
		}

		virtual
		void
		visit_inline_property_container(
				const GPlatesModel::InlinePropertyContainer &inline_property_container)
		{
			const GPlatesModel::PropertyName &curr_prop_name = inline_property_container.property_name();

			if ( ! d_property_names_to_allow.empty())
			{
				// We're not allowing all property names.
				if ( std::find(d_property_names_to_allow.begin(), d_property_names_to_allow.end(),
						curr_prop_name) == d_property_names_to_allow.end())
				{
					// The current property name is not allowed.
					return;
				}
			}

			visit_property_values(inline_property_container);
		}

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

	protected:
		// Only derived class can instantiate.
		PropertyValueFinderBase()
		{  }

		// Only derived class can instantiate.
		explicit
		PropertyValueFinderBase(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
	};


	// Declare template class PropertyValueFinder but never define it.
	// We will rely on specialisations of this class for each property value type.
	template <class PropertyValueType>
	class PropertyValueFinder;


	// Macro to declare a template specialisation of class PropertyValueFinder.
	// NOTE: We wouldn't need to do this if all "visit" methods were called 'visit' instead
	// of 'visit_gpml_plate_id' for example - in which case a simple template class would suffice.
#define DECLARE_PROPERTY_VALUE_FINDER_CLASS(property_value_type, visit_property_value_method) \
	template <> \
	class PropertyValueFinder<property_value_type> : \
			private PropertyValueFinderBase \
	{ \
	public: \
		typedef std::vector<const property_value_type *> property_value_container_type; \
		typedef property_value_container_type::const_iterator property_value_container_const_iterator; \
		typedef std::pair<property_value_container_const_iterator, property_value_container_const_iterator> \
				property_value_container_range; \
		\
		PropertyValueFinder() \
		{  } \
 \
		explicit \
		PropertyValueFinder( \
				const GPlatesModel::PropertyName &property_name_to_allow) : \
			PropertyValueFinderBase(property_name_to_allow) \
		{ \
		} \
 \
		/* Returns begin/end iterator to any found property values. */ \
		property_value_container_range \
		find_property_values( \
				const GPlatesModel::FeatureHandle &feature_handle) \
		{ \
			d_found_property_values.clear(); \
 \
			visit_feature_handle(feature_handle); \
 \
			return std::make_pair(d_found_property_values.begin(), d_found_property_values.end()); \
		} \
 \
	private: \
		virtual \
		void \
		visit_property_value_method( \
				const property_value_type &property_value) \
		{ \
			d_found_property_values.push_back(&property_value); \
		} \
 \
		property_value_container_type d_found_property_values; \
	}

	// Explicitly declare for all property values we're interested in.
	// If you don't find a property value type you're interested in here then
	// just add it using DECLARE_PROPERTY_VALUE_FINDER_CLASS().
	// TODO: possibly remove other finders like "XsStringFinder.cc".
	DECLARE_PROPERTY_VALUE_FINDER_CLASS(GPlatesPropertyValues::XsString, visit_xs_string);
	DECLARE_PROPERTY_VALUE_FINDER_CLASS(GPlatesPropertyValues::XsDouble, visit_xs_double);
	DECLARE_PROPERTY_VALUE_FINDER_CLASS(GPlatesPropertyValues::XsInteger, visit_xs_integer);
	DECLARE_PROPERTY_VALUE_FINDER_CLASS(GPlatesPropertyValues::XsBoolean, visit_xs_boolean);
	DECLARE_PROPERTY_VALUE_FINDER_CLASS(GPlatesPropertyValues::Enumeration, visit_enumeration);
	DECLARE_PROPERTY_VALUE_FINDER_CLASS(GPlatesPropertyValues::GpmlPlateId, visit_gpml_plate_id);
	DECLARE_PROPERTY_VALUE_FINDER_CLASS(GPlatesPropertyValues::GmlTimePeriod, visit_gml_time_period);
	DECLARE_PROPERTY_VALUE_FINDER_CLASS(GPlatesPropertyValues::GmlTimeInstant, visit_gml_time_instant);

	/**
	 * Returns true if @a feature_handle has any property values of type @a PropertyValueType
	 * with property name @a property_name.
	 * If so then the property values are returned in @a property_values.
	 * For example:
	 *    std::vector<const GPlatesPropertyValues::Enumeration *> property_values;
	 *    if (get_property_values(feature_handle, property_name, &property_values))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType>
	bool
	get_property_values(
			const GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			std::vector<const PropertyValueType *> *property_values)
	{
		GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType> property_value_finder(
				property_name);

		typename GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType>::property_value_container_range
				property_value_range = property_value_finder.find_property_values(feature_handle);

		property_values->insert(property_values->end(),
				property_value_range.first,
				property_value_range.second);

		return !property_values->empty();
	}

	/**
	 * Returns true if @a feature_handle has a property value of type @a PropertyValueType and
	 * property name @a property_name and 
	 * If true then property value is returned in @a property_value.
	 * If has more than one property matching criteria then only first is returned.
	 * For example:
	 *    const GPlatesPropertyValues::Enumeration *property_value;
	 *    if (get_property_value(feature_handle, property_name, &property_value))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType>
	bool
	get_property_value(
			const GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			const PropertyValueType **property_value)
	{
		GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType> property_value_finder(
				property_name);

		typename GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType>::property_value_container_range
				property_value_range = property_value_finder.find_property_values(feature_handle);

		if (property_value_range.first == property_value_range.second)
		{
			return false;
		}

		// Return first property value to caller.
		*property_value = *property_value_range.first;

		return true;
	}
}

#endif // GPLATES_FEATUREVISITORS_PROPERTYVALUEFINDER_H
