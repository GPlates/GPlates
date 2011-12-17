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

#include <boost/foreach.hpp>
#include <QDebug>

#include "PersistentOpenGLObjects.h"

#include "RasterColourPalette.h"

#include "app-logic/ApplicationState.h"

#include "opengl/GLCubeSubdivisionCache.h"
#include "opengl/GLMultiResolutionCubeReconstructedRaster.h"
#include "opengl/GLRenderer.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"


GPlatesGui::PersistentOpenGLObjects::PersistentOpenGLObjects(
		const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
		GPlatesAppLogic::ApplicationState &application_state) :
	d_non_list_objects(new NonListObjects()),
	d_list_objects(
			new ListObjects(
					opengl_context->get_shared_state(),
					*d_non_list_objects))
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
		objects_from_another_context->d_list_objects->opengl_shared_state)
	{
		d_list_objects = objects_from_another_context->d_list_objects;
	}
	else
	{
		d_list_objects.reset(
				new ListObjects(
						opengl_context->get_shared_state(),
						*d_non_list_objects));
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


GPlatesGui::PersistentOpenGLObjects::cache_handle_type
GPlatesGui::PersistentOpenGLObjects::render_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type &resolved_raster,
		const RasterColourPalette::non_null_ptr_to_const_type &source_raster_colour_palette,
		const Colour &source_raster_modulate_colour,
		boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection)
{
	// Get the GL layer corresponding to the layer the raster came from.
	GLLayer &gl_raster_layer = d_list_objects->gl_layers.get_layer(
			resolved_raster->get_raster_layer_proxy());

	// The reconstructed static polygon meshes layer usage comes from another layer.
	boost::optional<GPlatesUtils::non_null_intrusive_ptr<ReconstructedStaticPolygonMeshesLayerUsage> >
			reconstructed_polygon_meshes_layer_usage;
	if (resolved_raster->get_reconstructed_polygons_layer_proxy())
	{
		// Get the GL layer corresponding to the layer the reconstructed polygons came from.
		GLLayer &gl_reconstructed_polygons_layer = d_list_objects->gl_layers.get_layer(
				resolved_raster->get_reconstructed_polygons_layer_proxy().get());

		reconstructed_polygon_meshes_layer_usage =
				gl_reconstructed_polygons_layer.get_reconstructed_static_polygon_meshes_layer_usage();

		// Set the current reconstruction time.
		reconstructed_polygon_meshes_layer_usage.get()->set_reconstruction_time(
				renderer,
				resolved_raster->get_reconstruction_time());
	}

	// The age grid layer usage comes from another layer.
	boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > age_grid_layer_usage;
	if (resolved_raster->get_age_grid_layer_proxy())
	{
		// Get the GL layer corresponding to the layer the age grid came from.
		GLLayer &gl_age_grid_layer = d_list_objects->gl_layers.get_layer(
				resolved_raster->get_age_grid_layer_proxy().get());

		age_grid_layer_usage = gl_age_grid_layer.get_age_grid_layer_usage();

		// Set the current reconstruction time.
		age_grid_layer_usage.get()->set_reconstruction_time(
				renderer,
				resolved_raster->get_reconstruction_time());
	}

	// Get the raster layer usage so we can set the colour palette.
	const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> raster_layer_usage =
			gl_raster_layer.get_raster_layer_usage();

	// Set the colour palette.
	raster_layer_usage->set_raster_colour_palette(renderer, source_raster_colour_palette);
	// Set the modulate colour.
	raster_layer_usage->set_raster_modulate_colour(renderer, source_raster_modulate_colour);

	// Get the static polygon reconstructed raster layer usage so we can update its input layer usages.
	const GPlatesUtils::non_null_intrusive_ptr<StaticPolygonReconstructedRasterLayerUsage>
			static_polygon_reconstructed_raster_layer_usage =
					gl_raster_layer.get_static_polygon_reconstructed_raster_layer_usage();

	// Set/udate the layer usage inputs.
	static_polygon_reconstructed_raster_layer_usage->set_layer_inputs(
			reconstructed_polygon_meshes_layer_usage,
			age_grid_layer_usage);

	// Get the map raster layer usage.
	const GPlatesUtils::non_null_intrusive_ptr<MapRasterLayerUsage> map_raster_layer_usage =
			gl_raster_layer.get_map_raster_layer_usage();

	//
	// Now that we've finished updating everything we can get onto rendering...
	//


	// Render a 2D map view raster if we have a map projection.
	if (map_projection)
	{
		// Get the raster map view.
		boost::optional<GPlatesOpenGL::GLMultiResolutionRasterMapView::non_null_ptr_type> multi_resolution_raster_map_view =
				map_raster_layer_usage->get_multi_resolution_raster_map_view(
						renderer,
						// The global map cube mesh shared by all layers...
						d_list_objects->get_multi_resolution_map_cube_mesh(renderer, *map_projection.get()));

		cache_handle_type cache_handle;

		//  Render the map view of raster if successful.
		if (multi_resolution_raster_map_view)
		{
			multi_resolution_raster_map_view.get()->render(renderer, cache_handle);
		}

		return cache_handle;
	}

	// Next try to render a reconstructed raster in 3D globe view.
	// Only attempt if there is a reconstructed polygons layer.
	if (reconstructed_polygon_meshes_layer_usage)
	{
		boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
				static_polygon_reconstructed_raster =
						static_polygon_reconstructed_raster_layer_usage->get_static_polygon_reconstructed_raster(
								renderer);
		if (static_polygon_reconstructed_raster)
		{
			//
			// We are rendering a *reconstructed* raster.
			//
			cache_handle_type cache_handle;
			static_polygon_reconstructed_raster.get()->render(renderer, cache_handle);

			return cache_handle;
		}
		// else drop through and render the *unreconstructed* raster...
	}

	// We have a regular, unreconstructed raster - although it can still be a time-dependent raster.
	// Get the multi-resolution raster.
	boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> multi_resolution_raster =
			raster_layer_usage->get_multi_resolution_raster(renderer);

	cache_handle_type cache_handle;

	// Render the multi-resolution raster, if we have one, in 3D globe view.
	if (multi_resolution_raster)
	{
		multi_resolution_raster.get()->render(renderer, cache_handle);
	}

	return cache_handle;
}


