/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "PropertyValueSetter.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"


void
GPlatesFeatureVisitors::PropertyValueSetter::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// Visit only a specific property. All this really does is verify that
	// the property iterator does indeed belong to this feature handle - you
	// could just call (*it)->accept_visitor(PropertyValueSetter&) yourself.
	GPlatesModel::FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		if (iter == d_target_property_iter) {
			// Elements of this properties vector can be NULL pointers.  (See the comment in
			// "model/FeatureRevision.h" for more details.)
			if (iter.is_valid() && *iter != NULL) {
				(*iter)->accept_visitor(*this);
			}
		}
	}
}


void
GPlatesFeatureVisitors::PropertyValueSetter::visit_inline_property_container(
		GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	// If we get here, we've found the InlinePropertyContainer we want.
	// Now to do the assignment - to begin with, we assume we are only interested
	// in assigning to the first PropertyValue of the container.
	if (inline_property_container.begin() == inline_property_container.end()) {
		// FIXME: Fail, although really we should probably be adding a new value.
		// Why is this property container empty anyway?
		return;
	}
	
	d_old_property_value = *inline_property_container.begin();
	*inline_property_container.begin() = d_new_property_value;
}




