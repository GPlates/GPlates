/* $Id$ */

/**
 * \file
 * Finds property values of a feature that satisfy specified property names and property value types.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include <algorithm>  // std::find
#include <typeinfo>
#include <utility>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/optional.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/remove_const.hpp>

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/PropertyValue.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/CopyConst.h"


namespace GPlatesFeatureVisitors
{
	///////////////
	// Interface //
	///////////////

	/**
	 * Returns the derived property value of type @a PropertValueType if @a property_value_base is
	 * an instance of that type.
	 *
	 * @a reconstruction_time only applies to time-dependent properties in which case the
	 * value of the property at the specified time is returned.
	 * It is effectively ignored for GpmlConstantValue properties.
	 *
	 * Note that only *const* property values are handled.
	 * This is because the returned property value might be a newly created object which, if modified,
	 * would not affect the original property value.
	 * An example of this is a time-dependent property where a new *interpolated* property value
	 * (sample) is returned for an irregularly sampled property value (when the reconstruction time does
	 * not match any of the sample times) - and modifying that would not insert the new interpolated
	 * property value into the irregularly sampled sequence.
	 *
	 * For example:
	 *    const GPlatesModel::PropertyValue &property_value_base = ...;
	 *    boost::optional<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> property_value =
	 *        get_property_value<GPlatesPropertyValues::Enumeration>(property_value_base);
	 *    if (property_value)
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType>
	boost::optional<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_value(
			const GPlatesModel::PropertyValue &property_value_base,
			const double &reconstruction_time = 0);

	/**
	 * Returns true if @a feature_or_property has/is a property value of type @a PropertyValueType
	 * and property name @a property_name.
	 *
	 * If true then property value is returned in @a property_value.
	 * If has more than one property matching criteria then only first is returned.
	 *
	 * @a PropertyValueType and @a FeatureOrPropertyType can be either const or non-const types
	 * but, of course, you cannot get a non-const PropertyValueType from a const FeatureOrPropertyType.
	 *
	 * @a FeatureOrPropertyType can be any of the following types:
	 *
	 *     GPlatesModel::FeatureHandle::const_weak_ref,
	 *     GPlatesModel::FeatureHandle::weak_ref,
	 *     GPlatesModel::FeatureCollectionHandle::const_iterator,
	 *     GPlatesModel::FeatureCollectionHandle::iterator,
	 *     GPlatesModel::FeatureHandle::const_iterator, or
	 *     GPlatesModel::FeatureHandle::iterator.
	 *
	 * @a reconstruction_time only applies to time-dependent properties in which case the
	 * value of the property at the specified time is returned.
	 * It is effectively ignored for constant-valued properties.
	 *
	 * Note that only *const* property values are handled.
	 * This is because the returned property value might be a newly created object which, if modified,
	 * would not affect the original property value.
	 * An example of this is a time-dependent property where a new *interpolated* property value
	 * (sample) is returned for an irregularly sampled property value (when the reconstruction time does
	 * not match any of the sample times) - and modifying that would not insert the new interpolated
	 * property value into the irregularly sampled sequence.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle::weak_ref feature_weak_ref = ...;
	 *    boost::optional<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> property_value =
	 *        get_property_value<GPlatesPropertyValues::Enumeration>(feature_weak_ref, property_name);
	 *    if (property_value)
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureOrPropertyType>
	boost::optional<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_value(
			const FeatureOrPropertyType &feature_or_property,
			const GPlatesModel::PropertyName &property_name,
			const double &reconstruction_time = 0);

	/**
	 * Returns true if @a feature_or_property has/is a property value of type @a PropertyValueType
	 * and property name in the sequence [@a property_names_begin, @a property_names_end).
	 *
	 * If true then property value is returned in @a property_value.
	 * If has more than one property matching criteria then only first is returned.
	 *
	 * @a PropertyValueType and @a FeatureOrPropertyType can be either const or non-const types
	 * but, of course, you cannot get a non-const PropertyValueType from a const FeatureOrPropertyType.
	 *
	 * @a FeatureOrPropertyType can be any of the following types:
	 *
	 *     GPlatesModel::FeatureHandle::const_weak_ref,
	 *     GPlatesModel::FeatureHandle::weak_ref,
	 *     GPlatesModel::FeatureCollectionHandle::const_iterator,
	 *     GPlatesModel::FeatureCollectionHandle::iterator,
	 *     GPlatesModel::FeatureHandle::const_iterator, or
	 *     GPlatesModel::FeatureHandle::iterator.
	 *
	 * @a reconstruction_time only applies to time-dependent properties in which case the
	 * value of the property at the specified time is returned.
	 * It is effectively ignored for constant-valued properties.
	 *
	 * Note that only *const* property values are handled.
	 * This is because the returned property value might be a newly created object which, if modified,
	 * would not affect the original property value.
	 * An example of this is a time-dependent property where a new *interpolated* property value
	 * (sample) is returned for an irregularly sampled property value (when the reconstruction time does
	 * not match any of the sample times) - and modifying that would not insert the new interpolated
	 * property value into the irregularly sampled sequence.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle::weak_ref feature_weak_ref = ...;
	 *    std::vector<GPlatesModel::PropertyName> property_names;
	 *    ...
	 *    boost::optional<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> property_value =
	 *        get_property_value<GPlatesPropertyValues::Enumeration>(
	 *            feature_weak_ref, property_names.begin(), property_names.end());
	 *    if (property_value)
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureOrPropertyType, typename PropertyNamesForwardIter>
	boost::optional<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_value(
			const FeatureOrPropertyType &feature_or_property,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			const double &reconstruction_time = 0);

	/**
	 * Returns true if @a feature_or_property has/is any property values of type @a PropertyValueType
	 * with property name @a property_name.
	 *
	 * If so then the property values are returned in @a property_values.
	 *
	 * @a PropertyValueType and @a FeatureOrPropertyType can be either const or non-const types
	 * but, of course, you cannot get a non-const PropertyValueType from a const FeatureOrPropertyType.
	 *
	 * @a FeatureOrPropertyType can be any of the following types:
	 *
	 *     GPlatesModel::FeatureHandle::const_weak_ref,
	 *     GPlatesModel::FeatureHandle::weak_ref,
	 *     GPlatesModel::FeatureCollectionHandle::const_iterator,
	 *     GPlatesModel::FeatureCollectionHandle::iterator,
	 *     GPlatesModel::FeatureHandle::const_iterator, or
	 *     GPlatesModel::FeatureHandle::iterator.
	 *
	 * @a reconstruction_time only applies to time-dependent properties in which case the
	 * value of the property at the specified time is returned.
	 * It is effectively ignored for constant-valued properties.
	 *
	 * Note that only *const* property values are handled.
	 * This is because the returned property value might be a newly created object which, if modified,
	 * would not affect the original property value.
	 * An example of this is a time-dependent property where a new *interpolated* property value
	 * (sample) is returned for an irregularly sampled property value (when the reconstruction time does
	 * not match any of the sample times) - and modifying that would not insert the new interpolated
	 * property value into the irregularly sampled sequence.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle::weak_ref feature_weak_ref = ...;
	 *    std::vector<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> property_values =
	 *        get_property_values<GPlatesPropertyValues::Enumeration>(feature_weak_ref, property_name);
	 *    if (!property_values.empty())
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureOrPropertyType>
	std::vector<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_values(
			const FeatureOrPropertyType &feature_or_property,
			const GPlatesModel::PropertyName &property_name,
			const double &reconstruction_time = 0);

	/**
	 * Returns true if @a feature_or_property has/is any property values of type @a PropertyValueType
	 * and property name in the sequence [@a property_names_begin, @a property_names_end).
	 *
	 * If so then the property values are returned in @a property_values.
	 *
	 * @a PropertyValueType and @a FeatureOrPropertyType can be either const or non-const types
	 * but, of course, you cannot get a non-const PropertyValueType from a const FeatureOrPropertyType.
	 *
	 * @a FeatureOrPropertyType can be any of the following types:
	 *
	 *     GPlatesModel::FeatureHandle::const_weak_ref,
	 *     GPlatesModel::FeatureHandle::weak_ref,
	 *     GPlatesModel::FeatureCollectionHandle::const_iterator,
	 *     GPlatesModel::FeatureCollectionHandle::iterator,
	 *     GPlatesModel::FeatureHandle::const_iterator, or
	 *     GPlatesModel::FeatureHandle::iterator.
	 *
	 * Note that only *const* property values are handled.
	 * This is because the returned property value might be a newly created object which, if modified,
	 * would not affect the original property value.
	 * An example of this is a time-dependent property where a new *interpolated* property value
	 * (sample) is returned for an irregularly sampled property value (when the reconstruction time does
	 * not match any of the sample times) - and modifying that would not insert the new interpolated
	 * property value into the irregularly sampled sequence.
	 *
	 * For example:
	 *    GPlatesModel::FeatureHandle::weak_ref feature_weak_ref = ...;
	 *    std::vector<GPlatesModel::PropertyName> property_names;
	 *    ...
	 *    std::vector<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> property_values =
	 *        get_property_values<GPlatesPropertyValues::Enumeration>(
	 *            feature_weak_ref, property_names.begin(), property_names.end());
	 *    if (!property_values.empty())
	 *    {
	 *       ...
	 *    }
	 */
	template <class PropertyValueType, class FeatureOrPropertyType, typename PropertyNamesForwardIter>
	std::vector<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_values(
			const FeatureOrPropertyType &feature_or_property,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			const double &reconstruction_time = 0);


	////////////////////
	// Implementation //
	////////////////////
	namespace Implementation
	{
		//
		// NOTE: These function are not templates purely so they can be defined in the ".cc" file to avoid
		// cyclic header dependencies including headers for irregular sampling and piecewise aggregation.
		//
		void
		visit_gpml_constant_value(
				GPlatesModel::ConstFeatureVisitor::gpml_constant_value_type &gpml_constant_value,
				GPlatesModel::ConstFeatureVisitor &property_value_finder_visitor);
		void
		visit_gpml_irregular_sampling_at_reconstruction_time(
				GPlatesModel::ConstFeatureVisitor::gpml_irregular_sampling_type &gpml_irregular_sampling,
				GPlatesModel::ConstFeatureVisitor &property_value_finder_visitor,
				const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
				const std::type_info &property_value_type_info);
		void
		visit_gpml_piecewise_aggregation_at_reconstruction_time(
				GPlatesModel::ConstFeatureVisitor::gpml_piecewise_aggregation_type &gpml_piecewise_aggregation,
				GPlatesModel::ConstFeatureVisitor &property_value_finder_visitor,
				const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time);


		/**
		 * Base implementation class for feature visitor that finds all PropertyValue's of
		 * type PropertyValueType and with property name included in a specified list that are
		 * contained within a feature.
		 *
		 * NOTE: The property values can be constant over time or time-dependent - in the latter
		 * case the reconstruction time is used to lookup the property value.
		 */
		template <class PropertyValueType>
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

		protected:

			virtual
			bool
			initialise_pre_property_values(
					top_level_property_inline_type &top_level_property_inline)
			{
				const GPlatesModel::PropertyName &curr_prop_name = top_level_property_inline.get_property_name();

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
					gpml_constant_value_type &gpml_constant_value)
			{
				Implementation::visit_gpml_constant_value(gpml_constant_value, *this);
			}

			// In case property value is time-dependent.
			virtual
			void
			visit_gpml_irregular_sampling(
					gpml_irregular_sampling_type &gpml_irregular_sampling)
			{
				Implementation::visit_gpml_irregular_sampling_at_reconstruction_time(
						gpml_irregular_sampling, *this, d_reconstruction_time,
						// Optimisation to avoid interpolating a property value when it's the
						// wrong type and will just get discarded anyway...
						typeid(PropertyValueType));
			}

			// In case property value is time-dependent.
			virtual
			void
			visit_gpml_piecewise_aggregation(
					gpml_piecewise_aggregation_type &gpml_piecewise_aggregation) 
			{
				// No optimisation here (like with 'visit_gpml_irregular_sampling') because
				// nested property value type could be another time-dependent wrapper type.

				Implementation::visit_gpml_piecewise_aggregation_at_reconstruction_time(
						gpml_piecewise_aggregation, *this, d_reconstruction_time);
			}

			// Only derived class can instantiate.
			explicit
			PropertyValueFinderBase(
					const double &reconstruction_time = 0) :
				d_reconstruction_time(GPlatesPropertyValues::GeoTimeInstant(reconstruction_time))
			{  }

			// Only derived class can instantiate.
			explicit
			PropertyValueFinderBase(
					const GPlatesModel::PropertyName &property_name_to_allow,
					const double &reconstruction_time = 0) :
				d_reconstruction_time(GPlatesPropertyValues::GeoTimeInstant(reconstruction_time))
			{
				d_property_names_to_allow.push_back(property_name_to_allow);
			}

			std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;

			GPlatesPropertyValues::GeoTimeInstant d_reconstruction_time;
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
#define DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
			property_value_type, \
			visit_property_value_method \
		) \
		namespace GPlatesFeatureVisitors \
		{ \
			namespace Implementation \
			{ \
				template <> \
				class PropertyValueFinder<property_value_type> : \
						public PropertyValueFinderBase<property_value_type> \
				{ \
				public: \
					typedef std::vector<GPlatesUtils::non_null_intrusive_ptr<property_value_type> > property_value_container_type; \
					typedef property_value_container_type::const_iterator property_value_container_const_iterator; \
					typedef std::pair<property_value_container_const_iterator, property_value_container_const_iterator> \
							property_value_container_range; \
					\
					PropertyValueFinder( \
							const double &reconstruction_time = 0) : \
						PropertyValueFinderBase<property_value_type>(reconstruction_time) \
					{  } \
			 \
					explicit \
					PropertyValueFinder( \
							const GPlatesModel::PropertyName &property_name_to_allow, \
							const double &reconstruction_time = 0) : \
						PropertyValueFinderBase<property_value_type>(property_name_to_allow, reconstruction_time) \
					{ \
					} \
			 \
					/* Returns begin/end iterator to any found property values. */ \
					property_value_container_range \
					find_property_values( \
							const feature_weak_ref_type &feature_weak_ref) \
					{ \
						d_found_property_values.clear(); \
						visit_feature(feature_weak_ref); \
						return std::make_pair(d_found_property_values.begin(), d_found_property_values.end()); \
					} \
			 \
					/* Returns begin/end iterator to any found property values. */ \
					property_value_container_range \
					find_property_values( \
							const feature_collection_iterator_type &feature_collection_iterator) \
					{ \
						d_found_property_values.clear(); \
						visit_feature(feature_collection_iterator); \
						return std::make_pair(d_found_property_values.begin(), d_found_property_values.end()); \
					} \
			 \
					/* Returns begin/end iterator to any found property values. */ \
					property_value_container_range \
					find_property_values( \
							const feature_iterator_type &feature_iterator) \
					{ \
						d_found_property_values.clear(); \
						visit_feature_property(feature_iterator); \
						return std::make_pair(d_found_property_values.begin(), d_found_property_values.end()); \
					} \
			 \
					/* Returns begin/end iterator to any found property values. */ \
					property_value_container_range \
					find_property_values( \
							/* Using PropertyValue instead of derived property value type to avoid undefined class error... */ \
							GPlatesUtils::CopyConst<property_value_type, GPlatesModel::PropertyValue>::type &property_value_base) \
					{ \
						d_found_property_values.clear(); \
						property_value_base.accept_visitor(*this); \
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
			} \
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
		/* for const property-value */ \
		DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
				boost::add_const<property_value_type>::type, \
				visit_property_value_method)
	}


	template <class PropertyValueType>
	boost::optional<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_value(
			const GPlatesModel::PropertyValue &property_value_base,
			const double &reconstruction_time)
	{
		typedef Implementation::PropertyValueFinder<typename boost::add_const<PropertyValueType>::type>
				property_value_finder_type;

		property_value_finder_type property_value_finder(reconstruction_time);

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(property_value_base);

		if (property_value_range.first == property_value_range.second)
		{
			return boost::none;
		}

		// Return first property value to caller.
		return *property_value_range.first;
	}


	template <class PropertyValueType, class FeatureOrPropertyType>
	std::vector<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_values(
			const FeatureOrPropertyType &feature_or_property,
			const GPlatesModel::PropertyName &property_name,
			const double &reconstruction_time)
	{
		typedef Implementation::PropertyValueFinder<typename boost::add_const<PropertyValueType>::type>
				property_value_finder_type;

		property_value_finder_type property_value_finder(property_name, reconstruction_time);

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_or_property);

		return std::vector<typename PropertyValueType::non_null_ptr_to_const_type>(
				property_value_range.first,
				property_value_range.second);
	}


	template <class PropertyValueType, class FeatureOrPropertyType, typename PropertyNamesForwardIter>
	std::vector<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_values(
			const FeatureOrPropertyType &feature_or_property,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			const double &reconstruction_time)
	{
		typedef Implementation::PropertyValueFinder<typename boost::add_const<PropertyValueType>::type>
				property_value_finder_type;

		property_value_finder_type property_value_finder(reconstruction_time);

		// Add the sequence of property names to allow.
		std::for_each(property_names_begin, property_names_end,
				boost::bind(
						&property_value_finder_type::add_property_name_to_allow,
						boost::ref(property_value_finder),
						boost::placeholders::_1));

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_or_property);

		return std::vector<typename PropertyValueType::non_null_ptr_to_const_type>(
				property_value_range.first,
				property_value_range.second);
	}


	template <class PropertyValueType, class FeatureOrPropertyType>
	boost::optional<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_value(
			const FeatureOrPropertyType &feature_or_property,
			const GPlatesModel::PropertyName &property_name,
			const double &reconstruction_time)
	{
		typedef Implementation::PropertyValueFinder<typename boost::add_const<PropertyValueType>::type>
				property_value_finder_type;

		property_value_finder_type property_value_finder(property_name, reconstruction_time);

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_or_property);

		if (property_value_range.first == property_value_range.second)
		{
			return boost::none;
		}

		// Return first property value to caller.
		return *property_value_range.first;
	}


	template <class PropertyValueType, class FeatureOrPropertyType, typename PropertyNamesForwardIter>
	boost::optional<typename PropertyValueType::non_null_ptr_to_const_type>
	get_property_value(
			const FeatureOrPropertyType &feature_or_property,
			PropertyNamesForwardIter property_names_begin,
			PropertyNamesForwardIter property_names_end,
			const double &reconstruction_time)
	{
		typedef Implementation::PropertyValueFinder<typename boost::add_const<PropertyValueType>::type>
				property_value_finder_type;

		property_value_finder_type property_value_finder(reconstruction_time);

		// Add the sequence of property names to allow.
		std::for_each(property_names_begin, property_names_end,
				boost::bind(
						&property_value_finder_type::add_property_name_to_allow,
						boost::ref(property_value_finder),
						boost::placeholders::_1));

		typename property_value_finder_type::property_value_container_range property_value_range =
				property_value_finder.find_property_values(feature_or_property);

		if (property_value_range.first == property_value_range.second)
		{
			return boost::none;
		}

		// Return first property value to caller.
		return *property_value_range.first;
	}
}

#endif // GPLATES_FEATUREVISITORS_PROPERTYVALUEFINDER_H
