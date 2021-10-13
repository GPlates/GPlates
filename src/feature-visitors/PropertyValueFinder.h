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

#include <vector>
#include <algorithm>  // std::find
#include <utility>
#include <boost/bind.hpp>

#include "model/FeatureVisitor.h"
#include "model/TopLevelPropertyInline.h"
#include "model/PropertyName.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


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
	 * @a reconstruction_time only applies to time-dependent properties in which case the
	 * value of the property at the specified time is returned.
	 * It is effectively ignored for constant-valued properties.
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
			PropertyValueType *&property_value,
			const double &reconstruction_time = 0);

	/**
	 * Returns true if @a feature_weak_ref has a property value of type @a PropertyValueType
	 * and property name in the sequence [@a property_names_begin, @a property_names_end).
	 *
	 * If true then property value is returned in @a property_value.
	 * If has more than one property matching criteria then only first is returned.
	 * @a PropertyValueType and @a FeatureWeakRefType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * @a reconstruction_time only applies to time-dependent properties in which case the
	 * value of the property at the specified time is returned.
	 * It is effectively ignored for constant-valued properties.
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
			PropertyValueType *&property_value,
			const double &reconstruction_time = 0);

	/**
	 * Returns true if @a feature_weak_ref has any property values of type @a PropertyValueType
	 * with property name @a property_name.
	 *
	 * If so then the property values are returned in @a property_values.
	 * @a PropertyValueType and @a FeatureWeakRefType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * @a reconstruction_time only applies to time-dependent properties in which case the
	 * value of the property at the specified time is returned.
	 * It is effectively ignored for constant-valued properties.
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
			std::vector<PropertyValueType *> &property_values,
			const double &reconstruction_time = 0);

	/**
	 * Returns true if @a feature_weak_ref has any property values of type @a PropertyValueType
	 * and property name in the sequence [@a property_names_begin, @a property_names_end).
	 *
	 * If so then the property values are returned in @a property_values.
	 * @a PropertyValueType and @a FeatureWeakRefType can be either const or non-const types,
	 * but you cannot get a non-const property value from a const feature handle.
	 *
	 * @a reconstruction_time only applies to time-dependent properties in which case the
	 * value of the property at the specified time is returned.
	 * It is effectively ignored for constant-valued properties.
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
			std::vector<PropertyValueType *> &property_values,
			const double &reconstruction_time = 0);


	////////////////////
	// Implementation //
	////////////////////
	namespace Implementation
	{
		//
		// NOTE: These function are templates purely so they can be defined in the ".cc" file to avoid
		// cyclic header dependencies including headers for irregular sampling and piecewise aggregation.
		//
		void
		visit_gpml_constant_value(
				GPlatesModel::ConstFeatureVisitor::gpml_constant_value_type &gpml_constant_value,
				GPlatesModel::ConstFeatureVisitor &visitor);
		void
		visit_gpml_constant_value(
				GPlatesModel::FeatureVisitor::gpml_constant_value_type &gpml_constant_value,
				GPlatesModel::FeatureVisitor &visitor);
		void
		visit_gpml_irregular_sampling_at_reconstruction_time(
				GPlatesModel::ConstFeatureVisitor::gpml_irregular_sampling_type &gpml_irregular_sampling,
				GPlatesModel::ConstFeatureVisitor &visitor,
				const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time);
		void
		visit_gpml_irregular_sampling_at_reconstruction_time(
				GPlatesModel::FeatureVisitor::gpml_irregular_sampling_type &gpml_irregular_sampling,
				GPlatesModel::FeatureVisitor &visitor,
				const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time);
		void
		visit_gpml_piecewise_aggregation_at_reconstruction_time(
				GPlatesModel::ConstFeatureVisitor::gpml_piecewise_aggregation_type &gpml_piecewise_aggregation,
				GPlatesModel::ConstFeatureVisitor &visitor,
				const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time);
		void
		visit_gpml_piecewise_aggregation_at_reconstruction_time(
				GPlatesModel::FeatureVisitor::gpml_piecewise_aggregation_type &gpml_piecewise_aggregation,
				GPlatesModel::FeatureVisitor &visitor,
				const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time);


		/**
		 * Base implementation class for feature visitor that finds all PropertyValue's of
		 * type PropertyValueType and with property name included in a specified list that are
		 * contained within a feature.
		 *
		 * NOTE: The property values can be constant over time or time-dependent - in the latter
		 * case the reconstruction time is used to lookup the property value.
		 */
		template <class FeatureVisitorType>
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
					typename FeatureVisitorType::top_level_property_inline_type &top_level_property_inline)
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
					typename FeatureVisitorType::gpml_constant_value_type &gpml_constant_value)
			{
				Implementation::visit_gpml_constant_value(gpml_constant_value, *this);
			}

			// In case property value is time-dependent.
			virtual
			void
			visit_gpml_irregular_sampling(
					typename FeatureVisitorType::gpml_irregular_sampling_type &gpml_irregular_sampling)
			{
				Implementation::visit_gpml_irregular_sampling_at_reconstruction_time(
						gpml_irregular_sampling, *this, d_reconstruction_time);
			}

			// In case property value is time-dependent.
			virtual
			void
			visit_gpml_piecewise_aggregation(
					typename FeatureVisitorType::gpml_piecewise_aggregation_type &gpml_piecewise_aggregation) 
			{
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
			features_iterator_type \
		) \
		namespace GPlatesFeatureVisitors \
		{ \
			namespace Implementation \
			{ \
				template <> \
				class PropertyValueFinder<property_value_type, feature_weak_ref_type> : \
						public PropertyValueFinderBase<feature_visitor_type> \
				{ \
				public: \
					typedef std::vector<property_value_type *> property_value_container_type; \
					typedef property_value_container_type::const_iterator property_value_container_const_iterator; \
					typedef std::pair<property_value_container_const_iterator, property_value_container_const_iterator> \
							property_value_container_range; \
					\
					PropertyValueFinder( \
							const double &reconstruction_time = 0) : \
						PropertyValueFinderBase<feature_visitor_type>(reconstruction_time) \
					{  } \
			 \
					explicit \
					PropertyValueFinder( \
							const GPlatesModel::PropertyName &property_name_to_allow, \
							const double &reconstruction_time = 0) : \
						PropertyValueFinderBase<feature_visitor_type>(property_name_to_allow, reconstruction_time) \
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
			/* const weak_ref/features_iterator for const property-value */ \
			DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
					const property_value_type, \
					visit_property_value_method, \
					GPlatesModel::ConstFeatureVisitor, \
					GPlatesModel::FeatureHandle::const_weak_ref, \
					GPlatesModel::FeatureCollectionHandle::const_iterator) \
			/* non-const weak_ref/features_iterator for const property-value */ \
			DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
					const property_value_type, \
					visit_property_value_method, \
					GPlatesModel::ConstFeatureVisitor, \
					GPlatesModel::FeatureHandle::weak_ref, \
					GPlatesModel::FeatureCollectionHandle::iterator) \

		// FIXME: Currently removing the version that returns a non-const property-value.
		//
		// When using a non-const feature visitor the top-level properties are currently deep cloned in
		// visit_feature_properties() so that changes to the model can be tracked (for JC's unsaved changes).
		// The result is references cannot be kept to the property values because they are released when
		// committed back the model inside visit_feature_properties().
		// The function 'get_property_value()' uses both non-const and const visitors internally to
		// retrieve property values and return them to the caller.
		// And for the non-const visitor the property value raw pointer that 'get_property_value()'
		// returns to its caller is pointing to an object that has been deallocated.
		// The design of 'get_property_value()' itself is ok in that it's returning a raw pointer to
		// the caller with the understanding that the pointer only be used locally by the caller.
		// However the new way of keeping track of changes to the model means that property value
		// references now become invalid immediately after visiting that property value with a
		// non-const visitor which is necessary to track changes to the model.
		// The const visitor does not clone because const property values cannot be modified and
		// so it doesn't exhibit this problem.
		//
		// So this only breaks for the non-const version of 'get_property_value()' so
		// it is disabled to prevent caller's using it.
		// There's currently no code that uses the non-const version.
		//
#if 0
		/* non-const weak_ref/features_iterator for non-const property-value */ \
		DECLARE_PROPERTY_VALUE_FINDER_CLASS( \
				property_value_type, \
				visit_property_value_method, \
				GPlatesModel::FeatureVisitor, \
				GPlatesModel::FeatureHandle::weak_ref, \
				GPlatesModel::FeatureCollectionHandle::iterator)
#endif
	}


	template <class PropertyValueType, class FeatureWeakRefType>
	bool
	get_property_values(
			const FeatureWeakRefType &feature_weak_ref,
			const GPlatesModel::PropertyName &property_name,
			std::vector<PropertyValueType *> &property_values,
			const double &reconstruction_time)
	{
		typedef Implementation::PropertyValueFinder<PropertyValueType, FeatureWeakRefType>
				property_value_finder_type;

		property_value_finder_type property_value_finder(property_name, reconstruction_time);

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
			std::vector<PropertyValueType *> &property_values,
			const double &reconstruction_time)
	{
		typedef Implementation::PropertyValueFinder<PropertyValueType, FeatureWeakRefType>
				property_value_finder_type;

		property_value_finder_type property_value_finder(reconstruction_time);

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
			PropertyValueType *&property_value,
			const double &reconstruction_time)
	{
		typedef Implementation::PropertyValueFinder<PropertyValueType, FeatureWeakRefType>
				property_value_finder_type;

		property_value_finder_type property_value_finder(property_name, reconstruction_time);

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
			PropertyValueType *&property_value,
			const double &reconstruction_time)
	{
		typedef Implementation::PropertyValueFinder<PropertyValueType, FeatureWeakRefType>
				property_value_finder_type;

		property_value_finder_type property_value_finder(reconstruction_time);

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