void
GPlatesGui::PersistentOpenGLObjects::render_filled_polygons(
		GPlatesOpenGL::GLRenderer &renderer,
		const filled_polygons_type &filled_polygons)
{
	d_list_objects->get_multi_resolution_filled_polygons(renderer)->render(renderer, filled_polygons);
}


void
GPlatesGui::PersistentOpenGLObjects::handle_layer_about_to_be_removed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
	GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type layer_proxy_handle = layer.get_layer_proxy_handle();

	d_list_objects->gl_layers.remove_layer(layer_proxy_handle);
}


GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::RasterLayerUsage(
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &raster_layer_proxy) :
	d_raster_layer_proxy(raster_layer_proxy),
	d_raster_colour_palette_dirty(true),
	d_raster_modulate_colour(Colour::get_white()),
	d_raster_modulate_colour_dirty(true)
{
}


void
GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::set_raster_colour_palette(
		GPlatesOpenGL::GLRenderer &renderer,
		const RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette)
{
	if (raster_colour_palette == d_raster_colour_palette)
	{
		// Nothing has changed so just return.
		return;
	}

	d_raster_colour_palette = raster_colour_palette;

	// The visual raster source will need to update itself with the new raster colour palette.
	d_raster_colour_palette_dirty = true;
}


void
GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::set_raster_modulate_colour(
		GPlatesOpenGL::GLRenderer &renderer,
		const Colour &raster_modulate_colour)
{
	if (raster_modulate_colour == d_raster_modulate_colour)
	{
		// Nothing has changed so just return.
		return;
	}

	d_raster_modulate_colour = raster_modulate_colour;

	// The visual raster source will need to update itself with the new raster modulate colour.
	d_raster_modulate_colour_dirty = true;
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::get_multi_resolution_raster(
		GPlatesOpenGL::GLRenderer &renderer)
{
	if (!d_raster_layer_proxy->get_proxied_raster() || !d_raster_colour_palette)
	{
		d_visual_raster_source = boost::none;

		// There's no proxied raster or colour palette so nothing we can do.
		return boost::none;
	}

	if (!d_raster_layer_proxy->get_georeferencing())
	{
		d_multi_resolution_raster = boost::none;

		// There's no georeferencing so nothing we can do.
		return boost::none;
	}

	// If we're not up-to-date with respect to the proxied raster in the raster layer proxy...
	// This can happen for time-dependent rasters when the time changes.
	if (d_raster_colour_palette_dirty ||
		!d_raster_layer_proxy->get_proxied_raster_subject_token().is_observer_up_to_date(
				d_proxied_raster_observer_token))
	{
		// If we have a visual raster source then attempt to change the raster first
		// since it's cheaper than rebuilding the multi-resolution raster.
		if (d_visual_raster_source)
		{
			if (d_visual_raster_source.get()->change_raster(
					renderer,
					d_raster_layer_proxy->get_proxied_raster().get(),
					d_raster_colour_palette.get()))
			{
				d_raster_colour_palette_dirty = false;
			}
			else
			{
				// Change raster was unsuccessful, so rebuild visual raster source.
				d_visual_raster_source = boost::none;
			}
		}

		// We have taken measures to be up-to-date with respect to the proxied raster in the raster layer proxy.
		d_raster_layer_proxy->get_proxied_raster_subject_token().update_observer(
				d_proxied_raster_observer_token);
	}

	// Rebuild the visual raster source if necessary.
	if (!d_visual_raster_source)
	{
		// NOTE: We also invalidate the multi-resolution raster since it must link
		// to the visual raster source and hence must also be rebuilt.
		d_multi_resolution_raster = boost::none;

		d_visual_raster_source = GPlatesOpenGL::GLVisualRasterSource::create(
				renderer,
				d_raster_layer_proxy->get_proxied_raster().get(),
				d_raster_colour_palette.get(),
				d_raster_modulate_colour);
		if (d_visual_raster_source)
		{
			d_raster_colour_palette_dirty = false;
			d_raster_modulate_colour_dirty = false;
		}
		else
		{
			// Unable to create a source proxy raster so nothing we can do.
			return boost::none;
		}
	}

	// Update the modulate colour if it's still dirty.
	if (d_raster_modulate_colour_dirty)
	{
		d_visual_raster_source.get()->change_modulate_colour(renderer, d_raster_modulate_colour);
		d_raster_modulate_colour_dirty = false;
	}

	// If we're not up-to-date with respect to the raster feature in the raster layer proxy then rebuild.
	if (!d_raster_layer_proxy->get_raster_feature_subject_token().is_observer_up_to_date(
			d_raster_feature_observer_token))
	{
		d_multi_resolution_raster = boost::none;

		// We have taken measures to be up-to-date with respect to the raster feature in the raster layer proxy.
		d_raster_layer_proxy->get_raster_feature_subject_token().update_observer(
				d_raster_feature_observer_token);
	}

	// Rebuild the multi-resolution raster if necessary.
	if (!d_multi_resolution_raster)
	{
		const GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type multi_resolution_raster =
				GPlatesOpenGL::GLMultiResolutionRaster::create(
						renderer,
						d_raster_layer_proxy->get_georeferencing().get(),
						d_visual_raster_source.get());

		d_multi_resolution_raster = multi_resolution_raster;
	}

	return d_multi_resolution_raster.get();
}


bool
GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return layer_proxy_handle == d_raster_layer_proxy;
}


