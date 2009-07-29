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
#include "model/TopLevelPropertyInline.h"
#include "model/PropertyName.h"
#include "model/types.h"
#include "property-values/GpmlConstantValue.h"


namespace GPlatesFeatureVisitors
{
	///////////////
	// Interface //
	///////////////

	/**
	 * Returns true if @a feature_weak_ref has a property value of type @a PropertyValueType
	 * and property name @a property_name.
	 *
	 * If true then property value is returned in @a property_value.
	 * If has more than one property matching criteria then only first is returned.
	 * @a PropertyValueType and @a FeatureWeakRefType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle::weak_ref feature_weak_ref = ...;
	 *    const GPlatesPropertyValues::Enumeration *property_value;
	 *    if (get_property_value(feature_weak_ref, property_name, property_value))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureWeakRefType>
	bool
	get_property_value(
			const FeatureWeakRefType &feature_weak_ref,
			const GPlatesModel::PropertyName &property_name,
			PropertyValueType *&property_value);

	/**
	 * Returns true if @a feature_weak_ref has a property value of type @a PropertyValueType
	 * and property name in the sequence [@a property_names_begin, @a property_names_end).
	 *
	 * If true then property value is returned in @a property_value.
	 * If has more than one property matching criteria then only first is returned.
	 * @a PropertyValueType and @a FeatureWeakRefType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle::weak_ref feature_weak_ref = ...;
	 *    const GPlatesPropertyValues::Enumeration *property_value;
	 *    std::vector<GPlatesModel::PropertyName> property_names;
	 *    ...
	 *    if (get_property_value(feature_weak_ref, property_names.begin(), property_names.end(), property_value))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureWeakRefType, typename PropertyNamesForwardIter>
	bool
	get_property_value(
			const FeatureWeakRefType &feature_weak_ref,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			PropertyValueType *&property_value);

	/**
	 * Returns true if @a feature_weak_ref has any property values of type @a PropertyValueType
	 * with property name @a property_name.
	 *
	 * If so then the property values are returned in @a property_values.
	 * @a PropertyValueType and @a FeatureWeakRefType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle::weak_ref feature_weak_ref = ...;
	 *    std::vector<const GPlatesPropertyValues::Enumeration *> property_values;
	 *    if (get_property_values(feature_weak_ref, property_name, property_values))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureWeakRefType>
	bool
	get_property_values(
			const FeatureWeakRefType &feature_weak_ref,
			const GPlatesModel::PropertyName &property_name,
			std::vector<PropertyValueType *> &property_values);

	/**
	 * Returns true if @a feature_weak_ref has any property values of type @a PropertyValueType
	 * and property name in the sequence [@a property_names_begin, @a property_names_end).
	 *
	 * If so then the property values are returned in @a property_values.
	 * @a PropertyValueType and @a FeatureWeakRefType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle::weak_ref feature_weak_ref = ...;
	 *    std::vector<const GPlatesPropertyValues::Enumeration *> property_values;
	 *    std::vector<GPlatesModel::PropertyName> property_names;
	 *    ...
	 *    if (get_property_values(feature_weak_ref, property_names.begin(), property_names.end(), property_values))
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureWeakRefType, typename PropertyNamesForwardIter>
	bool
	get_property_values(
			const FeatureWeakRefType &feature_weak_ref,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			std::vector<PropertyValueType *> &property_values);


	////////////////////
	// Implementation //
	////////////////////


	/**
	 * Base implementation class for const feature visitor that finds all PropertyValue's of
	 * type PropertyValueType and with property name included in a specified list that are
	 * contained within a feature.
	 */
	template <class FeatureVisitorType,
			class FeatureWeakRefType,
			class TopLevelPropertyInlineType,
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

	protected:

		virtual
		bool
		initialise_pre_property_values(
				TopLevelPropertyInlineType &top_level_property_inline)
		{
			const GPlatesModel::PropertyName &curr_prop_name = top_level_property_inline.property_name();

			if ( ! d_property_names_to_allow.empty())
			{
				// We're not allowing all property names.
				if ( std::find(d_property_names_to_allow.begin(), d_property_names_to_allow.end(),
						curr_prop_name) == d_property_names_to_allow.end())
				{
					// The current property name is not allowed.
					return false;
				}
			}
			return true;
		}

		virtual
		void
		visit_gpml_constant_value(
				GpmlConstantValueType &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

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
	// We will rely on specialisations of this class for each property value type and
	// feature visitor type.
	template <class PropertyValueType, class FeatureWeakRefType>
	class PropertyValueFinder;


	// Macro to declare a template specialisation of class PropertyValueFinder.
	// NOTE: We wouldn't need to do this if all "visit" methods were called 'visit' instead
	// of 'visit_gpml_plate_id' for example - in which case a simple template class would suffice.
	// However having "visit" methods named as they are probably help readability and avoids needing
	// 'using ConstFeatureVisitor::visit;" declarations in derived visitor classes.
	//
#define DECLARE_PROPERTY_VALUE_FINDER_CLASS(property_value_type, \
		visit_property_value_method, \
		feature_visitor_type, \
		feature_weak_ref_type, \
		features_iterator_type, \
		inline_property_container_type, \
		gpml_constant_value_type \
) \
	namespace GPlatesFeatureVisitors \
	{ \
		template <> \
		class PropertyValueFinder<property_value_type, feature_weak_ref_type> : \
				public PropertyValueFinderBase< \
						feature_visitor_type, \
						feature_weak_ref_type, \
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
						feature_weak_ref_type, \
						inline_property_container_type, \
						gpml_constant_value_type> (property_name_to_allow) \
			{ \
			} \
	 \
			/* Returns begin/end iterator to any found property values. */ \
			property_value_container_range \
			find_property_values( \
					const feature_weak_ref_type &feature_weak_ref) \
			{ \
				d_found_property_values.clear(); \
	 \
				visit_feature(feature_weak_ref); \
	 \
				return std::make_pair(d_found_property_values.begin(), d_found_property_values.end()); \
			} \
	 \
			/* Returns begin/end iterator to any found property values. */ \
			property_value_container_range \
			find_property_values( \
					features_iterator_type &features_iterator) \
			{ \
				d_found_property_values.clear(); \
	 \
				visit_feature(features_iterator); \
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
		}; \
	}


