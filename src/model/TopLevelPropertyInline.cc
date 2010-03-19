/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class TopLevelPropertyInline.
 *
 * Most recent change:
 *   $Date$
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

#include "TopLevelPropertyInline.h"
#include "ConstFeatureVisitor.h"
#include "FeatureVisitor.h"


const GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesModel::TopLevelPropertyInline::deep_clone() const 
{
	TopLevelPropertyInline::non_null_ptr_type dup(
			new TopLevelPropertyInline(*this),
			GPlatesUtils::NullIntrusivePointerHandler());

	const_iterator iter, end_ = d_values.end();
	for (iter = d_values.begin(); iter != end_; ++iter) {
		PropertyValue::non_null_ptr_type cloned_pval = (*iter)->deep_clone_as_prop_val();
		dup->d_values.push_back(cloned_pval);
	}
	return TopLevelProperty::non_null_ptr_type(dup);
}


void
GPlatesModel::TopLevelPropertyInline::bubble_up_change(
		const PropertyValue *old_value,
		PropertyValue::non_null_ptr_type new_value,
		DummyTransactionHandle &transaction)
{
}


void
GPlatesModel::TopLevelPropertyInline::accept_visitor(
		ConstFeatureVisitor &visitor) const 
{
	visitor.visit_top_level_property_inline(*this);
}


void
GPlatesModel::TopLevelPropertyInline::accept_visitor(
		FeatureVisitor &visitor) 
{
	visitor.visit_top_level_property_inline(*this);
}
