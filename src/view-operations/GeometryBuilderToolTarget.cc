/* $Id$ */

/**
 * \file 
 * Determines which @a GeometryBuilder and main rendered layer each
 * geometry builder canvas tool should target.
 * $Revision$
 * $Date$
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

#include "GeometryBuilderToolTarget.h"
#include "RenderedGeometryCollection.h"
#include "global/ControlFlowException.h"


GPlatesViewOperations::GeometryBuilderToolTarget::GeometryBuilderToolTarget(
		GeometryBuilder &digitise_geom_builder,
		GeometryBuilder &focused_feature_geom_builder,
		const RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesGui::FeatureFocus &feature_focus) :
d_digitise_geom_builder(&digitise_geom_builder),
d_focused_feature_geom_builder(&focused_feature_geom_builder),
d_rendered_geom_collection(&rendered_geom_collection),
d_feature_focus(&feature_focus),
d_is_geometry_in_focus(false),
d_main_rendered_layer_active_state(
		rendered_geom_collection.capture_main_layer_active_state()),
d_current_tool_type(DIGITISE_GEOMETRY)
{
	initialise_targets();

	connect_to_feature_focus(feature_focus);
	connect_to_rendered_geom_collection(rendered_geom_collection);

	// Start off with digitise geometry tool.
	activate(DIGITISE_GEOMETRY);
}

void
GPlatesViewOperations::GeometryBuilderToolTarget::activate(
		ToolType tool_type)
{
	d_current_tool_type = tool_type;

	update();
}

GPlatesViewOperations::GeometryBuilder *
GPlatesViewOperations::GeometryBuilderToolTarget::get_geometry_builder_for_active_tool() const
{
	return d_current_geom_builder_targets[d_current_tool_type];
}

GPlatesViewOperations::RenderedGeometryCollection::MainLayerType
GPlatesViewOperations::GeometryBuilderToolTarget::get_main_rendered_layer_for_active_tool() const
{
	return d_current_main_layer_targets[d_current_tool_type];
}

void
GPlatesViewOperations::GeometryBuilderToolTarget::connect_to_feature_focus(
		const GPlatesGui::FeatureFocus &feature_focus)
{
	QObject::connect(
		&feature_focus,
		SIGNAL(focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
		this,
		SLOT(set_focus(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));
}

void
GPlatesViewOperations::GeometryBuilderToolTarget::connect_to_rendered_geom_collection(
		const RenderedGeometryCollection &rendered_geom_collection)
{
	QObject::connect(
		&rendered_geom_collection,
		SIGNAL(collection_was_updated(
				GPlatesViewOperations::RenderedGeometryCollection &,
				GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
		this,
		SLOT(collection_was_updated(
				GPlatesViewOperations::RenderedGeometryCollection &,
				GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)));
}

void
GPlatesViewOperations::GeometryBuilderToolTarget::initialise_targets()
{
	// Initially all the tools target the new geometry builder.
	for (int tool_index = 0; tool_index < NUM_TOOLS; ++tool_index)
	{
		d_current_geom_builder_targets[tool_index] = d_digitise_geom_builder;
		d_current_main_layer_targets[tool_index] = RenderedGeometryCollection::DIGITISATION_LAYER;
	}
}

void
GPlatesViewOperations::GeometryBuilderToolTarget::update()
{
	update_move_vertex();
}

void
GPlatesViewOperations::GeometryBuilderToolTarget::update_move_vertex()
{
	//
	// See if geometry builder has changed.
	//

	GeometryBuilder *new_move_vertex_geom_builder = target_focus_geometry()
		? d_focused_feature_geom_builder
		: d_digitise_geom_builder;

	if (new_move_vertex_geom_builder != d_current_geom_builder_targets[MOVE_VERTEX])
	{
		d_current_geom_builder_targets[MOVE_VERTEX] = new_move_vertex_geom_builder;

		emit switched_move_vertex_geometry_builder(*this, new_move_vertex_geom_builder);
	}

	//
	// See if main rendered layer has changed.
	//

	RenderedGeometryCollection::MainLayerType new_main_layer_type = target_focus_geometry()
		? RenderedGeometryCollection::GEOMETRY_FOCUS_MANIPULATION_LAYER
		: RenderedGeometryCollection::DIGITISATION_LAYER;

	if (new_main_layer_type != d_current_main_layer_targets[MOVE_VERTEX])
	{
		d_current_main_layer_targets[MOVE_VERTEX] = new_main_layer_type;

		emit switched_move_vertex_main_rendered_layer(*this, new_main_layer_type);
	}
}

bool
GPlatesViewOperations::GeometryBuilderToolTarget::target_focus_geometry() const
{
	// Use the focus geometry builder/layer if there is geometry in focus and
	// the geometry focus rendered layer is currently active (which means the
	// focused geometry is visible).
	if (d_is_geometry_in_focus)
	{
		if (d_main_rendered_layer_active_state.is_active(
					RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER))
		{
			return true;
		}

		if (d_main_rendered_layer_active_state.is_active(
					RenderedGeometryCollection::GEOMETRY_FOCUS_MANIPULATION_LAYER))
		{
			return true;
		}
	}

	return false;
}

void
GPlatesViewOperations::GeometryBuilderToolTarget::set_focus(
		GPlatesModel::FeatureHandle::weak_ref /*feature_ref*/,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry)
{
	d_is_geometry_in_focus = focused_geometry;

	update();
}

void
GPlatesViewOperations::GeometryBuilderToolTarget::collection_was_updated(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type /*main_layers_updated*/)
{
	d_main_rendered_layer_active_state = rendered_geom_collection.capture_main_layer_active_state();

	update();
}
