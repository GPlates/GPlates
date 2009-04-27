/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009 The University of Sydney, Australia
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

#include "ConstFeatureVisitor.h"
#include "FeatureHandle.h"
#include "TopLevelPropertyInline.h"


GPlatesModel::ConstFeatureVisitor::~ConstFeatureVisitor()
{  }


void
GPlatesModel::ConstFeatureVisitor::visit_feature_properties(
		const FeatureHandle &feature_handle)
{
	FeatureHandle::properties_const_iterator iter = feature_handle.properties_begin();
	FeatureHandle::properties_const_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesModel::ConstFeatureVisitor::visit_property_values(
		const TopLevelPropertyInline &top_level_property_inline)
{
	TopLevelPropertyInline::const_iterator iter = top_level_property_inline.begin();
	TopLevelPropertyInline::const_iterator end = top_level_property_inline.end();
	for ( ; iter != end; ++iter) {
		(*iter)->accept_visitor(*this);
	}
}