GPlatesGui::PersistentOpenGLObjects::CubeRasterLayerUsage::CubeRasterLayerUsage(
		const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> &raster_layer_usage) :
	d_raster_layer_usage(raster_layer_usage)
{
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::CubeRasterLayerUsage::get_multi_resolution_cube_raster(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// Get the source multi-resolution raster.
	const boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> multi_resolution_raster =
			d_raster_layer_usage->get_multi_resolution_raster(renderer);

	// If source multi-resolution raster is a different object...
	if (d_multi_resolution_raster != multi_resolution_raster)
	{
		d_multi_resolution_raster = multi_resolution_raster;

		// We need to rebuild the multi-resolution cube raster.
		d_multi_resolution_cube_raster = boost::none;
	}

	if (!d_multi_resolution_cube_raster)
	{
		if (!d_multi_resolution_raster)
		{
			// There's no multi-resolution raster so nothing we can do.
			return boost::none;
		}

		// Attempt to create the multi-resolution cube raster.
		d_multi_resolution_cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						renderer,
						d_multi_resolution_raster.get(),
						GPlatesOpenGL::GLMultiResolutionCubeRaster::DEFAULT_TILE_TEXEL_DIMENSION,
						true/*adapt_tile_dimension_to_source_resolution*/,
						GPlatesOpenGL::GLMultiResolutionCubeRaster::DEFAULT_FIXED_POINT_TEXTURE_FILTER,
						// Use non-default here - our source GLMultiResolutionRaster in turn references
						// a GLVisualRasterSource which has caching that insulates us from the
						// file system so we don't really need any caching in GLMultiResolutionRaster
						// and can save a lot of texture memory usage as a result...
						false/*cache_source_tile_textures*/);
	}

	return d_multi_resolution_cube_raster;
}


bool
GPlatesGui::PersistentOpenGLObjects::CubeRasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return d_raster_layer_usage->is_required_direct_or_indirect_dependency(layer_proxy_handle);
}


GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::AgeGridLayerUsage(
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &age_grid_raster_layer_proxy) :
	d_age_grid_raster_layer_proxy(age_grid_raster_layer_proxy),
	d_reconstruction_time(0),
	d_reconstruction_time_dirty(true)
{
}


