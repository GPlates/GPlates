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
	FeatureVisitorBase<FeatureHandle>::visit_feature_property(
			const feature_iterator_type &feature_iterator)
	{
		// Note that if you dereference a feature children iterator, you get a
		// TopLevelProperty::non_null_ptr_to_const_type. To modify properties in a
		// feature, you need to make a deep clone of the property, modify the clone
		// and then call set on the feature.
		TopLevelProperty::non_null_ptr_type prop_clone = (*feature_iterator)->deep_clone();
		prop_clone->accept_visitor(*this);
		*feature_iterator = prop_clone;
	}


	template<>
	void
	FeatureVisitorBase<const FeatureHandle>::visit_feature_property(
			const feature_iterator_type &feature_iterator)
	{
		(*feature_iterator)->accept_visitor(*this);
	}
}


void
GPlatesModel::FeatureVisitorThatGuaranteesNotToModify::visit_feature_property(
		const feature_iterator_type &feature_iterator)
{
	TopLevelProperty::non_null_ptr_to_const_type const_prop = *feature_iterator;

	// HACK: This is the hack that this whole class is built upon (to avoid cloning properties).
	// Cast away the const from the TopLevelProperty.
	// The class derived from this class guarantees that they won't modify the property values.
	// The derived class just wants non-const references.
	// Note that for FeatureVisitor (which is non-const) the property value references are
	// not valid after visitation because the cloned property is set in the model (which does
	// another clone) and so the first clone (that the references reference) is destroyed
	// making the references invalid.
	// So why would the derived class want non-const reference if they're not going
	// to modify the property values ?
	// This is probably because they want a non-const reference to the feature being
	// visited - presumably to pass to other objects that will later modify the feature
	// (after this visitor has finished visiting all property values and returned).
	TopLevelProperty *prop = const_cast<TopLevelProperty *>(const_prop.get());

	prop->accept_visitor(*this);
}
