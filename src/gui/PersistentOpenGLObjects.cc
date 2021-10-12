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

#include "RasterColourPalette.h"
#include "RasterColourScheme.h"

#include "app-logic/ApplicationState.h"

#include "opengl/GLMultiResolutionRasterNode.h"
#include "opengl/GLMultiResolutionReconstructedRasterNode.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"


namespace 
{
	// Setup a default colour scheme for non-RGBA rasters...
	// This should work for all raster types.
	boost::optional<GPlatesGui::RasterColourScheme::non_null_ptr_type>
	create_default_raster_colour_scheme(
			const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raw_raster)
	{
		GPlatesPropertyValues::RasterStatistics *statistics_ptr =
			GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*raw_raster);
		if (!statistics_ptr)
		{
			return boost::none;
		}

		GPlatesPropertyValues::RasterStatistics &statistics = *statistics_ptr;
		if (!statistics.mean || !statistics.standard_deviation)
		{
			return boost::none;
		}
		double mean = *statistics.mean;
		double std_dev = *statistics.standard_deviation;
		GPlatesGui::DefaultRasterColourPalette::non_null_ptr_type rgba8_palette =
				GPlatesGui::DefaultRasterColourPalette::create(mean, std_dev);

		return GPlatesGui::RasterColourScheme::create<double>("band name", rgba8_palette);
	}
} // anonymous namespace


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
	d_list_objects->release_layer(layer);
}


