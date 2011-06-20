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


void
GPlatesGui::PersistentOpenGLObjects::render_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type &resolved_raster,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &source_raster_colour_palette)
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
	}

	// The age grid layer usage comes from another layer.
	boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > age_grid_layer_usage;
	if (resolved_raster->get_age_grid_layer_proxy())
	{
		// Get the GL layer corresponding to the layer the age grid came from.
		GLLayer &gl_age_grid_layer = d_list_objects->gl_layers.get_layer(
				resolved_raster->get_age_grid_layer_proxy().get());

		age_grid_layer_usage = gl_age_grid_layer.get_age_grid_layer_usage();
	}

	// Get the raster layer usage so we can set the colour palette.
	const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> raster_layer_usage =
			gl_raster_layer.get_raster_layer_usage();

	// Set the colour palette before we get the multi-resolution raster because this
	// could invalidate it requiring it to be rebuilt.
	raster_layer_usage->set_raster_colour_palette(source_raster_colour_palette);

	// Need to force the raster to update if there's a time-dependent raster and we're
	// reconstructing this raster below. Doing this causes 'GLMultiResolutionRaster' to
	// check the proxied raster source and change it if necessary (without rebuilding itself).
	//
	// FIXME: Need to sort out the mess of dependencies.
	raster_layer_usage->get_multi_resolution_raster();

	// Get the static polygon reconstructed raster layer usage so we can update its input layer usages.
	const GPlatesUtils::non_null_intrusive_ptr<StaticPolygonReconstructedRasterLayerUsage>
			static_polygon_reconstructed_raster_layer_usage =
					gl_raster_layer.get_static_polygon_reconstructed_raster_layer_usage();

	// Update the layer usage inputs.
	static_polygon_reconstructed_raster_layer_usage->update(
			reconstructed_polygon_meshes_layer_usage,
			age_grid_layer_usage);


	//
	// Now that we've finished updating everything we can get onto rendering...
	//


	// If we have reconstructed polygons then we can reconstruct the raster...
	if (reconstructed_polygon_meshes_layer_usage)
	{
		boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
				static_polygon_reconstructed_raster =
						static_polygon_reconstructed_raster_layer_usage->get_static_polygon_reconstructed_raster(
								resolved_raster->get_reconstruction_time());
		if (static_polygon_reconstructed_raster)
		{
			//
			// We are rendering a *reconstructed* raster.
			//
			static_polygon_reconstructed_raster.get()->render(renderer);

			return;
		}
		// else drop through and render the *unreconstructed* raster...
	}

	// We have a regular, unreconstructed raster - although it can still be a time-dependent raster.

	// Get the multi-resolution raster.
	boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> multi_resolution_raster =
			raster_layer_usage->get_multi_resolution_raster();

	// Render the multi-resolution raster.
	if (multi_resolution_raster)
	{
		multi_resolution_raster.get()->render(renderer);
	}
}


void
GPlatesGui::PersistentOpenGLObjects::render_filled_polygons(
		GPlatesOpenGL::GLRenderer &renderer,
		const filled_polygons_spatial_partition_type &filled_polygons)
{
	d_list_objects->get_multi_resolution_filled_polygons()->render(renderer, filled_polygons);
}


void
GPlatesGui::PersistentOpenGLObjects::handle_layer_about_to_be_removed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
	GPlatesAppLogic::LayerProxy::non_null_ptr_type layer_proxy = layer.get_layer_proxy();

	d_list_objects->gl_layers.remove_layer(layer_proxy.get());
}


GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::RasterLayerUsage(
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &raster_layer_proxy,
		const boost::shared_ptr<GPlatesOpenGL::GLTextureResourceManager> &texture_resource_manager,
		const boost::shared_ptr<GPlatesOpenGL::GLVertexBufferResourceManager> &vertex_buffer_resource_manager) :
	d_texture_resource_manager(texture_resource_manager),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
	d_raster_layer_proxy(raster_layer_proxy)
{
}


