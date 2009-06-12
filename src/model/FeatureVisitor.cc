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

#include <iostream>
#include <boost/none.hpp>

#include "FeatureVisitor.h"
#include "TopLevelPropertyInline.h"


GPlatesModel::FeatureVisitor::~FeatureVisitor()
{  }


void
GPlatesModel::FeatureVisitor::visit_feature_properties(
		FeatureHandle &feature_handle)
{
	// FIXME: Store the properties_iterator that was most recently visited,
	// as used in ReconstructedFeatureGeometryPopulator and
	// QueryFeaturePropertiesWidgetPopulator and
	// ViewFeatureGeometriesWidgetPopulator.
	FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		if (iter.is_valid()) {
			d_current_top_level_propname = (*iter)->property_name();
			(*iter)->accept_visitor(*this);
			d_current_top_level_propname = boost::none;
		}
	}
}


void
GPlatesModel::FeatureVisitor::visit_property_values(
		TopLevelPropertyInline &top_level_property_inline)
{
	TopLevelPropertyInline::const_iterator iter = top_level_property_inline.begin();
	TopLevelPropertyInline::const_iterator end = top_level_property_inline.end();
	for ( ; iter != end; ++iter) {
		(*iter)->accept_visitor(*this);
	}
}


void
GPlatesModel::FeatureVisitor::log_invalid_weak_ref(
		const FeatureHandle::weak_ref &feature_weak_ref)
{
	std::cerr << "invalid weak-ref not dereferenced." << std::endl;
}


void
GPlatesModel::FeatureVisitor::log_invalid_iterator(
		const FeatureCollectionHandle::features_iterator &iterator)
{
	std::cerr << "invalid iterator not dereferenced." << std::endl;
}
