/* $Id$ */

/**
 * \file 
 * Registry allowing fast lookup of FeatureHandle::weak_ref when given
 * a FeatureId.
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

#include "FeatureIdRegistry.h"


GPlatesModel::FeatureIdRegistry::FeatureIdRegistry()
{  }


void
GPlatesModel::FeatureIdRegistry::register_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	if (feature_ref.is_valid()) {
		const GPlatesModel::FeatureId &feature_id = feature_ref->feature_id();
		if ( ! find(feature_id)) {
			d_id_map.insert(std::make_pair(feature_id, feature_ref));
		} else {
			// Exception - feature id already exists in registry.
		}
	} else {
		// Exception - invalid weak_ref given for registration.
	}
}


void
GPlatesModel::FeatureIdRegistry::deregister_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	if (feature_ref.is_valid()) {
		d_id_map.erase(feature_ref->feature_id());
	} else {
		// Exception - invalid weak_ref given for registration.
	}
}


boost::optional<GPlatesModel::FeatureHandle::weak_ref>
GPlatesModel::FeatureIdRegistry::find(
		const GPlatesModel::FeatureId &feature_id)
{
	id_map_const_iterator result = d_id_map.find(feature_id);
	if (result != d_id_map.end()) {
		return result->second;
	} else {
		return boost::none;
	}
}