void
GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::set_raster_colour_palette(
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette)
{
	if (raster_colour_palette == d_raster_colour_palette)
	{
		// Nothing has changed so just return.
		return;
	}

	d_raster_colour_palette = raster_colour_palette;

	// If we have a proxied raster source then attempt to change the raster first
	// since it's cheaper than rebuilding the multi-resolution raster.
	if (d_proxied_raster_source)
	{
		if (d_raster_layer_proxy->get_proxied_raster())
		{
			if (d_proxied_raster_source.get()->change_raster(
					d_raster_layer_proxy->get_proxied_raster().get(),
					d_raster_colour_palette.get()))
			{
				// We are now up-to-date with respect to the proxied raster in the raster layer proxy.
				d_raster_layer_proxy->get_proxied_raster_subject_token().update_observer(
						d_proxied_raster_observer_token);

				return;
			}
		}
	}

	// The colour palette has changed and we were unable to change the raster so
	// invalidate everything.
	d_proxied_raster_source = boost::none;
	d_multi_resolution_raster = boost::none;
	d_rebuild_subject_token.invalidate();

	// We are now up-to-date with respect to the proxied raster in the raster layer proxy.
	d_raster_layer_proxy->get_proxied_raster_subject_token().update_observer(
			d_proxied_raster_observer_token);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::get_multi_resolution_raster()
{
	if (!d_raster_layer_proxy->get_proxied_raster() || !d_raster_colour_palette)
	{
		d_proxied_raster_source = boost::none;

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
	if (!d_raster_layer_proxy->get_proxied_raster_subject_token().is_observer_up_to_date(
			d_proxied_raster_observer_token))
	{
		// If we have a proxied raster source then attempt to change the raster first
		// since it's cheaper than rebuilding the multi-resolution raster.
		if (d_proxied_raster_source)
		{
			if (!d_proxied_raster_source.get()->change_raster(
					d_raster_layer_proxy->get_proxied_raster().get(),
					d_raster_colour_palette.get()))
			{
				// Unsuccessful, so rebuild.
				d_proxied_raster_source = boost::none;
			}
		}

		// We are now up-to-date with respect to the proxied raster in the raster layer proxy.
		d_raster_layer_proxy->get_proxied_raster_subject_token().update_observer(
				d_proxied_raster_observer_token);
	}

	// Rebuild the proxied raster source if necessary.
	if (!d_proxied_raster_source)
	{
		// NOTE: We also invalidate the multi-resolution raster since it must link
		// to the proxied raster source and hence must also be rebuilt.
		d_multi_resolution_raster = boost::none;

		d_proxied_raster_source = GPlatesOpenGL::GLProxiedRasterSource::create(
				d_raster_layer_proxy->get_proxied_raster().get(),
				d_raster_colour_palette.get());
		if (!d_proxied_raster_source)
		{
			// Unable to create a source proxy raster so nothing we can do.
			return boost::none;
		}
	}

	// If we're not up-to-date with respect to the raster feature in the raster layer proxy then rebuild.
	if (!d_raster_layer_proxy->get_raster_feature_subject_token().is_observer_up_to_date(
			d_raster_feature_observer_token))
	{
		d_multi_resolution_raster = boost::none;

		// We are now up-to-date with respect to the raster feature in the raster layer proxy.
		d_raster_layer_proxy->get_raster_feature_subject_token().update_observer(
				d_raster_feature_observer_token);
	}

	// Rebuild the multi-resolution raster if necessary.
	if (!d_multi_resolution_raster)
	{
		const GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type multi_resolution_raster =
				GPlatesOpenGL::GLMultiResolutionRaster::create(
						d_raster_layer_proxy->get_georeferencing().get(),
						d_proxied_raster_source.get(),
						d_texture_resource_manager,
						d_vertex_buffer_resource_manager);

		d_multi_resolution_raster = multi_resolution_raster;
	}

	return d_multi_resolution_raster.get();
}


const GPlatesUtils::SubjectToken &
GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::get_rebuild_subject_token()
{
	// If we're not up-to-date with respect to the proxied raster in the raster layer proxy
	// then we do *not* need to tell clients to update themselves with respect to us.
	// This is because even if the proxied raster changes we are not rebuilding our proxied raster
	// source (ie, not destroying and recreating) which means clients don't need to rebuild either.

	// However if we're not up-to-date with respect to the raster feature in the raster layer proxy...
	if (!d_raster_layer_proxy->get_raster_feature_subject_token().is_observer_up_to_date(
			d_raster_feature_observer_token))
	{
		// Let clients know they need to update themselves with respect to us.
		d_rebuild_subject_token.invalidate();
	}

	return d_rebuild_subject_token;
}


bool
GPlatesGui::PersistentOpenGLObjects::RasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy) const
{
	return layer_proxy == d_raster_layer_proxy;
}


GPlatesGui::PersistentOpenGLObjects::CubeRasterLayerUsage::CubeRasterLayerUsage(
		const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> &raster_layer_usage,
		const GPlatesGlobal::PointerTraits<cube_subdivision_projection_transforms_cache_type>::non_null_ptr_type &
				cube_subdivision_projection_transforms_cache,
		const GPlatesOpenGL::GLTextureResourceManager::shared_ptr_type &texture_resource_manager) :
	d_texture_resource_manager(texture_resource_manager),
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_raster_layer_usage(raster_layer_usage)
{
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::CubeRasterLayerUsage::get_multi_resolution_cube_raster()
{
	// If we're not up-to-date with respect to the raster layer usage...
	if (!d_raster_layer_usage->get_rebuild_subject_token().is_observer_up_to_date(
			d_raster_layer_usage_observer_token))
	{
		// Then we need to rebuild the multi-resolution cube raster.
		d_multi_resolution_cube_raster = boost::none;

		// We are now up-to-date with respect to the raster layer usage.
		d_raster_layer_usage->get_rebuild_subject_token().update_observer(
				d_raster_layer_usage_observer_token);
	}

	if (!d_multi_resolution_cube_raster)
	{
		boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> multi_resolution_raster =
				d_raster_layer_usage->get_multi_resolution_raster();
		if (!multi_resolution_raster)
		{
			// Unable to get the multi-resolution raster so nothing we can do.
			return boost::none;
		}

		// Attempt to create the multi-resolution cube raster.
		d_multi_resolution_cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						multi_resolution_raster.get(),
						d_cube_subdivision_projection_transforms_cache,
						d_texture_resource_manager);
	}

	return d_multi_resolution_cube_raster;
}


const GPlatesUtils::SubjectToken &
GPlatesGui::PersistentOpenGLObjects::CubeRasterLayerUsage::get_rebuild_subject_token()
{
	// If we're not up-to-date with respect to the raster layer usage...
	if (!d_raster_layer_usage->get_rebuild_subject_token().is_observer_up_to_date(
			d_raster_layer_usage_observer_token))
	{
		// Let clients know they need to update themselves with respect to us.
		d_rebuild_subject_token.invalidate();
	}

	return d_rebuild_subject_token;
}


bool
GPlatesGui::PersistentOpenGLObjects::CubeRasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy) const
{
	return d_raster_layer_usage->is_required_direct_or_indirect_dependency(layer_proxy);
}


GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::AgeGridLayerUsage(
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &age_grid_raster_layer_proxy,
		const GPlatesGlobal::PointerTraits<cube_subdivision_projection_transforms_cache_type>::non_null_ptr_type &
				cube_subdivision_projection_transforms_cache,
		const GPlatesOpenGL::GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
		const boost::shared_ptr<GPlatesOpenGL::GLVertexBufferResourceManager> &vertex_buffer_resource_manager) :
	d_texture_resource_manager(texture_resource_manager),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_age_grid_raster_layer_proxy(age_grid_raster_layer_proxy)
{
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::get_age_grid_mask_multi_resolution_cube_raster(
		const double &reconstruction_time)
{
	check_input_raster();

	// Rebuild the age grid *mask* cube raster if necessary.
	if (!d_age_grid_mask_multi_resolution_cube_raster)
	{
		if (!d_age_grid_mask_multi_resolution_source)
		{
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
							reconstruction_time,
							age_grid_raster.get(),
							d_texture_resource_manager);
			if (!d_age_grid_mask_multi_resolution_source)
			{
				// Unable to get age grid mask source so nothing we can do.
				return boost::none;
			}
		}

		if (!d_age_grid_mask_multi_resolution_raster)
		{
			// Create an age grid mask multi-resolution raster.
			GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type age_grid_mask_multi_resolution_raster =
					GPlatesOpenGL::GLMultiResolutionRaster::create(
							d_age_grid_raster_layer_proxy->get_georeferencing().get(),
							d_age_grid_mask_multi_resolution_source.get(),
							d_texture_resource_manager,
							d_vertex_buffer_resource_manager);

			d_age_grid_mask_multi_resolution_raster = age_grid_mask_multi_resolution_raster;
		}

		// Create an age grid mask multi-resolution cube raster.
		GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type age_grid_mask_multi_resolution_cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						d_age_grid_mask_multi_resolution_raster.get(),
						d_cube_subdivision_projection_transforms_cache,
						d_texture_resource_manager);

		d_age_grid_mask_multi_resolution_cube_raster = age_grid_mask_multi_resolution_cube_raster;
	}

	update(reconstruction_time);

	return d_age_grid_mask_multi_resolution_cube_raster;
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::get_age_grid_coverage_multi_resolution_cube_raster(
		const double &reconstruction_time)
{
	check_input_raster();

	// Rebuild the age grid *coverage* cube raster if necessary.
	if (!d_age_grid_coverage_multi_resolution_cube_raster)
	{
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
					GPlatesOpenGL::GLAgeGridCoverageSource::create(
							age_grid_raster.get());
			if (!d_age_grid_coverage_multi_resolution_source)
			{
				// Unable to get age grid coverage source so nothing we can do.
				return boost::none;
			}
		}

		if (!d_age_grid_coverage_multi_resolution_raster)
		{
			// Create an age grid coverage multi-resolution raster.
			GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type age_grid_coverage_multi_resolution_raster =
					GPlatesOpenGL::GLMultiResolutionRaster::create(
							d_age_grid_raster_layer_proxy->get_georeferencing().get(),
							d_age_grid_coverage_multi_resolution_source.get(),
							d_texture_resource_manager,
							d_vertex_buffer_resource_manager);

			d_age_grid_coverage_multi_resolution_raster = age_grid_coverage_multi_resolution_raster;
		}

		// Create an age grid coverage multi-resolution cube raster.
		GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type age_grid_coverage_multi_resolution_cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						d_age_grid_coverage_multi_resolution_raster.get(),
						d_cube_subdivision_projection_transforms_cache,
						d_texture_resource_manager);

		d_age_grid_coverage_multi_resolution_cube_raster = age_grid_coverage_multi_resolution_cube_raster;
	}

	update(reconstruction_time);

	return d_age_grid_coverage_multi_resolution_cube_raster;
}


