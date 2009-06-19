/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "AppLogicUtils.h"

#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/ConstFeatureVisitor.h"


void
GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
		GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		GPlatesModel::FeatureVisitor &visitor)
{
	// Before we dereference the weak_ref be sure that it's valid to dereference.
	if (feature_collection.is_valid())
	{
		GPlatesModel::FeatureCollectionHandle::features_iterator iter =
				feature_collection->features_begin();
		GPlatesModel::FeatureCollectionHandle::features_iterator end =
				feature_collection->features_end();
		for ( ; iter != end; ++iter)
		{
			// Check that it's ok to dereference the iterator.
			if (iter.is_valid())
			{
				visitor.visit_feature(iter);
			}
		}
	}
}


void
GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
		GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
		GPlatesModel::ConstFeatureVisitor &visitor)
{
	// Before we dereference the weak_ref be sure that it's valid to dereference.
	if (feature_collection.is_valid())
	{
		GPlatesModel::FeatureCollectionHandle::features_const_iterator iter =
				feature_collection->features_begin();
		GPlatesModel::FeatureCollectionHandle::features_const_iterator end =
				feature_collection->features_end();
		for ( ; iter != end; ++iter)
		{
			// Check that it's ok to dereference the iterator.
			if (iter.is_valid())
			{
				visitor.visit_feature(iter);
			}
		}
	}
}
