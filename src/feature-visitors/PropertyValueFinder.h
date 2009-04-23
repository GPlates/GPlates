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
#include <boost/bind.hpp>

#include "model/ConstFeatureVisitor.h"
#include "model/InlinePropertyContainer.h"
#include "model/PropertyName.h"
#include "model/types.h"
#include "property-values/GpmlConstantValue.h"


namespace GPlatesPropertyValues
{
	class Enumeration;
	class GmlLineString;
	class GmlMultiPoint;
	class GmlOrientableCurve;
	class GmlPoint;
	class GmlPolygon;
	class GmlTimeInstant;
	class GmlTimePeriod;
	class GpmlFeatureReference;
	class GpmlFeatureSnapshotReference;
	class GpmlFiniteRotation;
	class GpmlFiniteRotationSlerp;
	class GpmlHotSpotTrailMark;
	class GpmlIrregularSampling;
	class GpmlKeyValueDictionary;
	class GpmlMeasure;
	class GpmlOldPlatesHeader;
	class GpmlPiecewiseAggregation;
	class GpmlPlateId;
	class GpmlPolarityChronId;
	class GpmlPropertyDelegate;
	class GpmlRevisionId;
	class UninterpretedPropertyValue;
	class XsBoolean;
	class XsDouble;
	class XsInteger;
	class XsString;
}

namespace GPlatesFeatureVisitors
{
	/**
	 * Returns true if @a feature_handle has a property value of type @a PropertyValueType and
	 * property name @a property_name.
	 * If true then property value is returned in @a property_value.
	 * If has more than one property matching criteria then only first is returned.
	 * @a PropertyValueType and @a FeatureHandleType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle &feature_handle = ...;
	 *    const GPlatesPropertyValues::Enumeration *property_value;
	 *    if (get_property_value(feature_handle, property_name, &property_value))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureHandleType>
	bool
	get_property_value(
			FeatureHandleType &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			PropertyValueType **property_value);

	/**
	 * Returns true if @a feature_handle has a property value of type @a PropertyValueType and
	 * property name in the sequence [@a property_names_begin, @a property_names_end).
	 * If true then property value is returned in @a property_value.
	 * If has more than one property matching criteria then only first is returned.
	 * @a PropertyValueType and @a FeatureHandleType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle &feature_handle = ...;
	 *    const GPlatesPropertyValues::Enumeration *property_value;
	 *    std::vector<GPlatesModel::PropertyName> property_names;
	 *    ...
	 *    if (get_property_value(feature_handle, property_names.begin(), property_names.end(), &property_value))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureHandleType, typename PropertyNamesForwardIter>
	bool
	get_property_value(
			FeatureHandleType &feature_handle,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			PropertyValueType **property_value);

	/**
	 * Returns true if @a feature_handle has any property values of type @a PropertyValueType
	 * with property name @a property_name.
	 * If so then the property values are returned in @a property_values.
	 * @a PropertyValueType and @a FeatureHandleType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle &feature_handle = ...;
	 *    std::vector<const GPlatesPropertyValues::Enumeration *> property_values;
	 *    if (get_property_values(feature_handle, property_name, &property_values))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureHandleType>
	bool
	get_property_values(
			FeatureHandleType &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			std::vector<PropertyValueType *> *property_values);

	/**
	 * Returns true if @a feature_handle has any property values of type @a PropertyValueType
	 * and property name in the sequence [@a property_names_begin, @a property_names_end).
	 * If so then the property values are returned in @a property_values.
	 * @a PropertyValueType and @a FeatureHandleType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle &feature_handle = ...;
	 *    std::vector<const GPlatesPropertyValues::Enumeration *> property_values;
	 *    std::vector<GPlatesModel::PropertyName> property_names;
	 *    ...
	 *    if (get_property_values(feature_handle, property_names.begin(), property_names.end(), &property_values))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureHandleType, typename PropertyNamesForwardIter>
	bool
	get_property_values(
			FeatureHandleType &feature_handle,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			std::vector<PropertyValueType *> *property_values);


	/**
	 * Base implementation class for const feature visitor that finds all PropertyValue's of
	 * type PropertyValueType and with property name included in a specified list that are
	 * contained within a feature.
	 */
	template <class FeatureVisitorType,
			class FeatureHandleType,
			class InlinePropertyContainerType,
			class GpmlConstantValueType>
	class PropertyValueFinderBase :
			public FeatureVisitorType
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
				FeatureHandleType &feature_handle)
		{
			// Now visit each of the properties in turn.
			visit_feature_properties(feature_handle);
		}

		virtual
		void
		visit_inline_property_container(
				InlinePropertyContainerType &inline_property_container)
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
				GpmlConstantValueType &gpml_constant_value)
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
	// However having "visit" methods named as they are probably help readability and avoids needing
	// 'using ConstFeatureVisitor::visit;" declarations in derived visitor classes.