void
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::set_reconstruction_time(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time)
{
	if (d_reconstruction_time == reconstruction_time)
	{
		// The time has not changed.
		return;
	}

	d_reconstruction_time = reconstruction_time;

	// Need to update for the new reconstruction time.
	d_reconstruction_time_dirty = true;
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::get_age_grid_mask_multi_resolution_raster(
		GPlatesOpenGL::GLRenderer &renderer)
{
	check_input_raster();

	if (!d_age_grid_mask_multi_resolution_source)
	{
		// NOTE: We must invalidate the multi-resolution raster since it must link
		// to the age grid mask source and hence must also be rebuilt.
		d_age_grid_mask_multi_resolution_raster = boost::none;

		// Get the present-day age grid raster.
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> age_grid_raster =
				d_age_grid_raster_layer_proxy->get_proxied_raster(0/*present-day*/);
		if (!age_grid_raster)
		{
			// Unable to get proxied raster so nothing we can do.
			return boost::none;
		}

		d_age_grid_mask_multi_resolution_source =
				GPlatesOpenGL::GLAgeGridMaskSource::create(
						renderer,
						d_reconstruction_time.dval(),
						age_grid_raster.get());
		if (!d_age_grid_mask_multi_resolution_source)
		{
			// Unable to get age grid mask source so nothing we can do.
			return boost::none;
		}

		d_reconstruction_time_dirty = false;
	}

	// Rebuild the age grid *mask* raster if necessary.
	if (!d_age_grid_mask_multi_resolution_raster)
	{
		// Create an age grid mask multi-resolution raster.
		d_age_grid_mask_multi_resolution_raster =
				GPlatesOpenGL::GLMultiResolutionRaster::create(
						renderer,
						d_age_grid_raster_layer_proxy->get_georeferencing().get(),
						d_age_grid_mask_multi_resolution_source.get(),
						// Avoids blending seams due to anisotropic filtering which gives age grid
						// mask alpha values that are not either 0.0 or 1.0...
						GPlatesOpenGL::GLMultiResolutionRaster::FIXED_POINT_TEXTURE_FILTER_NO_ANISOTROPIC);
	}

	update();

	return d_age_grid_mask_multi_resolution_raster;
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::get_age_grid_coverage_multi_resolution_raster(
		GPlatesOpenGL::GLRenderer &renderer)
{
	check_input_raster();

	if (!d_age_grid_coverage_multi_resolution_source)
	{
		// Get the present-day age grid raster.
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> age_grid_raster =
				d_age_grid_raster_layer_proxy->get_proxied_raster(0/*present-day*/);
		if (!age_grid_raster)
		{
			// Unable to get proxied raster so nothing we can do.
			return boost::none;
		}

		d_age_grid_coverage_multi_resolution_source =
				GPlatesOpenGL::GLCoverageSource::create(
						age_grid_raster.get(),
						// The age grid coverage is designed for use with
						// class 'GLMultiResolutionStaticPolygonReconstructedRaster' so we
						// distribute the coverage across pixel channels according to what it wants...
						GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::AgeGridRaster::COVERAGE_TEXTURE_DATA_FORMAT);
		if (!d_age_grid_coverage_multi_resolution_source)
		{
			// Unable to get age grid coverage source so nothing we can do.
			return boost::none;
		}
	}

	// Rebuild the age grid *coverage* cube raster if necessary.
	if (!d_age_grid_coverage_multi_resolution_raster)
	{

		// Create an age grid coverage multi-resolution raster.
		GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type age_grid_coverage_multi_resolution_raster =
				GPlatesOpenGL::GLMultiResolutionRaster::create(
						renderer,
						d_age_grid_raster_layer_proxy->get_georeferencing().get(),
						d_age_grid_coverage_multi_resolution_source.get(),
						// Avoids blending seams due to anisotropic filtering which gives age grid
						// coverage alpha values that are not either 0.0 or 1.0...
						GPlatesOpenGL::GLMultiResolutionRaster::FIXED_POINT_TEXTURE_FILTER_NO_ANISOTROPIC);

		d_age_grid_coverage_multi_resolution_raster = age_grid_coverage_multi_resolution_raster;
	}

	update();

	return d_age_grid_coverage_multi_resolution_raster;
}


bool
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return layer_proxy_handle == d_age_grid_raster_layer_proxy;
}


void
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::check_input_raster()
{
	if (!d_age_grid_raster_layer_proxy->get_georeferencing())
	{
		// Rebuild everything that depends on georeferencing directly or indirectly.
		d_age_grid_mask_multi_resolution_raster = boost::none;
		d_age_grid_coverage_multi_resolution_raster = boost::none;

		// There's no georeferencing so nothing we can do.
		return;
	}

	// If we're not up-to-date with respect to the age grid feature in the age grid layer proxy...
	if (!d_age_grid_raster_layer_proxy->get_raster_feature_subject_token().is_observer_up_to_date(
			d_age_grid_raster_feature_observer_token))
	{
		// Rebuild everything.
		d_age_grid_mask_multi_resolution_source = boost::none;
		d_age_grid_mask_multi_resolution_raster = boost::none;
		d_age_grid_coverage_multi_resolution_source = boost::none;
		d_age_grid_coverage_multi_resolution_raster = boost::none;

		// We have taken measures to be up-to-date with respect to the age grid feature in the age grid layer proxy.
		d_age_grid_raster_layer_proxy->get_raster_feature_subject_token().update_observer(
				d_age_grid_raster_feature_observer_token);
	}

	//
	// NOTE: Since we use the age grid only at *present-day* we don't need to update ourself
	// if the age grid raster layer has changed proxied rasters (ie, if, for some reason,
	// the age grid raster is time-dependent).
	//
}


void
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::update()
{
	if (d_reconstruction_time_dirty)
	{
		if (d_age_grid_mask_multi_resolution_source)
		{
			// Update the reconstruction time for the age grid mask.
			d_age_grid_mask_multi_resolution_source.get()->update_reconstruction_time(d_reconstruction_time.dval());

			d_reconstruction_time_dirty = false;
		}
	}
}


GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::ReconstructedStaticPolygonMeshesLayerUsage(
		const GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type &reconstructed_static_polygon_meshes_layer_proxy) :
	d_reconstructed_static_polygon_meshes_layer_proxy(reconstructed_static_polygon_meshes_layer_proxy),
	d_reconstruction_time(0),
	d_reconstruction_time_dirty(true)
{
}


void
GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::set_reconstruction_time(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time)
{
	if (d_reconstruction_time == reconstruction_time)
	{
		// The time has not changed.
		return;
	}

	d_reconstruction_time = reconstruction_time;

	// Need to update for the new reconstruction time.
	d_reconstruction_time_dirty = true;
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type
GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::get_reconstructed_static_polygon_meshes(
		GPlatesOpenGL::GLRenderer &renderer,
		bool reconstructing_with_age_grid)
{
	// If we're not up-to-date with respect to the present day polygons in the reconstruct layer proxy...
	if (!d_reconstructed_static_polygon_meshes_layer_proxy->get_reconstructable_feature_collections_subject_token()
		.is_observer_up_to_date(d_present_day_polygons_observer_token))
	{
		// The present day polygons have changed so need to rebuild the meshes.
		d_reconstructed_static_polygon_meshes = boost::none;

		// We have taken measures to be up-to-date with respect to the present day polygons in the reconstruct layer proxy.
		d_reconstructed_static_polygon_meshes_layer_proxy->get_reconstructable_feature_collections_subject_token()
			.update_observer(d_present_day_polygons_observer_token);
	}

	bool need_to_update = false;

	// Rebuild the GLReconstructedStaticPolygonMeshes object if necessary.
	if (!d_reconstructed_static_polygon_meshes)
	{
		d_reconstructed_static_polygon_meshes =
				GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::create(
						renderer,
						d_reconstructed_static_polygon_meshes_layer_proxy->get_present_day_polygon_meshes(),
						d_reconstructed_static_polygon_meshes_layer_proxy->get_present_day_geometries(),
						d_reconstructed_static_polygon_meshes_layer_proxy->get_reconstructions_spatial_partition(
								d_reconstruction_time.dval()));

		d_reconstruction_time_dirty = false;

		// Even though we just created the 'GLReconstructedStaticPolygonMeshes' we still need to
		// update it in case we're using age grids which need a reconstructions spatial partition
		// that ignores the active time periods of features.
		need_to_update = true;

		// We have taken measures to be up-to-date with respect to the reconstructed polygons in the reconstruct layer proxy.
		d_reconstructed_static_polygon_meshes_layer_proxy->get_subject_token().update_observer(
				d_reconstructed_polygons_observer_token);
	}

	// If the reconstruction time is dirty...
	if (d_reconstruction_time_dirty)
	{
		need_to_update = true;

		d_reconstruction_time_dirty = false;
	}

	// If we're changing decision to reconstruction with an age grid...
	if (d_reconstructing_with_age_grid != reconstructing_with_age_grid)
	{
		need_to_update = true;

		d_reconstructing_with_age_grid = reconstructing_with_age_grid;
	}

	// If we're not up-to-date with respect to the reconstructed polygons in the reconstruct layer proxy...
	if (!d_reconstructed_static_polygon_meshes_layer_proxy->get_subject_token().is_observer_up_to_date(
			d_reconstructed_polygons_observer_token))
	{
		need_to_update = true;

		// We have taken measures to be up-to-date with respect to the reconstructed polygons in the reconstruct layer proxy.
		d_reconstructed_static_polygon_meshes_layer_proxy->get_subject_token().update_observer(
				d_reconstructed_polygons_observer_token);
	}

	if (need_to_update)
	{
		update();
	}

	return d_reconstructed_static_polygon_meshes.get();
}


void
GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::update()
{
	//
	// Update
	//

	// The reconstructions spatial partition for *active* features.
	const GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type
			reconstructions_spatial_partition = d_reconstructed_static_polygon_meshes_layer_proxy
					->get_reconstructions_spatial_partition(d_reconstruction_time.dval());

	// The reconstructions spatial partition for *active* or *inactive* features.
	boost::optional<GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type>
					active_or_inactive_reconstructions_spatial_partition;
	// It's only needed if we've been asked to help reconstruct a raster with the aid of an age grid.
	if (d_reconstructing_with_age_grid)
	{
		// Use the same reconstruct params but specify that reconstructions should include *inactive* features also.
		GPlatesAppLogic::ReconstructParams reconstruct_params =
				d_reconstructed_static_polygon_meshes_layer_proxy->get_current_reconstruct_params();
		reconstruct_params.set_reconstruct_by_plate_id_outside_active_time_period(true);

		// Get a new reconstructions spatial partition that includes *inactive* reconstructions.
		active_or_inactive_reconstructions_spatial_partition = d_reconstructed_static_polygon_meshes_layer_proxy
					->get_reconstructions_spatial_partition(reconstruct_params, d_reconstruction_time.dval());
	}

	d_reconstructed_static_polygon_meshes.get()->update(
			reconstructions_spatial_partition,
			active_or_inactive_reconstructions_spatial_partition);
}


bool
GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return layer_proxy_handle == d_reconstructed_static_polygon_meshes_layer_proxy;
}


GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::StaticPolygonReconstructedRasterLayerUsage(
		const GPlatesUtils::non_null_intrusive_ptr<CubeRasterLayerUsage> &cube_raster_layer_usage) :
	d_cube_raster_layer_usage(cube_raster_layer_usage)
{
}


void
GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::set_layer_inputs(
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<ReconstructedStaticPolygonMeshesLayerUsage> > &
				reconstructed_polygon_meshes_layer_usage,
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > &age_grid_layer_usage)
{
	// See if we've switched layer usages.
	if (d_reconstructed_polygon_meshes_layer_usage != reconstructed_polygon_meshes_layer_usage)
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		d_reconstructed_polygon_meshes = boost::none;
		d_reconstructed_polygon_meshes_layer_usage = reconstructed_polygon_meshes_layer_usage;
	}
	if (d_age_grid_layer_usage != age_grid_layer_usage)
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		d_age_grid_mask_raster = boost::none;
		d_age_grid_coverage_raster = boost::none;
		d_age_grid_layer_usage = age_grid_layer_usage;
	}
}


boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::get_static_polygon_reconstructed_raster(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// If *reconstructed* rasters are not supported then return invalid.
	if (!GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::is_supported(renderer))
	{
		return boost::none;
	}

	// If we don't have reconstructed polygon meshes then we cannot reconstruct.
	if (!d_reconstructed_polygon_meshes_layer_usage)
	{
		return boost::none;
	}

	// Get the source multi-resolution cube raster.
	const boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> multi_resolution_cube_raster =
			d_cube_raster_layer_usage->get_multi_resolution_cube_raster(renderer);

	// If source multi-resolution cube raster is a different object...
	if (d_multi_resolution_cube_raster != multi_resolution_cube_raster)
	{
		d_multi_resolution_cube_raster = multi_resolution_cube_raster;

		// We need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;
	}

	// Get the reconstructed polygon meshes.
	const boost::optional<GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type> reconstructed_polygon_meshes =
			d_reconstructed_polygon_meshes_layer_usage.get()->get_reconstructed_static_polygon_meshes(
					renderer,
					d_age_grid_layer_usage/*reconstructing_with_age_grid*/);

	// If reconstructed polygon meshes is a different object...
	if (d_reconstructed_polygon_meshes != reconstructed_polygon_meshes)
	{
		d_reconstructed_polygon_meshes = reconstructed_polygon_meshes;

		// We need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;
	}

	// If we are using an age grid to assist reconstruction...
	if (d_age_grid_layer_usage)
	{
		// Get the age grid mask raster.
		boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> age_grid_mask_raster =
				d_age_grid_layer_usage.get()->get_age_grid_mask_multi_resolution_raster(renderer);

		// Get the age grid coverage cube raster.
		boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> age_grid_coverage_raster =
				d_age_grid_layer_usage.get()->get_age_grid_coverage_multi_resolution_raster(renderer);

		// If the age grid rasters are different objects...
		if (d_age_grid_mask_raster != age_grid_mask_raster ||
			d_age_grid_coverage_raster != age_grid_coverage_raster)
		{
			d_age_grid_mask_raster = age_grid_mask_raster;
			d_age_grid_coverage_raster = age_grid_coverage_raster;

			// We need to rebuild the reconstructed raster.
			d_reconstructed_raster = boost::none;
		}
	}

	if (!d_reconstructed_raster)
	{
		if (!d_multi_resolution_cube_raster)
		{
			// There's no multi-resolution cube raster so nothing we can do.
			return boost::none;
		}

		if (!d_reconstructed_polygon_meshes)
		{
			// There's no reconstructed polygon meshes so nothing we can do.
			return boost::none;
		}

		// We can only reconstruct with an age grid if we have both rasters.
		// If one is successfully created then typically the other one will be too.
		if (d_age_grid_mask_raster && d_age_grid_coverage_raster)
		{
			// Create a reconstructed raster with the age grid.
			d_reconstructed_raster =
					GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create(
							renderer,
							d_multi_resolution_cube_raster.get(),
							d_reconstructed_polygon_meshes.get(),
							GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::AgeGridRaster(
									d_age_grid_mask_raster.get(),
									d_age_grid_coverage_raster.get()));
		}
		else
		{
			// Create a reconstructed raster without the age grid.
			d_reconstructed_raster =
					GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create(
							renderer,
							d_multi_resolution_cube_raster.get(),
							d_reconstructed_polygon_meshes.get());
		}
	}

	return d_reconstructed_raster;
}