const GPlatesUtils::SubjectToken &
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::get_rebuild_subject_token()
{
	//
	// NOTE: Since we use the age grid only at *present-day* we don't need to update ourself
	// if the age grid raster layer has changed proxied rasters (ie, if, for some reason,
	// the age grid raster is time-dependent).
	//
	// So we use the 'raster feature' subject token instead of the 'proxied raster' subject token.
	// 

	// If we're not up-to-date with respect to the *present-day* proxied raster in the age grid layer proxy...
	if (!d_age_grid_raster_layer_proxy->get_raster_feature_subject_token().is_observer_up_to_date(
			d_age_grid_raster_feature_observer_token))
	{
		// Let clients know they need to update themselves with respect to us.
		d_rebuild_subject_token.invalidate();
	}

	return d_rebuild_subject_token;
}


bool
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy) const
{
	return layer_proxy == d_age_grid_raster_layer_proxy;
}


void
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::check_input_raster()
{
	if (!d_age_grid_raster_layer_proxy->get_georeferencing())
	{
		// Rebuild everything that depends on georeferencing directly or indirectly.
		d_age_grid_mask_multi_resolution_raster = boost::none;
		d_age_grid_mask_multi_resolution_cube_raster = boost::none;
		d_age_grid_coverage_multi_resolution_raster = boost::none;
		d_age_grid_coverage_multi_resolution_cube_raster = boost::none;

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
		d_age_grid_mask_multi_resolution_cube_raster = boost::none;
		d_age_grid_coverage_multi_resolution_source = boost::none;
		d_age_grid_coverage_multi_resolution_raster = boost::none;
		d_age_grid_coverage_multi_resolution_cube_raster = boost::none;

		// We are now up-to-date with respect to the age grid feature in the age grid layer proxy.
		d_age_grid_raster_layer_proxy->get_raster_feature_subject_token().update_observer(
				d_age_grid_raster_feature_observer_token);
	}
}


void
GPlatesGui::PersistentOpenGLObjects::AgeGridLayerUsage::update(
		const double &reconstruction_time)
{
	// Update the reconstruction time for the age grid mask.
	d_age_grid_mask_multi_resolution_source.get()->update_reconstruction_time(reconstruction_time);
}


GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::ReconstructedStaticPolygonMeshesLayerUsage(
		const GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type &reconstructed_static_polygon_meshes_layer_proxy,
		const GPlatesGlobal::PointerTraits<cube_subdivision_projection_transforms_cache_type>::non_null_ptr_type &
				cube_subdivision_projection_transforms_cache,
		const GPlatesGlobal::PointerTraits<cube_subdivision_loose_bounds_cache_type>::non_null_ptr_type &
				cube_subdivision_loose_bounds_cache,
		const GPlatesGlobal::PointerTraits<cube_subdivision_bounding_polygons_cache_type>::non_null_ptr_type &
				cube_subdivision_bounding_polygons_cache,
		const GPlatesOpenGL::GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager) :
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_cube_subdivision_loose_bounds_cache(cube_subdivision_loose_bounds_cache),
	d_cube_subdivision_bounding_polygons_cache(cube_subdivision_bounding_polygons_cache),
	d_reconstructed_static_polygon_meshes_layer_proxy(reconstructed_static_polygon_meshes_layer_proxy)
{
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type
GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::get_reconstructed_static_polygon_meshes(
		const double &reconstruction_time)
{
	// If we're not up-to-date with respect to the present day polygons in the reconstruct layer proxy...
	if (!d_reconstructed_static_polygon_meshes_layer_proxy->get_reconstructable_feature_collections_subject_token()
		.is_observer_up_to_date(d_present_day_polygons_observer_token))
	{
		// The present day polygons have changed so need to rebuild the meshes.
		d_reconstructed_static_polygon_meshes = boost::none;
		d_reconstruction_time = boost::none;

		// We are now up-to-date with respect to the present day polygons in the reconstruct layer proxy.
		d_reconstructed_static_polygon_meshes_layer_proxy->get_reconstructable_feature_collections_subject_token()
			.update_observer(d_present_day_polygons_observer_token);
	}

	// Rebuild the GLReconstructedStaticPolygonMeshes object if necessary.
	if (!d_reconstructed_static_polygon_meshes)
	{
		d_reconstructed_static_polygon_meshes =
				GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::create(
						d_reconstructed_static_polygon_meshes_layer_proxy->get_present_day_polygon_meshes(),
						d_reconstructed_static_polygon_meshes_layer_proxy->get_present_day_geometries(),
						d_reconstructed_static_polygon_meshes_layer_proxy->get_reconstructions_spatial_partition(
								reconstruction_time),
						d_cube_subdivision_projection_transforms_cache,
						d_cube_subdivision_loose_bounds_cache,
						d_cube_subdivision_bounding_polygons_cache,
						d_vertex_buffer_resource_manager);

		// We are now up-to-date with the reconstruction time.
		d_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);

		// We are now up-to-date with respect to the reconstructed polygons in the reconstruct layer proxy.
		d_reconstructed_static_polygon_meshes_layer_proxy->get_subject_token().update_observer(
				d_reconstructed_polygons_observer_token);
	}

	update(reconstruction_time);

	return d_reconstructed_static_polygon_meshes.get();
}


