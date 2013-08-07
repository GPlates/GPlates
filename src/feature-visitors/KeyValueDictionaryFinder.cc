/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#include "KeyValueDictionaryFinder.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlKeyValueDictionary.h"

namespace
{
	template<typename C, typename E>
	bool
	contains_elem(
			const C &container,
			const E &elem)
	{
		return (std::find(container.begin(), container.end(), elem) != container.end());
	}
}


bool
GPlatesFeatureVisitors::KeyValueDictionaryFinder::initialise_pre_property_values(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	const GPlatesModel::PropertyName &curr_prop_name = top_level_property_inline.get_property_name();

	if ( ! d_property_names_to_allow.empty()) {
		// We're not allowing all property names.
		if ( ! contains_elem(d_property_names_to_allow, curr_prop_name)) {
			// The current property name is not allowed.
			return false;
		}
	}	
	return true;
}


void
GPlatesFeatureVisitors::KeyValueDictionaryFinder::visit_gpml_key_value_dictionary(
		const GPlatesPropertyValues::GpmlKeyValueDictionary &dictionary)
{

	d_found_key_value_dictionaries.push_back(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type(&dictionary,
			GPlatesUtils::NullIntrusivePointerHandler()));
}
