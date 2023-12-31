/* $Id$ */

/**
 * \file 
 * Contains the implementation of the TopLevelPropertyRef class.
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

#include "TopLevelPropertyRef.h"

#include "FeatureHandle.h"
#include "TopLevelProperty.h"


GPlatesModel::TopLevelPropertyRef::TopLevelPropertyRef(
		const FeatureHandle::iterator &iterator) :
	d_feature_ref(iterator.handle_weak_ref()),
	d_index(iterator.index())
{
}


GPlatesModel::TopLevelPropertyRef::TopLevelPropertyRef(
		const TopLevelPropertyRef &other) :
	GPlatesUtils::SafeBool<TopLevelPropertyRef>(other),
	d_feature_ref(other.d_feature_ref),
	d_index(other.d_index)
{
}


GPlatesModel::TopLevelPropertyRef::~TopLevelPropertyRef()
{
}


GPlatesModel::TopLevelPropertyRef::operator GPlatesGlobal::PointerTraits<const GPlatesModel::TopLevelProperty>::non_null_ptr_type() const
{
	return pointer();
}


const GPlatesModel::TopLevelProperty *
GPlatesModel::TopLevelPropertyRef::operator->() const
{
	return pointer().get();
}


const GPlatesModel::TopLevelProperty *
GPlatesModel::TopLevelPropertyRef::get() const
{
	return pointer().get();
}


const GPlatesModel::TopLevelProperty &
GPlatesModel::TopLevelPropertyRef::operator*() const
{
	return *(pointer());
}


void
GPlatesModel::TopLevelPropertyRef::operator=(
		GPlatesGlobal::PointerTraits<const TopLevelProperty>::non_null_ptr_type new_property)
{
	if (d_index != INVALID_INDEX && d_feature_ref)
	{
		d_feature_ref->set(FeatureHandle::iterator(*d_feature_ref, d_index), new_property);
	}
}


bool
GPlatesModel::TopLevelPropertyRef::boolean_test() const
{
	return pointer();
}


GPlatesGlobal::PointerTraits<const GPlatesModel::TopLevelProperty>::non_null_ptr_type
GPlatesModel::TopLevelPropertyRef::pointer() const
{
	return d_feature_ref->get(d_index);
}