void
GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::update(
		const double &reconstruction_time)
{
	// Do we need to update the reconstructed static polygons meshes for the current reconstruction time?
	bool need_to_update_reconstruction_time = false;

	if (d_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		need_to_update_reconstruction_time = true;

		d_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// If we're not up-to-date with respect to the reconstructed polygons in the reconstruct layer proxy...
	if (!d_reconstructed_static_polygon_meshes_layer_proxy->get_subject_token().is_observer_up_to_date(
			d_reconstructed_polygons_observer_token))
	{
		need_to_update_reconstruction_time = true;

		// We are now up-to-date with respect to the reconstructed polygons in the reconstruct layer proxy.
		d_reconstructed_static_polygon_meshes_layer_proxy->get_subject_token().update_observer(
				d_reconstructed_polygons_observer_token);
	}

	if (need_to_update_reconstruction_time)
	{
		d_reconstructed_static_polygon_meshes.get()->update(
				d_reconstructed_static_polygon_meshes_layer_proxy->get_reconstructions_spatial_partition(
						reconstruction_time));
	}
}


const GPlatesUtils::SubjectToken &
GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::get_rebuild_subject_token()
{
	// If we're not up-to-date with respect to the present day polygons in the reconstruct layer proxy...
	if (!d_reconstructed_static_polygon_meshes_layer_proxy->get_reconstructable_feature_collections_subject_token()
		.is_observer_up_to_date(d_present_day_polygons_observer_token))
	{
		// Let clients know they need to update themselves with respect to us.
		d_rebuild_subject_token.invalidate();
	}

	// If we're not up-to-date with respect to the reconstructed polygons in the reconstruct layer proxy
	// then we don't need to let the client know because we're not rebuilding.

	return d_rebuild_subject_token;
}


bool
GPlatesGui::PersistentOpenGLObjects::ReconstructedStaticPolygonMeshesLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy) const
{
	return layer_proxy == d_reconstructed_static_polygon_meshes_layer_proxy;
}


GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::StaticPolygonReconstructedRasterLayerUsage(
		const GPlatesUtils::non_null_intrusive_ptr<CubeRasterLayerUsage> &cube_raster_layer_usage,
		const GPlatesGlobal::PointerTraits<cube_subdivision_projection_transforms_cache_type>::non_null_ptr_type &
				cube_subdivision_projection_transforms_cache,
		const GPlatesGlobal::PointerTraits<cube_subdivision_bounds_cache_type>::non_null_ptr_type &
				cube_subdivision_bounds_cache,
		const GPlatesOpenGL::GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
		const GPlatesOpenGL::GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager) :
	d_texture_resource_manager(texture_resource_manager),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_cube_subdivision_bounds_cache(cube_subdivision_bounds_cache),
	d_cube_raster_layer_usage(cube_raster_layer_usage)
{
}


boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::get_static_polygon_reconstructed_raster(
		const double &reconstruction_time)
{
	// If we're not up-to-date with respect to the cube raster layer usage...
	if (!d_cube_raster_layer_usage->get_rebuild_subject_token().is_observer_up_to_date(
			d_cube_raster_layer_usage_observer_token))
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		// We are now up-to-date with respect to the cube raster layer usage.
		d_cube_raster_layer_usage->get_rebuild_subject_token().update_observer(
				d_cube_raster_layer_usage_observer_token);
	}

	// If we're not up-to-date with respect to the reconstructed polygon meshes layer usage...
	if (d_reconstructed_polygon_meshes_layer_usage &&
		!d_reconstructed_polygon_meshes_layer_usage.get()->get_rebuild_subject_token().is_observer_up_to_date(
			d_reconstructer_polygon_meshes_layer_usage_observer_token))
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		// We are now up-to-date with respect to the reconstructed polygon meshes layer usage.
		d_reconstructed_polygon_meshes_layer_usage.get()->get_rebuild_subject_token().update_observer(
				d_reconstructer_polygon_meshes_layer_usage_observer_token);
	}

	// If we're not up-to-date with respect to the age grid layer usage...
	if (d_age_grid_layer_usage &&
		!d_age_grid_layer_usage.get()->get_rebuild_subject_token().is_observer_up_to_date(
			d_age_grid_layer_usage_observer_token))
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		// We are now up-to-date with respect to the age grid layer usage.
		d_age_grid_layer_usage.get()->get_rebuild_subject_token().update_observer(
				d_age_grid_layer_usage_observer_token);
	}

	if (!d_reconstructed_raster)
	{
		// Attempt to get the multi-resolution cube raster.
		boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> multi_resolution_cube_raster =
				d_cube_raster_layer_usage->get_multi_resolution_cube_raster();
		if (!multi_resolution_cube_raster)
		{
			// We were unable to get the cube raster so nothing we can do.
			return boost::none;
		}

		// If we don't have reconstructed polygon meshes then we cannot reconstruct.
		if (!d_reconstructed_polygon_meshes_layer_usage)
		{
			return boost::none;
		}
		GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type reconstructed_static_polygon_meshes =
				d_reconstructed_polygon_meshes_layer_usage.get()->get_reconstructed_static_polygon_meshes(
						reconstruction_time);

		// We are reconstructing a raster - let's see if we are also using an age grid to help out.
		if (d_age_grid_layer_usage)
		{
			// Get the age grid mask cube raster.
			boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> age_grid_mask_cube_raster =
					d_age_grid_layer_usage.get()->get_age_grid_mask_multi_resolution_cube_raster(
							reconstruction_time);

			// Get the age grid coverage cube raster.
			boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> age_grid_coverage_cube_raster =
					d_age_grid_layer_usage.get()->get_age_grid_coverage_multi_resolution_cube_raster(
							reconstruction_time);

			// We can only reconstruct with an age grid if we have both cube rasters.
			// If one is successfully created then typically the other one will be too.
			if (age_grid_mask_cube_raster && age_grid_coverage_cube_raster)
			{
				// TODO: Add age grid to reconstructed raster.
				d_reconstructed_raster =
						GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create(
								multi_resolution_cube_raster.get(),
								reconstructed_static_polygon_meshes,
								d_cube_subdivision_projection_transforms_cache,
								d_cube_subdivision_bounds_cache,
								d_texture_resource_manager,
								d_vertex_buffer_resource_manager,
								GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::AgeGridParams(
										age_grid_mask_cube_raster.get(),
										age_grid_coverage_cube_raster.get()));

				return d_reconstructed_raster;
			}
			// else drop through to reconstructing without an age grid...
		}

		// Create a reconstructed raster without the age grid.
		d_reconstructed_raster =
				GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create(
						multi_resolution_cube_raster.get(),
						reconstructed_static_polygon_meshes,
						d_cube_subdivision_projection_transforms_cache,
						d_cube_subdivision_bounds_cache,
						d_texture_resource_manager,
						d_vertex_buffer_resource_manager);

		return d_reconstructed_raster;
	}

	// See if the reconstructed polygon meshes need updating.
	if (d_reconstructed_polygon_meshes_layer_usage)
	{
		d_reconstructed_polygon_meshes_layer_usage.get()->update(reconstruction_time);
	}

	// See if the age grid needs updating.
	if (d_age_grid_layer_usage)
	{
		d_age_grid_layer_usage.get()->update(reconstruction_time);
	}

	return d_reconstructed_raster;
}


void
GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::update(
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<ReconstructedStaticPolygonMeshesLayerUsage> > &
				reconstructed_polygon_meshes_layer_usage,
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > &age_grid_layer_usage)
{
	// See if we've switched layer usages.
	if (d_reconstructed_polygon_meshes_layer_usage != reconstructed_polygon_meshes_layer_usage)
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		// We're observing a different object.
		d_reconstructer_polygon_meshes_layer_usage_observer_token.reset();

		d_reconstructed_polygon_meshes_layer_usage = reconstructed_polygon_meshes_layer_usage;
	}
	if (d_age_grid_layer_usage != age_grid_layer_usage)
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		// We're observing a different object.
		d_age_grid_layer_usage_observer_token.reset();

		d_age_grid_layer_usage = age_grid_layer_usage;
	}
}


