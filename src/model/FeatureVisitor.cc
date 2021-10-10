/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include "FeatureVisitor.h"
#include "FeatureHandle.h"
#include "InlinePropertyContainer.h"


GPlatesModel::FeatureVisitor::~FeatureVisitor()
{  }


void
GPlatesModel::FeatureVisitor::visit_feature_properties(
		FeatureHandle &feature_handle)
{
	// FIXME: Store the properties_iterator that was most recently visited,
	// as used in ReconstructedFeatureGeometryPopulator.
	FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesModel::FeatureVisitor::visit_property_values(
		InlinePropertyContainer &inline_property_container)
{
	InlinePropertyContainer::const_iterator iter = inline_property_container.begin();
	InlinePropertyContainer::const_iterator end = inline_property_container.end();
	for ( ; iter != end; ++iter) {
		(*iter)->accept_visitor(*this);
	}
}
