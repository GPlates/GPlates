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

#include "gui/FeatureFocus.h"

#include "model/FeatureHandle.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelUtils.h"

void
GPlatesViewOperations::CloneOperation::clone_focused_geometry()
{
	// There might not be a geometry in the focused feature geometry builder.
	// This can happen when the user selects a topological plate boundary - it is not
	// inserted into the builder because it shouldn't be modified (because it references
	// other features).
	//
	// FIXME: we should disable the 'clone geometry' button in this case so that this
	// code never gets called (and so the user doesn't think they can clone a topology).
	//
	// Actually it probably should be possible to clone the geometry of a topological polygon -
	// the only reason it's prevented in FocusedFeatureGeometryManipulator is so tools
	// like MoveVertex, etc don't try to move a vertex in a topology which makes little sense.
	// This would require some significant changes though so it'll have to wait a while.
	//
	// For now simply refuse to clone geometry if the focused feature geometry builder
	// contains no geometry.
	if (!d_focused_feature_geometry_builder->has_geometry())
	{
		return;
	}

	GPlatesViewOperations::GeometryType::Value type = 
		d_focused_feature_geometry_builder->get_actual_type_of_current_geometry();

	switch (type)
	{
	case GPlatesViewOperations::GeometryType::POLYLINE: 
		d_choose_canvas_tool->choose_digitise_polyline_tool();		
		break;

	case GPlatesViewOperations::GeometryType::MULTIPOINT:
		d_choose_canvas_tool->choose_digitise_multipoint_tool();
		break;

	case GPlatesViewOperations::GeometryType::POLYGON:
		d_choose_canvas_tool->choose_digitise_polygon_tool();	
		break;

	default:
		// A digitise geometry tool has never been chosen yet, so do nothing.
		return;
	}
	d_digitise_geometry_builder->set_geometry(
			type,
			d_focused_feature_geometry_builder->get_geometry_point_begin(0),
			d_focused_feature_geometry_builder->get_geometry_point_end(0));
}

void 
GPlatesViewOperations::CloneOperation::clone_focused_feature()
{
	GPlatesModel::FeatureHandle::weak_ref feature_ref = 
			d_view_state.get_feature_focus().focused_feature();

	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
			*GPlatesAppLogic::get_feature_collection_containing_feature(
					d_view_state.get_application_state().get_feature_collection_file_state(),
					feature_ref);

	GPlatesModel::FeatureHandle::non_null_ptr_type new_feature_ptr = feature_ref->clone();

	// GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
	feature_collection_ref->add(new_feature_ptr);
	// transaction.commit();

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
		d_focused_feature_geometry_builder->get_geometry_point_begin(0), 
		d_focused_feature_geometry_builder->get_geometry_point_end(0),
		d_focused_feature_geometry_builder->get_actual_type_of_current_geometry()), 
		(*geo_property_iter)->property_name(),
		new_feature_ref);
#endif

	//set focus to the new feature 
	GPlatesModel::FeatureHandle::iterator geo_property_iter =
			*GPlatesFeatureVisitors::find_first_geometry_property(new_feature_ptr->reference());
	d_view_state.get_feature_focus().set_focus(
			new_feature_ptr->reference(),
			geo_property_iter);
	d_view_state.get_feature_focus().announce_modification_of_focused_feature();
}

