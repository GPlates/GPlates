/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include <algorithm>  // std::find

#include "PlateIdFinder.h"

#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"


void
GPlatesFeatureVisitors::PlateIdFinder::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Now visit each of the properties in turn.
	GPlatesModel::FeatureHandle::properties_const_iterator iter =
			feature_handle.properties_begin();
	GPlatesModel::FeatureHandle::properties_const_iterator end =
			feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			(*iter)->accept_visitor(*this);
		}
	}
}


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


void
GPlatesFeatureVisitors::PlateIdFinder::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	const GPlatesModel::PropertyName &curr_prop_name = inline_property_container.property_name();

	if (d_property_names_to_allow.empty()) {
		// We're allowing all property names.
		if ( ! contains_elem(d_property_names_to_allow, curr_prop_name)) {
			// The current property name is not allowed.
			return;
		}
	}

	GPlatesModel::InlinePropertyContainer::const_iterator iter =
			inline_property_container.begin(); 
	GPlatesModel::InlinePropertyContainer::const_iterator end =
			inline_property_container.end(); 
	for ( ; iter != end; ++iter) {
		(*iter)->accept_visitor(*this);
	}
}


void
GPlatesFeatureVisitors::PlateIdFinder::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::PlateIdFinder::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_found_plate_ids.push_back(gpml_plate_id.value());
}
