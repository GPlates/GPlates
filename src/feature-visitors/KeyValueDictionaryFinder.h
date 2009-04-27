/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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

#ifndef GPLATES_FEATUREVISITORS_KEYVALUEDICTIONARYFINDER_H
#define GPLATES_FEATUREVISITORS_KEYVALUEDICTIONARYFINDER_H

#include <vector>
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyName.h"
#include "property-values/GpmlKeyValueDictionary.h"

namespace GPlatesFeatureVisitors
{
	/**
	 * This const feature visitor finds key value dictionaries in the feature collection.
	 */
	class KeyValueDictionaryFinder:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		typedef std::vector<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> 
			key_value_dictionary_container_type;
		typedef key_value_dictionary_container_type::const_iterator key_value_dictionary_container_const_iterator;


		KeyValueDictionaryFinder()
		{  }

		explicit
		KeyValueDictionaryFinder(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		~KeyValueDictionaryFinder() {  }

		void
		add_property_name_to_allow(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_top_level_property_inline(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		virtual
		void
		visit_gpml_key_value_dictionary(
				const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);



		key_value_dictionary_container_const_iterator
		found_key_value_dictionaries_begin() const
		{
			return d_found_key_value_dictionaries.begin();
		}

		key_value_dictionary_container_const_iterator
		found_key_value_dictionaries_end() const
		{
			return d_found_key_value_dictionaries.end();
		}

		unsigned int
		number_of_found_dictionaries()
		{
			return static_cast<unsigned int>(d_found_key_value_dictionaries.size());
		}

	private:
		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		key_value_dictionary_container_type d_found_key_value_dictionaries;
	};
}

#endif // GPLATES_FEATUREVISITORS_KEYVALUEDICTIONARYFINDER_H