boost::optional<GPlatesOpenGL::GLRenderGraphNode::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::ListObjects::get_raster_render_graph_node(
		const GPlatesAppLogic::Layer &layer,
		const double &reconstruction_time,
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &source_georeferencing,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &source_raster,
		const boost::optional<GPlatesGui::RasterColourScheme::non_null_ptr_type> &source_raster_colour_scheme,
		const boost::optional<GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type> &
				reconstruct_raster_polygons,
		const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &age_grid_georeferencing,
		const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &age_grid_raster)
{
	RasterBuilder::Raster &raster = d_raster_builder.layer_to_raster_map[layer];

	const RasterBuilder::Raster &old_raster = raster;

	RasterBuilder::Raster new_raster;
	new_raster.input.source_georeferencing = source_georeferencing;
	new_raster.input.source_raster = source_raster;
	new_raster.input.source_raster_colour_scheme = source_raster_colour_scheme;
	new_raster.input.is_default_raster_colour_scheme = false;
	new_raster.input.reconstruct_raster_polygons = reconstruct_raster_polygons;
	new_raster.input.age_grid_georeferencing = age_grid_georeferencing;
	new_raster.input.age_grid_raster = age_grid_raster;

	//
	// First see if we need to create or update the source raster.
	//

	// Use the default colour scheme, for the raster, if the user hasn't set one.
	// But keep the default colour scheme from the last frame if the raster hasn't changed.
	// This means the multi-resolution rasters won't have to invalidate their texture caches
	// every time they're rendered because they think the raster colour scheme has changed.
	// Instead they'll see the same default colour scheme each frame.
	if (!new_raster.input.source_raster_colour_scheme)
	{
		// The default colour scheme depends on the raster (on its statistics) so it
		// needs to be changed when the raster changes.
		if (new_raster.input.source_raster != old_raster.input.source_raster ||
			// If the previous colour scheme was not a default colour scheme then we need to
			// create a default colour scheme regardless of whether the raster changed or not...
			!old_raster.input.is_default_raster_colour_scheme)
		{
			//qDebug() << "Creating default raster colour scheme.";
			new_raster.input.source_raster_colour_scheme =
					create_default_raster_colour_scheme(new_raster.input.source_raster.get());
			new_raster.input.is_default_raster_colour_scheme = true;
		}
		else
		{
			new_raster.input.source_raster_colour_scheme = old_raster.input.source_raster_colour_scheme;
			new_raster.input.is_default_raster_colour_scheme = old_raster.input.is_default_raster_colour_scheme;
		}
	}

	// If an old source raster does not exist, or  if the georeferencing has changed then
	// we need to build the raster from scratch.
	if (!old_raster.output.source_multi_resolution_raster ||
		new_raster.input.source_georeferencing != old_raster.input.source_georeferencing)
	{
		//qDebug() << "Georeferencing changed - creating new multi-resolution raster.";
		if (!create_source_multi_resolution_raster(new_raster))
		{
			raster = new_raster; // Write back any changes made during update.
			// Return early - everything depends on the source raster.
			return boost::none;
		}
	}
	// If the raster data and colour scheme have not changed then the raster is fine as it is.
	else if (new_raster.input.source_raster == old_raster.input.source_raster &&
		new_raster.input.source_raster_colour_scheme == old_raster.input.source_raster_colour_scheme)
	{
		new_raster.output.source_proxied_raster = old_raster.output.source_proxied_raster;
		new_raster.output.source_multi_resolution_raster = old_raster.output.source_multi_resolution_raster;
	}
	// Otherwise we can keep the existing raster but change the raster data and/or colour scheme.
	else
	{
		new_raster.output.source_proxied_raster = old_raster.output.source_proxied_raster;
		new_raster.output.source_multi_resolution_raster = old_raster.output.source_multi_resolution_raster;

		//qDebug() << "Raster colour scheme or raster changed - creating new multi-resolution raster.";
		// If we weren't able to change the raster then we'll need to rebuild it from scratch.
		// This should succeed if the raster dimensions haven't changed (note also that we're
		// only here because the georeferencing hasn't changed either).
		if (!new_raster.output.source_proxied_raster.get()->change_raster(
				new_raster.input.source_raster.get(), new_raster.input.source_raster_colour_scheme))
		{
			if (!create_source_multi_resolution_raster(new_raster))
			{
				raster = new_raster; // Write back any changes made during update.
				// Return early - everything depends on the source raster.
				return boost::none;
			}
		}
	}

	//
	// We only need multi-resolution *cube* rasters if we are reconstructing the rasters
	// which can only happen if we have reconstructing polygons.
	//

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

	//
	// Next see if we need to create or update the source multi-resolution cube raster.
	//

	// If a new source multi-resolution raster was created then we'll need to create a new
	// multi-resolution cube raster to attach to it.
	// We'll also need to create a new one if there was no old one.
	if (!old_raster.output.source_multi_resolution_cube_raster ||
		new_raster.output.source_multi_resolution_raster != old_raster.output.source_multi_resolution_raster)
	{
		new_raster.output.source_multi_resolution_cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						new_raster.output.source_multi_resolution_raster.get(),
						GPlatesOpenGL::GLMultiResolutionReconstructedRaster::get_cube_subdivision(),
						d_opengl_shared_state->get_texture_resource_manager());
	}
	else
	{
		// Otherwise we can just keep the previous source multi-resolution cube raster.
		new_raster.output.source_multi_resolution_cube_raster =
				old_raster.output.source_multi_resolution_cube_raster;
	}

	//
	// Next see if we need to create the age grid mask and coverage multi-resolution rasters.
	//

	// If we have an age grid raster (and georeferencing) then we need to build
	// age grid mask and coverage multi-resolution *cube* rasters for input to the
	// multi-resolution *reconstructing* raster.
	if (new_raster.input.age_grid_raster && new_raster.input.age_grid_georeferencing)
	{
		// If we don't have an age grid mask multi-resolution cube raster or
		// the input age grid raster has changed then create a new one.
		if (!old_raster.output.age_grid_mask_multi_resolution_cube_raster ||
			new_raster.input.age_grid_raster != old_raster.input.age_grid_raster)
		{
			new_raster.output.age_grid_mask_multi_resolution_source =
					GPlatesOpenGL::GLAgeGridMaskSource::create(
							reconstruction_time,
							new_raster.input.age_grid_raster.get(),
							d_opengl_shared_state->get_texture_resource_manager());
			if (new_raster.output.age_grid_mask_multi_resolution_source)
			{
				// Create an age grid mask multi-resolution raster.
				new_raster.output.age_grid_mask_multi_resolution_raster =
						GPlatesOpenGL::GLMultiResolutionRaster::create(
								new_raster.input.age_grid_georeferencing.get(),
								new_raster.output.age_grid_mask_multi_resolution_source.get(),
								d_opengl_shared_state->get_texture_resource_manager());

				// Create an age grid mask multi-resolution cube raster.
				new_raster.output.age_grid_mask_multi_resolution_cube_raster =
						GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
								new_raster.output.age_grid_mask_multi_resolution_raster.get(),
								GPlatesOpenGL::GLMultiResolutionReconstructedRaster::get_cube_subdivision(),
								d_opengl_shared_state->get_texture_resource_manager());
			}
			else
			{
				qWarning() << "Unable to process age grid raster - ignoring it.";
			}
		}
		else
		{
			// Otherwise we can just keep the previous age grid multi-resolution cube raster
			// and associated rasters.
			new_raster.output.age_grid_mask_multi_resolution_cube_raster =
					old_raster.output.age_grid_mask_multi_resolution_cube_raster;
			new_raster.output.age_grid_mask_multi_resolution_raster =
					old_raster.output.age_grid_mask_multi_resolution_raster;
			new_raster.output.age_grid_mask_multi_resolution_source =
					old_raster.output.age_grid_mask_multi_resolution_source;

			// Let the age grid mask know of the current reconstruction time as the mask
			// changes dynamically with the reconstruction time.
			new_raster.output.age_grid_mask_multi_resolution_source.get()
					->update_reconstruction_time(reconstruction_time);
		}

		// If we don't have an age grid coverage multi-resolution cube raster or
		// the input age grid raster has changed then create a new one.
		if (!old_raster.output.age_grid_coverage_multi_resolution_cube_raster ||
			new_raster.input.age_grid_raster != old_raster.input.age_grid_raster)
		{
			new_raster.output.age_grid_coverage_multi_resolution_source =
					GPlatesOpenGL::GLAgeGridCoverageSource::create(
							new_raster.input.age_grid_raster.get());
			if (new_raster.output.age_grid_coverage_multi_resolution_source)
			{
				// Create an age grid coverage multi-resolution raster.
				new_raster.output.age_grid_coverage_multi_resolution_raster =
						GPlatesOpenGL::GLMultiResolutionRaster::create(
								new_raster.input.age_grid_georeferencing.get(),
								new_raster.output.age_grid_coverage_multi_resolution_source.get(),
								d_opengl_shared_state->get_texture_resource_manager());

				// Create an age grid coverage multi-resolution cube raster.
				new_raster.output.age_grid_coverage_multi_resolution_cube_raster =
						GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
								new_raster.output.age_grid_coverage_multi_resolution_raster.get(),
								GPlatesOpenGL::GLMultiResolutionReconstructedRaster::get_cube_subdivision(),
								d_opengl_shared_state->get_texture_resource_manager());
			}
			else
			{
				qWarning() << "Unable to process age grid raster - ignoring it.";
			}
		}
		else
		{
			// Otherwise we can just keep the previous age grid multi-resolution cube raster.
			new_raster.output.age_grid_coverage_multi_resolution_cube_raster =
					old_raster.output.age_grid_coverage_multi_resolution_cube_raster;
			new_raster.output.age_grid_coverage_multi_resolution_raster =
					old_raster.output.age_grid_coverage_multi_resolution_raster;
			new_raster.output.age_grid_coverage_multi_resolution_source =
					old_raster.output.age_grid_coverage_multi_resolution_source;
		}
	}

	//
	// Next see if we need to create a new multi-resolution *reconstructed* raster.
	//

	// If the input reconstructing polygons have changed, or any of the multi-resolution *cube*
	// inputs to a multi-resolution *reconstructing* raster have changed, then create a new one.
	if (new_raster.input.reconstruct_raster_polygons !=
				old_raster.input.reconstruct_raster_polygons ||
		new_raster.output.source_multi_resolution_cube_raster !=
				old_raster.output.source_multi_resolution_cube_raster ||
		new_raster.output.age_grid_mask_multi_resolution_cube_raster !=
				old_raster.output.age_grid_mask_multi_resolution_cube_raster ||
		new_raster.output.age_grid_coverage_multi_resolution_cube_raster !=
				old_raster.output.age_grid_coverage_multi_resolution_cube_raster)
	{
		// TODO: Implement an efficient update of the reconstructed raster when the
		// raster polygons have not changed.
		new_raster.output.source_multi_resolution_reconstructed_raster =
				GPlatesOpenGL::GLMultiResolutionReconstructedRaster::create(
						new_raster.output.source_multi_resolution_cube_raster.get(),
						new_raster.input.reconstruct_raster_polygons.get(),
						d_opengl_shared_state->get_texture_resource_manager(),
						new_raster.output.age_grid_mask_multi_resolution_cube_raster,
						new_raster.output.age_grid_coverage_multi_resolution_cube_raster);
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
GPlatesGui::PersistentOpenGLObjects::ListObjects::create_source_multi_resolution_raster(
		RasterBuilder::Raster &new_raster)
{
	new_raster.output.source_proxied_raster = GPlatesOpenGL::GLProxiedRasterSource::create(
			new_raster.input.source_raster.get(),
			new_raster.input.source_raster_colour_scheme);
	if (!new_raster.output.source_proxied_raster)
	{
		return false;
	}

	new_raster.output.source_multi_resolution_raster =
			GPlatesOpenGL::GLMultiResolutionRaster::create(
					new_raster.input.source_georeferencing.get(),
					new_raster.output.source_proxied_raster.get(),
					d_opengl_shared_state->get_texture_resource_manager());

	return true;
}


void
GPlatesGui::PersistentOpenGLObjects::ListObjects::release_layer(
		const GPlatesAppLogic::Layer &layer)
{
	// If there's a raster in the layer about to be removed then release
	// the memory used by it.
	RasterBuilder::layer_to_raster_map_type::iterator raster_iter =
			d_raster_builder.layer_to_raster_map.find(layer);
	if (raster_iter != d_raster_builder.layer_to_raster_map.end())
	{
		d_raster_builder.layer_to_raster_map.erase(raster_iter);
	}
}