const GPlatesUtils::SubjectToken &
GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::get_rebuild_subject_token()
{
	// If we're not up-to-date with respect to the cube raster layer usage...
	if (!d_cube_raster_layer_usage->get_rebuild_subject_token().is_observer_up_to_date(
			d_cube_raster_layer_usage_observer_token))
	{
		// Let clients know they need to update themselves with respect to us.
		d_rebuild_subject_token.invalidate();
	}

	// If we're not up-to-date with respect to the reconstructed polygon meshes layer usage...
	if (d_reconstructed_polygon_meshes_layer_usage &&
		!d_reconstructed_polygon_meshes_layer_usage.get()->get_rebuild_subject_token().is_observer_up_to_date(
			d_reconstructer_polygon_meshes_layer_usage_observer_token))
	{
		// Let clients know they need to update themselves with respect to us.
		d_rebuild_subject_token.invalidate();
	}

	// If we're not up-to-date with respect to the age grid layer usage...
	if (d_age_grid_layer_usage &&
		!d_age_grid_layer_usage.get()->get_rebuild_subject_token().is_observer_up_to_date(
			d_age_grid_layer_usage_observer_token))
	{
		// Let clients know they need to update themselves with respect to us.
		d_rebuild_subject_token.invalidate();
	}

	return d_rebuild_subject_token;
}


bool
GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy) const
{
	// We require the source cube raster and the static polygon raster reconstructor.
	// But the age grid is optional.
	return d_cube_raster_layer_usage->is_required_direct_or_indirect_dependency(layer_proxy) ||
		(d_reconstructed_polygon_meshes_layer_usage &&
			d_reconstructed_polygon_meshes_layer_usage.get()->is_required_direct_or_indirect_dependency(layer_proxy));
}


void
GPlatesGui::PersistentOpenGLObjects::StaticPolygonReconstructedRasterLayerUsage::removing_layer(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy)
{
	// Return early if we not using an age grid or the age grid we're using does not
	// depend on the layer about to be removed.
	if (!d_age_grid_layer_usage ||
		!d_age_grid_layer_usage.get()->is_required_direct_or_indirect_dependency(layer_proxy))
	{
		return;
	}

	// Stop using the age grid layer usage.
	d_age_grid_layer_usage = boost::none;
	d_age_grid_layer_usage_observer_token.reset();

	// We'll need to rebuild our reconstructed raster.
	d_reconstructed_raster = boost::none;

	// Let clients know they need to update themselves with respect to us.
	d_rebuild_subject_token.invalidate();
}


GPlatesGui::PersistentOpenGLObjects::GLLayer::GLLayer(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy,
		const GPlatesOpenGL::GLCubeSubdivision::non_null_ptr_to_const_type &cube_subdivision,
		const GPlatesGlobal::PointerTraits<cube_subdivision_projection_transforms_cache_type>::non_null_ptr_type &
				cube_subdivision_projection_transforms_cache,
		const GPlatesGlobal::PointerTraits<cube_subdivision_bounds_cache_type>::non_null_ptr_type &
				cube_subdivision_bounds_cache,
		const GPlatesGlobal::PointerTraits<cube_subdivision_loose_bounds_cache_type>::non_null_ptr_type &
				cube_subdivision_loose_bounds_cache,
		const GPlatesGlobal::PointerTraits<cube_subdivision_bounding_polygons_cache_type>::non_null_ptr_type &
				cube_subdivision_bounding_polygons_cache,
		const boost::shared_ptr<GPlatesOpenGL::GLTextureResourceManager> &texture_resource_manager,
		const boost::shared_ptr<GPlatesOpenGL::GLVertexBufferResourceManager> &vertex_buffer_resource_manager) :
	d_cube_subdivision(cube_subdivision),
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_cube_subdivision_bounds_cache(cube_subdivision_bounds_cache),
	d_cube_subdivision_loose_bounds_cache(cube_subdivision_loose_bounds_cache),
	d_cube_subdivision_bounding_polygons_cache(cube_subdivision_bounding_polygons_cache),
	d_texture_resource_manager(texture_resource_manager),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
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
						new RasterLayerUsage(
								raster_layer_proxy,
								d_texture_resource_manager,
								d_vertex_buffer_resource_manager));
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
								get_raster_layer_usage(),
								d_cube_subdivision_projection_transforms_cache,
								d_texture_resource_manager));
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
								age_grid_layer_proxy,
								d_cube_subdivision_projection_transforms_cache,
								d_texture_resource_manager,
								d_vertex_buffer_resource_manager));
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
								reconstructed_static_polygon_meshes_layer_proxy,
								d_cube_subdivision_projection_transforms_cache,
								d_cube_subdivision_loose_bounds_cache,
								d_cube_subdivision_bounding_polygons_cache,
								d_vertex_buffer_resource_manager));
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
								get_cube_raster_layer_usage(),
								d_cube_subdivision_projection_transforms_cache,
								d_cube_subdivision_bounds_cache,
								d_texture_resource_manager,
								d_vertex_buffer_resource_manager));
	}

	return GPlatesUtils::dynamic_pointer_cast<StaticPolygonReconstructedRasterLayerUsage>(
			d_layer_usages[layer_usage_type].get());
}


void
GPlatesGui::PersistentOpenGLObjects::GLLayer::remove_references_to_layer(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy_to_be_removed)
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