	// NOTE: DECLARE_PROPERTY_VALUE_FINDER must be placed at the top of every derivation
	// of GPlatesModel::PropertyValue in order for the get property functions in this file to
	// work with that type of property value.
	//
	// First parameter should be the namespace qualified property value class.
	// Second parameter should be the name of the feature visitor method that visits the
	//
	// property value.
	// For example:
	//    DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::Enumeration, visit_enumeration)
	//
#define DECLARE_PROPERTY_VALUE_FINDER(property_value_type, visit_property_value_method) \
		/* const weak_ref/features_iterator for const property-value */ \
		DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
				const property_value_type, \
				visit_property_value_method, \
				GPlatesModel::ConstFeatureVisitor, \
				GPlatesModel::FeatureHandle::const_weak_ref, \
				GPlatesModel::FeatureCollectionHandle::features_const_iterator, \
				const GPlatesModel::TopLevelPropertyInline, \
				const GPlatesPropertyValues::GpmlConstantValue) \
		/* non-const weak_ref/features_iterator for const property-value */ \
		DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
				const property_value_type, \
				visit_property_value_method, \
				GPlatesModel::ConstFeatureVisitor, \
				GPlatesModel::FeatureHandle::weak_ref, \
				GPlatesModel::FeatureCollectionHandle::features_iterator, \
				const GPlatesModel::TopLevelPropertyInline, \
				const GPlatesPropertyValues::GpmlConstantValue) \
		/* non-const weak_ref/features_iterator for non-const property-value */ \
		DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
				property_value_type, \
				visit_property_value_method, \
				GPlatesModel::FeatureVisitor, \
				GPlatesModel::FeatureHandle::weak_ref, \
				GPlatesModel::FeatureCollectionHandle::features_iterator, \
				GPlatesModel::TopLevelPropertyInline, \
				GPlatesPropertyValues::GpmlConstantValue) \


	template <class PropertyValueType, class FeatureWeakRefType>
	bool
	get_property_values(
			const FeatureWeakRefType &feature_weak_ref,
			const GPlatesModel::PropertyName &property_name,
			std::vector<PropertyValueType *> &property_values)
	{
		typedef GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType, FeatureWeakRefType>
				property_value_finder_type;

		property_value_finder_type property_value_finder(property_name);

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_weak_ref);

		property_values.insert(property_values.end(),
				property_value_range.first,
				property_value_range.second);

		return !property_values.empty();
	}


	template <class PropertyValueType, class FeatureWeakRefType, typename PropertyNamesForwardIter>
	bool
	get_property_values(
			const FeatureWeakRefType &feature_weak_ref,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			std::vector<PropertyValueType *> &property_values)
	{
		typedef GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType, FeatureWeakRefType>
				property_value_finder_type;

		property_value_finder_type property_value_finder;

		// Add the sequence of property names to allow.
		std::for_each(property_names_begin, property_names_end,
				boost::bind(
						&property_value_finder_type::add_property_name_to_allow,
						boost::ref(property_value_finder),
						_1));

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_weak_ref);

		property_values.insert(property_values.end(),
				property_value_range.first,
				property_value_range.second);

		return !property_values.empty();
	}


	template <class PropertyValueType, class FeatureWeakRefType>
	bool
	get_property_value(
			const FeatureWeakRefType &feature_weak_ref,
			const GPlatesModel::PropertyName &property_name,
			PropertyValueType *&property_value)
	{
		typedef GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType, FeatureWeakRefType>
				property_value_finder_type;

		property_value_finder_type property_value_finder(property_name);

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_weak_ref);

		if (property_value_range.first == property_value_range.second)
		{
			property_value = NULL;
			return false;
		}

		// Return first property value to caller.
		property_value = *property_value_range.first;

		return true;
	}


	template <class PropertyValueType, class FeatureWeakRefType, typename PropertyNamesForwardIter>
	bool
	get_property_value(
			const FeatureWeakRefType &feature_weak_ref,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			PropertyValueType *&property_value)
	{
		typedef GPlatesFeatureVisitors::PropertyValueFinder<PropertyValueType, FeatureWeakRefType>
				property_value_finder_type;

		property_value_finder_type property_value_finder;

		// Add the sequence of property names to allow.
		std::for_each(property_names_begin, property_names_end,
				boost::bind(
						&property_value_finder_type::add_property_name_to_allow,
						boost::ref(property_value_finder),
						_1));

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_weak_ref);

		if (property_value_range.first == property_value_range.second)
		{
			property_value = NULL;
			return false;
		}

		// Return first property value to caller.
		property_value = *property_value_range.first;

		return true;
	}
}

#endif // GPLATES_FEATUREVISITORS_PROPERTYVALUEFINDER_H
