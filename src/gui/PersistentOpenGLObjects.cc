/* $Id$ */
 
/**
 * \file 
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

#include "PersistentOpenGLObjects.h"

#include "app-logic/ApplicationState.h"

#include "opengl/GLMultiResolutionRasterNode.h"
#include "opengl/GLMultiResolutionReconstructedRasterNode.h"


GPlatesGui::PersistentOpenGLObjects::PersistentOpenGLObjects(
		const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
		GPlatesAppLogic::ApplicationState &application_state) :
	d_list_objects(new ListObjects(opengl_context->get_shared_state())),
	d_non_list_objects(new NonListObjects())
{
	make_signal_slot_connections(application_state.get_reconstruct_graph());
}


GPlatesGui::PersistentOpenGLObjects::PersistentOpenGLObjects(
		const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
		const PersistentOpenGLObjects::non_null_ptr_type &objects_from_another_context,
		GPlatesAppLogic::ApplicationState &application_state) :
	// Non-list objects can always be shared
	d_non_list_objects(objects_from_another_context->d_non_list_objects)
{
	// If the OpenGL context shared state for 'this' object is the same as the 'other' object
	// then we can share the list objects.
	if (opengl_context->get_shared_state() ==
		objects_from_another_context->get_list_objects().get_opengl_shared_state())
	{
		d_list_objects = objects_from_another_context->d_list_objects;
	}
	else
	{
		d_list_objects.reset(new ListObjects(opengl_context->get_shared_state()));
	}

	make_signal_slot_connections(application_state.get_reconstruct_graph());
}


void
GPlatesGui::PersistentOpenGLObjects::make_signal_slot_connections(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph)
{
	// Listen in to when a layer gets removed.
	QObject::connect(
			&reconstruct_graph,
			SIGNAL(layer_about_to_be_removed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(handle_layer_about_to_be_removed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)));
}


void
GPlatesGui::PersistentOpenGLObjects::handle_layer_about_to_be_removed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
}


boost::optional<GPlatesOpenGL::GLRenderGraphNode::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::ListObjects::get_or_create_raster_render_graph_node(
		const GPlatesAppLogic::Layer &layer,
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &source_raster,
		const boost::optional<GPlatesGui::RasterColourScheme::non_null_ptr_type> &raster_colour_scheme,
		const boost::optional<GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type> &
				reconstruct_raster_polygons)
{
	RasterBuilder::Raster &raster = d_raster_builder.layer_to_raster_map[layer];

	const RasterBuilder::Raster &old_raster = raster;

	RasterBuilder::Raster new_raster;
	new_raster.input.georeferencing = georeferencing;
	new_raster.input.source_raster = source_raster;
	new_raster.input.raster_colour_scheme = raster_colour_scheme;
	new_raster.input.reconstruct_raster_polygons = reconstruct_raster_polygons;

	// First see if we need to update the source multi-resolution raster.
	const bool was_source_multi_resolution_raster_updated =
			update_source_multi_resolution_raster(old_raster, new_raster);

	// If no source multi-resolution raster exists then there's nothing left to do
	// because everything depends on that raster.
	if (!new_raster.output.source_multi_resolution_raster)
	{
		raster = new_raster; // Write back any changes made during update.
		return boost::none;
	}

	// If there's no polygons to reconstruct with then just return the
	// source multi-resolution raster - it'll get displayed, just not reconstructed.
	if (!new_raster.input.reconstruct_raster_polygons)
	{
		raster = new_raster; // Write back any changes made during update.
		GPlatesOpenGL::GLRenderGraphNode::non_null_ptr_type render_graph_node =
				GPlatesOpenGL::GLMultiResolutionRasterNode::create(
						new_raster.output.source_multi_resolution_raster.get());
		return render_graph_node;
	}

	// We need to update the source multi-resolution cube raster if the non-cube
	// version was updated - because it depends on it.
	if (was_source_multi_resolution_raster_updated)
	{
		new_raster.output.source_multi_resolution_cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						new_raster.output.source_multi_resolution_raster.get(),
						GPlatesOpenGL::GLMultiResolutionReconstructedRaster::get_cube_subdivision(),
						d_opengl_shared_state->get_texture_resource_manager());
	}
	else
	{
		// Otherwise we can just keep the previous one.
		new_raster.output.source_multi_resolution_cube_raster =
				old_raster.output.source_multi_resolution_cube_raster;
	}

	// TODO: Implement an efficient update of the reconstructed raster when the
	// raster polygons have not changed.
	if (was_source_multi_resolution_raster_updated ||
		new_raster.input.reconstruct_raster_polygons != old_raster.input.reconstruct_raster_polygons)
	{
		new_raster.output.source_multi_resolution_reconstructed_raster =
				GPlatesOpenGL::GLMultiResolutionReconstructedRaster::create(
						new_raster.output.source_multi_resolution_cube_raster.get(),
						new_raster.input.reconstruct_raster_polygons.get(),
						d_opengl_shared_state->get_texture_resource_manager());
	}
	else
	{
		// Otherwise we can keep the existing multi-resolution reconstructing raster.
		new_raster.output.source_multi_resolution_reconstructed_raster =
				old_raster.output.source_multi_resolution_reconstructed_raster;
	}

	raster = new_raster; // Write back any changes made.
	GPlatesOpenGL::GLRenderGraphNode::non_null_ptr_type render_graph_node =
			GPlatesOpenGL::GLMultiResolutionReconstructedRasterNode::create(
					new_raster.output.source_multi_resolution_reconstructed_raster.get());
	return render_graph_node;
}


bool
GPlatesGui::PersistentOpenGLObjects::ListObjects::update_source_multi_resolution_raster(
		const RasterBuilder::Raster &old_raster,
		RasterBuilder::Raster &new_raster)
{
	// If an old raster does not exist then attempt to create a new one.
	if (!old_raster.output.source_multi_resolution_raster)
	{
		// Create a new raster.
		new_raster.output.source_multi_resolution_raster =
				GPlatesOpenGL::GLMultiResolutionRaster::create(
						new_raster.input.georeferencing.get(),
						new_raster.input.source_raster.get(),
						new_raster.input.raster_colour_scheme,
						d_opengl_shared_state->get_texture_resource_manager());

		// Return true if successfully created.
		return new_raster.output.source_multi_resolution_raster;
	}

	// If the georeferencing has changed then we need to rebuild the raster from scratch.
	if (new_raster.input.georeferencing != old_raster.input.georeferencing)
	{
		new_raster.output.source_multi_resolution_raster =
				GPlatesOpenGL::GLMultiResolutionRaster::create(
						new_raster.input.georeferencing.get(),
						new_raster.input.source_raster.get(),
						new_raster.input.raster_colour_scheme,
						d_opengl_shared_state->get_texture_resource_manager());

		// Return true if successfully created.
		return new_raster.output.source_multi_resolution_raster;
	}

	// If the raster data and colour scheme have not changed then the raster is fine as it is.
	if (new_raster.input.source_raster == old_raster.input.source_raster &&
		new_raster.input.raster_colour_scheme == old_raster.input.raster_colour_scheme)
	{
		new_raster.output.source_multi_resolution_raster =
				old_raster.output.source_multi_resolution_raster;

		// NOTE: Return false to indicate that no update was necessary.
		return false;
	}

	// Otherwise we can keep the existing raster but change the raster data and/or colour scheme.
	new_raster.output.source_multi_resolution_raster =
			old_raster.output.source_multi_resolution_raster;

	if (new_raster.output.source_multi_resolution_raster.get()->change_raster(
			new_raster.input.source_raster.get(), new_raster.input.raster_colour_scheme))
	{
		return true;
	}

	// We were unable to change the raster data (probably because the raster dimensions changed).
	// So create a new one from scratch.
	new_raster.output.source_multi_resolution_raster =
			GPlatesOpenGL::GLMultiResolutionRaster::create(
					new_raster.input.georeferencing.get(),
					new_raster.input.source_raster.get(),
					new_raster.input.raster_colour_scheme,
					d_opengl_shared_state->get_texture_resource_manager());

	// Return true if successfully created.
	return new_raster.output.source_multi_resolution_raster;
}