GPlatesGui::PersistentOpenGLObjects::GLLayers::GLLayers(
		const GPlatesOpenGL::GLCubeSubdivision::non_null_ptr_to_const_type &cube_subdivision,
		const GPlatesGlobal::PointerTraits<cube_subdivision_projection_transforms_cache_type>::non_null_ptr_type &
				cube_subdivision_projection_transforms_cache,
		const GPlatesGlobal::PointerTraits<cube_subdivision_bounds_cache_type>::non_null_ptr_type &
				cube_subdivision_bounds_cache,
		const GPlatesGlobal::PointerTraits<cube_subdivision_loose_bounds_cache_type>::non_null_ptr_type &
				cube_subdivision_loose_bounds_cache,
		const GPlatesGlobal::PointerTraits<cube_subdivision_bounding_polygons_cache_type>::non_null_ptr_type &
				cube_subdivision_bounding_polygons_cache,
		const boost::shared_ptr<GPlatesOpenGL::GLTextureResourceManager> &texture_resource_manager,
		const boost::shared_ptr<GPlatesOpenGL::GLVertexBufferResourceManager> &vertex_buffer_resource_manager) :
	d_cube_subdivision(cube_subdivision),
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_cube_subdivision_bounds_cache(cube_subdivision_bounds_cache),
	d_cube_subdivision_loose_bounds_cache(cube_subdivision_loose_bounds_cache),
	d_cube_subdivision_bounding_polygons_cache(cube_subdivision_bounding_polygons_cache),
	d_texture_resource_manager(texture_resource_manager),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager)
{
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
	const GLLayer::non_null_ptr_type gl_layer =
			GLLayer::create(
					layer_proxy,
					d_cube_subdivision,
					d_cube_subdivision_projection_transforms_cache,
					d_cube_subdivision_bounds_cache,
					d_cube_subdivision_loose_bounds_cache,
					d_cube_subdivision_bounding_polygons_cache,
					d_texture_resource_manager,
					d_vertex_buffer_resource_manager);

	std::pair<layer_map_type::iterator, bool> insert_result =
			d_layer_map.insert(
					layer_map_type::value_type(layer_proxy, gl_layer));

	return *insert_result.first->second;
}


void
GPlatesGui::PersistentOpenGLObjects::GLLayers::remove_layer(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy_to_be_removed)
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
	gl_layers(
			non_list_objects.cube_subdivision,
			non_list_objects.cube_subdivision_projection_transforms_cache,
			non_list_objects.cube_subdivision_bounds_cache,
			non_list_objects.cube_subdivision_loose_bounds_cache,
			non_list_objects.cube_subdivision_bounding_polygons_cache,
			opengl_shared_state->get_texture_resource_manager(),
			opengl_shared_state->get_vertex_buffer_resource_manager()),
	d_non_list_objects(non_list_objects)
{
}


GPlatesOpenGL::GLMultiResolutionFilledPolygons::non_null_ptr_type
GPlatesGui::PersistentOpenGLObjects::ListObjects::get_multi_resolution_filled_polygons() const
{
	if (!d_multi_resolution_cube_mesh)
	{
		d_multi_resolution_cube_mesh =
				GPlatesOpenGL::GLMultiResolutionCubeMesh::non_null_ptr_to_const_type(
						GPlatesOpenGL::GLMultiResolutionCubeMesh::create(
								opengl_shared_state->get_texture_resource_manager(),
								opengl_shared_state->get_vertex_buffer_resource_manager()));
	}

	if (!d_multi_resolution_filled_polygons)
	{
		d_multi_resolution_filled_polygons =
				GPlatesOpenGL::GLMultiResolutionFilledPolygons::create(
						d_multi_resolution_cube_mesh.get(),
						d_non_list_objects.cube_subdivision_projection_transforms_cache,
						d_non_list_objects.cube_subdivision_bounds_cache,
						opengl_shared_state->get_texture_resource_manager(),
						opengl_shared_state->get_vertex_buffer_resource_manager());
	}

	return d_multi_resolution_filled_polygons.get();
}


GPlatesGui::PersistentOpenGLObjects::NonListObjects::NonListObjects() :
	cube_subdivision(GPlatesOpenGL::GLCubeSubdivision::create()),
	cube_subdivision_projection_transforms_cache(
			cube_subdivision_projection_transforms_cache_type::create(
					cube_subdivision,
					1024/*number cached projection transforms*/)),
	cube_subdivision_bounds_cache(
			cube_subdivision_bounds_cache_type::create(
					cube_subdivision,
					1024/*number cached bounds*/)),
	cube_subdivision_loose_bounds_cache(
			cube_subdivision_loose_bounds_cache_type::create(
					cube_subdivision,
					1024/*number cached loose bounds*/)),
	cube_subdivision_bounding_polygons_cache(
			cube_subdivision_bounding_polygons_cache_type::create(
					cube_subdivision,
					1024/*number cached bounding polygons*/))
{
}
