/* $Id$ */

/**
 * \file 
 * Contains template specialisations for the templated FeatureVisitor class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

namespace GPlatesModel
{

	template<>
	void
	FeatureVisitorBase<FeatureHandle>::visit_feature_properties(
			feature_handle_type &feature_handle)
	{
		feature_iterator_type iter = feature_handle.begin();
		feature_iterator_type end = feature_handle.end();
		for ( ; iter != end; ++iter)
		{
			d_current_top_level_propiter = iter;
			d_current_top_level_propname = (*iter)->property_name();

			// Note that if you dereference a feature children iterator, you get a
			// TopLevelProperty::non_null_ptr_to_const_type. To modify properties in a
			// feature, you need to make a deep clone of the property, modify the clone
			// and then call set on the feature.
			TopLevelProperty::non_null_ptr_type prop_clone = (*iter)->deep_clone();
			prop_clone->accept_visitor(*this);
			feature_handle.set(iter, prop_clone);

			d_current_top_level_propiter = boost::none;
			d_current_top_level_propname = boost::none;
		}
	}


	template<>
	void
	FeatureVisitorBase<const FeatureHandle>::visit_feature_properties(
			feature_handle_type &feature_handle)
	{
		feature_iterator_type iter = feature_handle.begin();
		feature_iterator_type end = feature_handle.end();
		for ( ; iter != end; ++iter)
		{
			d_current_top_level_propiter = iter;
			d_current_top_level_propname = (*iter)->property_name();
			(*iter)->accept_visitor(*this);
			d_current_top_level_propiter = boost::none;
			d_current_top_level_propname = boost::none;
		}
	}

}