bool
GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	// We require the source cube raster and the static polygon raster reconstructor.
	// But the age grid is optional.
	return d_cube_raster_layer_usage->is_required_direct_or_indirect_dependency(layer_proxy_handle) ||
		(d_reconstructed_polygon_meshes_layer_usage &&
			d_reconstructed_polygon_meshes_layer_usage.get()->is_required_direct_or_indirect_dependency(layer_proxy_handle));
}


void
GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::removing_layer(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle)
{
	// Return early if we not using an age grid or the age grid we're using does not
	// depend on the layer about to be removed.
	if (!d_age_grid_layer_usage ||
		!d_age_grid_layer_usage.get()->is_required_direct_or_indirect_dependency(layer_proxy_handle))
	{
		return;
	}

	// Stop using the age grid layer usage.
	d_age_grid_layer_usage = boost::none;
	d_age_grid_mask_raster = boost::none;
	d_age_grid_coverage_raster = boost::none;

	// We'll need to rebuild our reconstructed raster.
	d_reconstructed_raster = boost::none;
}


GPlatesGui::PersistentOpenGLObjects::MapRasterLayerUsage::MapRasterLayerUsage(
		const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> &raster_layer_usage,
		const GPlatesUtils::non_null_intrusive_ptr<StaticPolygonReconstructedRasterLayerUsage> &reconstructed_raster_layer_usage) :
	d_raster_layer_usage(raster_layer_usage),
	d_reconstructed_raster_layer_usage(reconstructed_raster_layer_usage)
{
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRasterMapView::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::MapRasterLayerUsage::get_multi_resolution_raster_map_view(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type &multi_resolution_map_cube_mesh)
{
	// Try getting the reconstructed raster.
	const boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
			reconstructed_raster = d_reconstructed_raster_layer_usage->get_static_polygon_reconstructed_raster(renderer);

	// If reconstructed raster is a different object...
	if (d_reconstructed_raster != reconstructed_raster)
	{
		d_reconstructed_raster = reconstructed_raster;

		// We need to rebuild the raster map view.
		d_multi_resolution_raster_map_view = boost::none;
	}

	// If we have a reconstructed raster then give preference to that.
	if (d_reconstructed_raster)
	{
		if (!d_multi_resolution_raster_map_view)
		{
			GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type multi_resolution_cube_reconstructed_raster =
					GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::create(
							renderer,
							d_reconstructed_raster.get());

			// Attempt to create the multi-resolution raster map view.
			d_multi_resolution_raster_map_view =
					GPlatesOpenGL::GLMultiResolutionRasterMapView::create(
							renderer,
							multi_resolution_cube_reconstructed_raster.get(),
							multi_resolution_map_cube_mesh.get());
		}

		return d_multi_resolution_raster_map_view;
	}

	// Try getting the regular (unreconstructed) raster.
	const boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> raster =
			d_raster_layer_usage->get_multi_resolution_raster(renderer);

	// If (unreconstructed) raster is a different object...
	if (d_raster != raster)
	{
		d_raster = raster;

		// We need to rebuild the raster map view.
		d_multi_resolution_raster_map_view = boost::none;
	}

	// If we have an (reconstructed) raster then fallback to that.
	if (d_raster)
	{
		if (!d_multi_resolution_raster_map_view)
		{
			GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type multi_resolution_cube_raster =
					GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
							renderer,
							d_raster.get(),
							GPlatesOpenGL::GLMultiResolutionCubeRaster::DEFAULT_TILE_TEXEL_DIMENSION,
							true/*adapt_tile_dimension_to_source_resolution*/,
							GPlatesOpenGL::GLMultiResolutionCubeRaster::DEFAULT_FIXED_POINT_TEXTURE_FILTER,
							// Use non-default here - our source GLMultiResolutionRaster in turn references
							// a GLVisualRasterSource which has caching that insulates us from the
							// file system so we don't really need any caching in GLMultiResolutionRaster
							// and can save a lot of texture memory usage as a result...
							false/*cache_source_tile_textures*/);

			// Attempt to create the multi-resolution raster map view.
			d_multi_resolution_raster_map_view =
					GPlatesOpenGL::GLMultiResolutionRasterMapView::create(
							renderer,
							multi_resolution_cube_raster.get(),
							multi_resolution_map_cube_mesh.get());
		}
	}

	return d_multi_resolution_raster_map_view;
}


bool
GPlatesGui::PersistentOpenGLObjects::MapRasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	// We require the source raster but the reconstructed raster is optional.
	return d_raster_layer_usage.get()->is_required_direct_or_indirect_dependency(layer_proxy_handle);
}