#define DECLARE_PROPERTY_VALUE_FINDER_CLASS(property_value_type, \
		visit_property_value_method, \
		feature_visitor_type, \
		feature_handle_type, \
		inline_property_container_type, \
		gpml_constant_value_type \
) \
	template <> \
	class PropertyValueFinder<property_value_type> : \
			public PropertyValueFinderBase< \
					feature_visitor_type, \
					feature_handle_type, \
					inline_property_container_type, \
					gpml_constant_value_type> \
	{ \
	public: \
		typedef std::vector<property_value_type *> property_value_container_type; \
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
			PropertyValueFinderBase< \
					feature_visitor_type, \
					feature_handle_type, \
					inline_property_container_type, \
					gpml_constant_value_type> (property_name_to_allow) \
		{ \
		} \
 \
		/* Returns begin/end iterator to any found property values. */ \
		property_value_container_range \
		find_property_values( \
				feature_handle_type &feature_handle) \
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
				property_value_type &property_value) \
		{ \
			d_found_property_values.push_back(&property_value); \
		} \
 \
		property_value_container_type d_found_property_values; \
	}


#define DECLARE_PROPERTY_VALUE_FINDER(property_value_type, visit_property_value_method) \
		/* const version */ \
		DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
				const property_value_type, \
				visit_property_value_method, \
				GPlatesModel::ConstFeatureVisitor, \
				const GPlatesModel::FeatureHandle, \
				const GPlatesModel::InlinePropertyContainer, \
				const GPlatesPropertyValues::GpmlConstantValue); \
		/* non-const version */ \
		DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
				property_value_type, \
				visit_property_value_method, \
				GPlatesModel::FeatureVisitor, \
				GPlatesModel::FeatureHandle, \
				GPlatesModel::InlinePropertyContainer, \
				GPlatesPropertyValues::GpmlConstantValue); \


	// Explicitly declare for all property values we're interested in.
	// If a property value type is added to the feature visitor base class then it'll
	// need to be added here too using DECLARE_PROPERTY_VALUE_FINDER().
	// TODO: possibly remove other finders like "XsStringFinder.cc" that have the same
	// functionality as, for example, PropertyValueFinder<XsString>.
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::Enumeration, visit_enumeration)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlLineString, visit_gml_line_string)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlMultiPoint, visit_gml_multi_point)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlOrientableCurve, visit_gml_orientable_curve)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlPoint, visit_gml_point)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlPolygon, visit_gml_polygon)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlTimeInstant, visit_gml_time_instant)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlTimePeriod, visit_gml_time_period)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlFeatureReference, visit_gpml_feature_reference)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlFeatureSnapshotReference, visit_gpml_feature_snapshot_reference)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlFiniteRotation, visit_gpml_finite_rotation)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlFiniteRotationSlerp, visit_gpml_finite_rotation_slerp)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlHotSpotTrailMark, visit_gpml_hot_spot_trail_mark)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlIrregularSampling, visit_gpml_irregular_sampling)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlKeyValueDictionary, visit_gpml_key_value_dictionary)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlMeasure, visit_gpml_measure)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlOldPlatesHeader, visit_gpml_old_plates_header)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlPiecewiseAggregation, visit_gpml_piecewise_aggregation)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlPlateId, visit_gpml_plate_id)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlPolarityChronId, visit_gpml_polarity_chron_id)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlPropertyDelegate, visit_gpml_property_delegate)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlRevisionId, visit_gpml_revision_id)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::UninterpretedPropertyValue, visit_uninterpreted_property_value)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::XsBoolean, visit_xs_boolean)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::XsDouble, visit_xs_double)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::XsInteger, visit_xs_integer)
	DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::XsString, visit_xs_string)


	template <class PropertyValueType, class FeatureHandleType>
	bool
	get_property_values(
			FeatureHandleType &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			std::vector<PropertyValueType *> *property_values)
	{
		typedef GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType> property_value_finder_type;

		property_value_finder_type property_value_finder(property_name);

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_handle);

		property_values->insert(property_values->end(),
				property_value_range.first,
				property_value_range.second);

		return !property_values->empty();
	}


	template <class PropertyValueType, class FeatureHandleType, typename PropertyNamesForwardIter>
	bool
	get_property_values(
			FeatureHandleType &feature_handle,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			std::vector<const PropertyValueType *> *property_values)
	{
		typedef GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType> property_value_finder_type;

		property_value_finder_type property_value_finder;

		// Add the sequence of property names to allow.
		std::for_each(property_names_begin, property_names_end,
				boost::bind(
						&property_value_finder_type::add_property_name_to_allow,
						boost::ref(property_value_finder),
						_1));

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_handle);

		property_values->insert(property_values->end(),
				property_value_range.first,
				property_value_range.second);

		return !property_values->empty();
	}


	template <class PropertyValueType, class FeatureHandleType>
	bool
	get_property_value(
			FeatureHandleType &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			PropertyValueType **property_value)
	{
		typedef GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType> property_value_finder_type;

		property_value_finder_type property_value_finder(property_name);

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_handle);

		if (property_value_range.first == property_value_range.second)
		{
			return false;
		}

		// Return first property value to caller.
		*property_value = *property_value_range.first;

		return true;
	}


	template <class PropertyValueType, class FeatureHandleType, typename PropertyNamesForwardIter>
	bool
	get_property_value(
			FeatureHandleType &feature_handle,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			const PropertyValueType **property_value)
	{
		typedef GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType> property_value_finder_type;

		property_value_finder_type property_value_finder;

		// Add the sequence of property names to allow.
		std::for_each(property_names_begin, property_names_end,
				boost::bind(
						&property_value_finder_type::add_property_name_to_allow,
						boost::ref(property_value_finder),
						_1));

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_handle);

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
