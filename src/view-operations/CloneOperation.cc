/* $Id$ */

/**
 * \file Interface for activating/deactivating geometry operations.
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

#include "CloneOperation.h"

#include "app-logic/ApplicationState.h"

#include "feature-visitors/GeometryTypeFinder.h"

#include "gui/CanvasToolWorkflows.h"
#include "gui/FeatureFocus.h"

#include "model/FeatureHandle.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/TopLevelProperty.h"


void
GPlatesViewOperations::CloneOperation::clone_focused_geometry()
{
	// It's currently possible to clone the geometry of a topological polygon -
	// the only reason it's prevented (in FocusedFeatureGeometryManipulator) is so tools
	// like MoveVertex, etc don't try to move a vertex in a topology which makes little sense.
	// But cloning a reconstruction-time snapshot of the dynamic polygon is fine.
	// Also a resolved topological network can be cloned but only the boundary is cloned.

	if (!d_focused_feature_geometry_builder.has_geometry())
	{
		return;
	}

	const GPlatesMaths::GeometryType::Value geometry_type = 
		d_focused_feature_geometry_builder.get_actual_type_of_current_geometry();

	// NOTE: We access the focused feature geometry builder *before* we switch to the
	// digitise workflow because once we switch there's no longer a feature in focus and hence
	// there's no longer any geometry in the focused feature geometry builder.
	d_digitise_geometry_builder.set_geometry(
			geometry_type,
			d_focused_feature_geometry_builder.get_geometry_point_begin(0),
			d_focused_feature_geometry_builder.get_geometry_point_end(0));

	switch (geometry_type)
	{
	case GPlatesMaths::GeometryType::POLYLINE:
		d_canvas_tool_workflows.choose_canvas_tool(
				GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYLINE);
		break;

	case GPlatesMaths::GeometryType::MULTIPOINT:
		d_canvas_tool_workflows.choose_canvas_tool(
				GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_MULTIPOINT);
		break;

	case GPlatesMaths::GeometryType::POLYGON:
		d_canvas_tool_workflows.choose_canvas_tool(
				GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYGON);
		break;

	default:
		// A digitise geometry tool has never been chosen yet, so do nothing.
		return;
	}
}

void 
GPlatesViewOperations::CloneOperation::clone_focused_feature(
		GPlatesModel::FeatureCollectionHandle::weak_ref target_feature_collection)
{
	// We want to merge model events across this scope so that only one model event
	// is generated instead of many as we incrementally modify the feature below.
	GPlatesModel::NotificationGuard model_notification_guard(
			*d_view_state.get_application_state().get_model_interface().access_model());

	GPlatesModel::FeatureHandle::weak_ref feature_ref = 
			d_view_state.get_feature_focus().focused_feature();

	// Use the feature's feature collection if target_feature_collection is invalid.
	if (!target_feature_collection.is_valid())
	{
		if (!feature_ref->parent_ptr())
		{
			return;
		}
		target_feature_collection = feature_ref->parent_ptr()->reference();
	}

	// FIXME: We don't use FeatureHandle::clone() here because it currently does a shallow copy
	// instead of a deep copy - the clone would share TopLevelProperty/PropertyValue objects with
	// the original feature, and (unlike the old gplates model) an in-place property-value edit can
	// now mutate a shared property object without going through FeatureHandle::set(), silently
	// modifying both features at once. So instead we build the clone by deep-cloning each property,
	// mirroring the workaround already used in GPlatesApi::feature_handle_clone()
	// (see src/api/PyFeature.cc). Once FeatureHandle has been updated to use the same revisioning
	// system as TopLevelProperty and PropertyValue then just delegate directly to
	// FeatureHandle::clone() (see pygplates/MERGE-EXECUTION-DETAILS.md section 8).
	GPlatesModel::FeatureHandle::non_null_ptr_type new_feature_ptr =
			GPlatesModel::FeatureHandle::create(feature_ref->feature_type());
	for (GPlatesModel::FeatureHandle::iterator properties_iter = feature_ref->begin();
			properties_iter != feature_ref->end();
			++properties_iter)
	{
		GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;
		new_feature_ptr->add(feature_property->clone());
	}

	target_feature_collection->add(new_feature_ptr);
		
	// We release the model notification guard which will cause a reconstruction to occur
	// because we modified the model - provided there are no nested higher-level guards.
	model_notification_guard.release_guard();

#if 0
	//Since the clone_feature function only does a "shallow copy" of geometry property,
	//we exclude geometry property when cloning the feature.
	//The geometry property will be appended to the feature later
	GPlatesModel::FeatureHandle::weak_ref  new_feature_ref = 
		GPlatesModel::ModelUtils::clone_feature(
		feature_ref,
		feature_collection_ref,
		get_view_state().get_application_state().get_model_interface(),
		&GPlatesFeatureVisitors::is_not_geometry_property);

	GPlatesModel::FeatureHandle::children_iterator geo_property_iter =
		*GPlatesFeatureVisitors::find_first_geometry_property(feature_ref);

	//create the geometry property and append to feature
	GPlatesModel::ModelUtils::append_property_value_to_feature(
		*GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
		d_focused_feature_geometry_builder.get_geometry_point_begin(0), 
		d_focused_feature_geometry_builder.get_geometry_point_end(0),
		d_focused_feature_geometry_builder.get_actual_type_of_current_geometry()), 
		(*geo_property_iter)->property_name(),
		new_feature_ref);
#endif

	// Set focus to the new feature. This might have led to ambiguity in the past,
	// but now that we indicate creation time in the clicked feature table this should be
	// less prone to causing an awkward user experience.
	// Also, focusing the clone after a duplication operation is common behaviour in
	// vector graphics software.
	d_view_state.get_feature_focus().set_focus(new_feature_ptr->reference());

	d_view_state.get_feature_focus().announce_modification_of_focused_feature();
}