GPlatesGui::PersistentOpenGLObjects::GLLayer::GLLayer(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy) :
	d_layer_proxy(layer_proxy),
	d_layer_usages(LayerUsage::NUM_TYPES)
{
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage>
GPlatesGui::PersistentOpenGLObjects::GLLayer::get_raster_layer_usage()
{
	const LayerUsage::Type layer_usage_type = LayerUsage::RASTER;

	if (!d_layer_usages[layer_usage_type])
	{
		// This will throw an exception (or abort in debug mode) if the dynamic cast fails
		// but that's because it's a program error if it fails.
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type raster_layer_proxy =
				GPlatesUtils::dynamic_pointer_cast<GPlatesAppLogic::RasterLayerProxy>(d_layer_proxy);

		// Create a new RasterLayerUsage object.
		d_layer_usages[layer_usage_type] = 
				GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage>(
						new RasterLayerUsage(raster_layer_proxy));
	}

	return GPlatesUtils::dynamic_pointer_cast<RasterLayerUsage>(d_layer_usages[layer_usage_type].get());
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesGui::PersistentOpenGLObjects::CubeRasterLayerUsage>
GPlatesGui::PersistentOpenGLObjects::GLLayer::get_cube_raster_layer_usage()
{
	const LayerUsage::Type layer_usage_type = LayerUsage::CUBE_RASTER;

	if (!d_layer_usages[layer_usage_type])
	{
		// Create a new CubeRasterLayerUsage object.
		d_layer_usages[layer_usage_type] = 
				GPlatesUtils::non_null_intrusive_ptr<CubeRasterLayerUsage>(
						new CubeRasterLayerUsage(
								// Note: Connecting to the raster in the same layer...
								get_raster_layer_usage()));
	}

	return GPlatesUtils::dynamic_pointer_cast<CubeRasterLayerUsage>(d_layer_usages[layer_usage_type].get());
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage>
GPlatesGui::PersistentOpenGLObjects::GLLayer::get_age_grid_layer_usage()
{
	const LayerUsage::Type layer_usage_type = LayerUsage::AGE_GRID;

	if (!d_layer_usages[layer_usage_type])
	{
		// This will throw an exception (or abort in debug mode) if the dynamic cast fails
		// but that's because it's a program error if it fails.
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type age_grid_layer_proxy =
				GPlatesUtils::dynamic_pointer_cast<GPlatesAppLogic::RasterLayerProxy>(d_layer_proxy);

		// Create a new RasterLayerUsage object.
		d_layer_usages[layer_usage_type] = 
				GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage>(
						new AgeGridLayerUsage(
								age_grid_layer_proxy));
	}

	return GPlatesUtils::dynamic_pointer_cast<AgeGridLayerUsage>(d_layer_usages[layer_usage_type].get());
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage>
GPlatesGui::PersistentOpenGLObjects::GLLayer::get_reconstructed_static_polygon_meshes_layer_usage()
{
	const LayerUsage::Type layer_usage_type = LayerUsage::RECONSTRUCTED_STATIC_POLYGON_MESHES;

	if (!d_layer_usages[layer_usage_type])
	{
		// This will throw an exception (or abort in debug mode) if the dynamic cast fails
		// but that's because it's a program error if it fails.
		const GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type
				reconstructed_static_polygon_meshes_layer_proxy =
						GPlatesUtils::dynamic_pointer_cast<GPlatesAppLogic::ReconstructLayerProxy>(
								d_layer_proxy);

		// Create a new ReconstructedStaticPolygonMeshesLayerUsage object.
		d_layer_usages[layer_usage_type] = 
				GPlatesUtils::non_null_intrusive_ptr<ReconstructedStaticPolygonMeshesLayerUsage>(
						new ReconstructedStaticPolygonMeshesLayerUsage(
								reconstructed_static_polygon_meshes_layer_proxy));
	}

	return GPlatesUtils::dynamic_pointer_cast<ReconstructedStaticPolygonMeshesLayerUsage>(
			d_layer_usages[layer_usage_type].get());
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage>
GPlatesGui::PersistentOpenGLObjects::GLLayer::get_static_polygon_reconstructed_raster_layer_usage()
{
	const LayerUsage::Type layer_usage_type = LayerUsage::STATIC_POLYGON_RECONSTRUCTED_RASTER;

	if (!d_layer_usages[layer_usage_type])
	{
		// Create a new StaticPolygonReconstructedRasterLayerUsage object.
		// NOTE: We only connect to the cube raster layer usage in this layer but
		// we don't connect to the static polygon meshes layer usage or age grid layer usage
		// because those can come from other layers and can change dynamically as the user
		// changes layer connections.
		d_layer_usages[layer_usage_type] = 
				GPlatesUtils::non_null_intrusive_ptr<StaticPolygonReconstructedRasterLayerUsage>(
						new StaticPolygonReconstructedRasterLayerUsage(
								get_cube_raster_layer_usage()));
	}

	return GPlatesUtils::dynamic_pointer_cast<StaticPolygonReconstructedRasterLayerUsage>(
			d_layer_usages[layer_usage_type].get());
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesGui::PersistentOpenGLObjects::MapRasterLayerUsage>
GPlatesGui::PersistentOpenGLObjects::GLLayer::get_map_raster_layer_usage()
{
	const LayerUsage::Type layer_usage_type = LayerUsage::MAP_RASTER;

	if (!d_layer_usages[layer_usage_type])
	{
		// Create a new MapRasterLayerUsage object.
		d_layer_usages[layer_usage_type] = 
				GPlatesUtils::non_null_intrusive_ptr<MapRasterLayerUsage>(
						new MapRasterLayerUsage(
								// Note: Connecting to the cube raster in the same layer...
								get_raster_layer_usage(),
								get_static_polygon_reconstructed_raster_layer_usage()));
	}

	return GPlatesUtils::dynamic_pointer_cast<MapRasterLayerUsage>(d_layer_usages[layer_usage_type].get());
}


void
GPlatesGui::PersistentOpenGLObjects::GLLayer::remove_references_to_layer(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_to_be_removed)
{
	// We can remove each layer usage as we come across it even if it depends on another
	// layer usage inside *this* layer because of the power of shared pointers.
	//
	// The main aim here is to remove any of *this* layer's shared pointer references to
	// layer usages that depend directly or indirectly on the layer proxy being removed.
	// If other layers reference our layer usages (that are being removed) then they'll remove
	// their references when it's their turn and when all is done there should be no more
	// references to those layer usages being removed.
	BOOST_FOREACH(
			// NOTE: This must be a reference and not a copy...
			boost::optional<GPlatesUtils::non_null_intrusive_ptr<LayerUsage> > &layer_usage_opt,
			d_layer_usages)
	{
		if (!layer_usage_opt)
		{
			// Layer usage slot not being used so continue to the next one.
			continue;
		}

		// If the current layer usage has a *required* dependency on the layer (proxy) being removed
		// then remove our reference to the current layer usage.
		// Otherwise it's still possible the current layer usage has an *optional* dependency
		// on the layer (proxy) being removed so give it a chance to stop using that dependency.
		if (layer_usage_opt.get()->is_required_direct_or_indirect_dependency(layer_proxy_to_be_removed))
		{
			// Remove our reference to the layer usage.
			layer_usage_opt = boost::none;
		}
		else
		{
			layer_usage_opt.get()->removing_layer(layer_proxy_to_be_removed);
		}
	}
}


GPlatesGui::PersistentOpenGLObjects::GLLayer &
GPlatesGui::PersistentOpenGLObjects::GLLayers::get_layer(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy)
{
	const layer_map_type::iterator gl_layer_iter = d_layer_map.find(layer_proxy);
	if (gl_layer_iter != d_layer_map.end())
	{
		// Return existing GL layer.
		return *gl_layer_iter->second;
	}

	// Create, and insert, a new GL layer.
	const GLLayer::non_null_ptr_type gl_layer = GLLayer::create(layer_proxy);

	std::pair<layer_map_type::iterator, bool> insert_result =
			d_layer_map.insert(
					layer_map_type::value_type(layer_proxy, gl_layer));

	return *insert_result.first->second;
}


void
GPlatesGui::PersistentOpenGLObjects::GLLayers::remove_layer(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_to_be_removed)
{
	// Look for the layer proxy in our map and remove it to release the memory and
	// OpenGL resources used by it.
	layer_map_type::iterator layer_proxy_to_be_removed_iter = d_layer_map.find(layer_proxy_to_be_removed);
	if (layer_proxy_to_be_removed_iter == d_layer_map.end())
	{
		// If we didn't find the GL layer then it only means a layer is about to be removed that
		// we have not created a GL layer for (eg, because it wasn't a raster or reconstructed
		// polygon meshes request for the layer).
		return;
	}

	// Remove the GL layer - each GL layer is associated with each layer proxy and directly
	// references it - so we don't need to explicitly check whether it does or not.
	d_layer_map.erase(layer_proxy_to_be_removed_iter);

	// Iterate over all remaining layers and within each layer remove any individual layer usages
	// that reference the layer proxy about to be removed.
	BOOST_FOREACH(const layer_map_type::value_type &gl_layer_map_entry, d_layer_map)
	{
		const GLLayer::non_null_ptr_type &gl_layer = gl_layer_map_entry.second;
		gl_layer->remove_references_to_layer(layer_proxy_to_be_removed);
	}
}


GPlatesGui::PersistentOpenGLObjects::ListObjects::ListObjects(
		const boost::shared_ptr<GPlatesOpenGL::GLContext::SharedState> &opengl_shared_state_input,
		const NonListObjects &non_list_objects) :
	opengl_shared_state(opengl_shared_state_input),
	d_non_list_objects(non_list_objects)
{
}


GPlatesOpenGL::GLMultiResolutionCubeMesh::non_null_ptr_to_const_type
GPlatesGui::PersistentOpenGLObjects::ListObjects::get_multi_resolution_cube_mesh(
		GPlatesOpenGL::GLRenderer &renderer) const
{
	if (!d_multi_resolution_cube_mesh)
	{
		d_multi_resolution_cube_mesh =
				GPlatesOpenGL::GLMultiResolutionCubeMesh::non_null_ptr_to_const_type(
						GPlatesOpenGL::GLMultiResolutionCubeMesh::create(renderer));
	}

	return d_multi_resolution_cube_mesh.get();
}


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type
GPlatesGui::PersistentOpenGLObjects::ListObjects::get_multi_resolution_map_cube_mesh(
		GPlatesOpenGL::GLRenderer &renderer,
		const MapProjection &map_projection) const
{
	if (!d_multi_resolution_map_cube_mesh)
	{
		d_multi_resolution_map_cube_mesh =
				GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_type(
						GPlatesOpenGL::GLMultiResolutionMapCubeMesh::create(
								renderer,
								map_projection));
	}

	// Update the map projection if it's changed.
	d_multi_resolution_map_cube_mesh.get()->update_map_projection(renderer, map_projection);

	return d_multi_resolution_map_cube_mesh.get();
}


GPlatesOpenGL::GLMultiResolutionFilledPolygons::non_null_ptr_type
GPlatesGui::PersistentOpenGLObjects::ListObjects::get_multi_resolution_filled_polygons(
		GPlatesOpenGL::GLRenderer &renderer) const
{
	if (!d_multi_resolution_filled_polygons)
	{
		d_multi_resolution_filled_polygons =
				GPlatesOpenGL::GLMultiResolutionFilledPolygons::create(
						renderer,
						get_multi_resolution_cube_mesh(renderer));
	}

	return d_multi_resolution_filled_polygons.get();
}
