/* $Id$ */

/**
 * \file
 * Contains the implementation of the DeleteFeatureOperation class.
 *
 * $Revision$
 * $Date$
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

#include "DeleteFeatureOperation.h"

#include "app-logic/ApplicationState.h"

#include "gui/FeatureFocus.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"


GPlatesViewOperations::DeleteFeatureOperation::DeleteFeatureOperation(
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesAppLogic::ApplicationState &application_state) :
	d_feature_focus(feature_focus),
	d_application_state(application_state)
{
}


void
GPlatesViewOperations::DeleteFeatureOperation::delete_focused_feature()
{
	if (d_feature_focus.is_valid())
	{
		GPlatesModel::FeatureHandle::weak_ref feature_ref = d_feature_focus.focused_feature();
		if (feature_ref)
		{
			GPlatesModel::FeatureCollectionHandle *feature_collection_ptr = feature_ref->parent_ptr();
			if (feature_collection_ptr)
			{
				d_feature_focus.unset_focus();

				GPlatesModel::container_size_type index = feature_ref->index_in_container();
				GPlatesModel::FeatureCollectionHandle::iterator iter_to_feature(
						*feature_collection_ptr, index);
				feature_collection_ptr->remove(iter_to_feature);

				d_application_state.reconstruct();
			}
		}
	}
}

