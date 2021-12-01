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

#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "GLVisualLayers.h"

#include "GL.h"
#include "GLCubeSubdivisionCache.h"
#include "GLMultiResolutionCubeReconstructedRaster.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructLayerProxy.h"
#include "app-logic/ResolvedScalarField3D.h"
#include "app-logic/ScalarField3DLayerProxy.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/RasterColourPalette.h"

#include "maths/MathsUtils.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"


GPlatesOpenGL::GLVisualLayers::GLVisualLayers(
		const GLContext::non_null_ptr_type &opengl_context,
		GPlatesAppLogic::ApplicationState &application_state) :
	d_non_list_objects(new NonListObjects()),
	d_list_objects(
			new ListObjects(
					opengl_context->get_shared_state(),
					*d_non_list_objects))
{
	make_signal_slot_connections(application_state.get_reconstruct_graph());
}


GPlatesOpenGL::GLVisualLayers::GLVisualLayers(
		const GLContext::non_null_ptr_type &opengl_context,
		const GLVisualLayers::non_null_ptr_type &objects_from_another_context,
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
GPlatesOpenGL::GLVisualLayers::make_signal_slot_connections(
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


GPlatesOpenGL::GLLight::non_null_ptr_type
GPlatesOpenGL::GLVisualLayers::get_light(
		GL &gl) const
{
	return d_list_objects->get_light(gl);
}


GPlatesOpenGL::GLVisualLayers::cache_handle_type
GPlatesOpenGL::GLVisualLayers::render_raster(
		GL &gl,
		const GLViewProjection &view_projection,
		const GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type &resolved_raster,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &source_raster_colour_palette,
		const GPlatesGui::Colour &source_raster_modulate_colour,
		float normal_map_height_field_scale_factor,
		boost::optional<GPlatesGui::MapProjection::non_null_ptr_to_const_type> map_projection)
{
	PROFILE_FUNC();

	// Get the GL layer corresponding to the layer the raster came from.
	GLLayer &gl_raster_layer = d_list_objects->gl_layers.get_layer(
			resolved_raster->get_raster_layer_proxy());

	// Get the raster layer usage so we can set the colour palette.
	const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> raster_layer_usage =
			gl_raster_layer.get_raster_layer_usage();

	// Set the colour palette.
	raster_layer_usage->set_raster_colour_palette(gl, source_raster_colour_palette);
	// Set the modulate colour.
	raster_layer_usage->set_raster_modulate_colour(gl, source_raster_modulate_colour);

	// The age grid layer usage comes from another layer.
	boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > age_grid_layer_usage;
	if (resolved_raster->get_age_grid_layer_proxy())
	{
		// Get the GL layer corresponding to the layer the age grid came from.
		GLLayer &gl_age_grid_layer = d_list_objects->gl_layers.get_layer(
				resolved_raster->get_age_grid_layer_proxy().get());

		age_grid_layer_usage = gl_age_grid_layer.get_age_grid_layer_usage();
	}

	// The normal map layer usage comes from another layer.
	boost::optional<GPlatesUtils::non_null_intrusive_ptr<NormalMapLayerUsage> > normal_map_layer_usage;
	if (resolved_raster->get_normal_map_layer_proxy())
	{
		// Get the GL layer corresponding to the layer the normal map came from.
		GLLayer &gl_normal_map_layer = d_list_objects->gl_layers.get_layer(
				resolved_raster->get_normal_map_layer_proxy().get());

		normal_map_layer_usage = gl_normal_map_layer.get_normal_map_layer_usage();
	}

	// Get the static polygon reconstructed raster layer usage so we can update its input layer usages.
	const GPlatesUtils::non_null_intrusive_ptr<StaticPolygonReconstructedRasterLayerUsage>
			static_polygon_reconstructed_raster_layer_usage =
					gl_raster_layer.get_static_polygon_reconstructed_raster_layer_usage();

	// If we're reconstructing the raster...
	if (!resolved_raster->get_reconstructed_polygons_layer_proxies().empty())
	{
		std::vector< GPlatesUtils::non_null_intrusive_ptr<ReconstructedStaticPolygonMeshesLayerUsage> >
				reconstructed_polygon_meshes_layer_usages;

		for (const auto &reconstruct_layer_proxy : resolved_raster->get_reconstructed_polygons_layer_proxies())
		{
			// The reconstructed static polygon meshes layer usage comes from another layer.
			// Get the GL layer corresponding to the layer the reconstructed polygons came from.
			GLLayer &gl_reconstructed_polygons_layer = d_list_objects->gl_layers.get_layer(reconstruct_layer_proxy);

			reconstructed_polygon_meshes_layer_usages.push_back(
					gl_reconstructed_polygons_layer.get_reconstructed_static_polygon_meshes_layer_usage());
		}

		// Set/update the layer usage inputs.
		static_polygon_reconstructed_raster_layer_usage->set_reconstructing_layer_inputs(
				gl,
				reconstructed_polygon_meshes_layer_usages,
				age_grid_layer_usage,
				normal_map_layer_usage,
				normal_map_height_field_scale_factor,
				d_list_objects->get_light(gl));
	}
	else // *not* reconstructing raster...
	{
		// Set/update the layer usage inputs.
		static_polygon_reconstructed_raster_layer_usage->set_non_reconstructing_layer_inputs(
				gl,
				d_list_objects->get_multi_resolution_cube_mesh(gl),
				age_grid_layer_usage,
				normal_map_layer_usage,
				normal_map_height_field_scale_factor,
				d_list_objects->get_light(gl));
	}

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
		boost::optional<GLMultiResolutionRasterMapView::non_null_ptr_type> multi_resolution_raster_map_view =
				map_raster_layer_usage->get_multi_resolution_raster_map_view(
						gl,
						// The global map cube mesh shared by all layers...
						d_list_objects->get_multi_resolution_map_cube_mesh(gl, *map_projection.get()),
						resolved_raster->get_reconstruction_time());

		cache_handle_type cache_handle;

		//  Render the map view of raster if successful.
		if (multi_resolution_raster_map_view)
		{
			multi_resolution_raster_map_view.get()->render(gl, view_projection, cache_handle);
		}

		return cache_handle;
	}

	// Next try to render a reconstructed raster in 3D globe view.
	// This also includes a *non* reconstructed raster that combines an age grid and/or normal map.
	boost::optional<GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
			globe_view_reconstructed_raster =
					static_polygon_reconstructed_raster_layer_usage->get_static_polygon_reconstructed_raster(
							gl,
							resolved_raster->get_reconstruction_time());
	if (globe_view_reconstructed_raster)
	{
		//
		// We are rendering a *reconstructed* raster in 3D globe view.
		//
		cache_handle_type cache_handle;
		globe_view_reconstructed_raster.get()->render(gl, view_projection, cache_handle);

		return cache_handle;
	}
	// else drop through and render the *unreconstructed* raster...

	// We have a regular, unreconstructed raster - although it can still be a time-dependent raster.
	// Get the multi-resolution raster.
	boost::optional<GLMultiResolutionRaster::non_null_ptr_type> globe_view_multi_resolution_raster =
			raster_layer_usage->get_multi_resolution_raster(gl);

	cache_handle_type cache_handle;

	// Render the multi-resolution raster, if we have one, in 3D globe view.
	if (globe_view_multi_resolution_raster)
	{
		globe_view_multi_resolution_raster.get()->render(gl, view_projection, cache_handle);
	}

	return cache_handle;
}


GPlatesOpenGL::GLVisualLayers::cache_handle_type
GPlatesOpenGL::GLVisualLayers::render_scalar_field_3d(
		GL &gl,
		const GLViewProjection &view_projection,
		const GPlatesAppLogic::ResolvedScalarField3D::non_null_ptr_to_const_type &resolved_scalar_field,
		const GPlatesViewOperations::ScalarField3DRenderParameters &render_parameters)
{
	PROFILE_FUNC();

	// Get the GL layer corresponding to the layer the scalar field came from.
	GLLayer &gl_scalar_field_layer = d_list_objects->gl_layers.get_layer(
			resolved_scalar_field->get_scalar_field_3d_layer_proxy());

	// Get the scalar field layer usage.
	const GPlatesUtils::non_null_intrusive_ptr<ScalarField3DLayerUsage> scalar_field_layer_usage =
			gl_scalar_field_layer.get_scalar_field_3d_layer_usage();

	// Determine the field colour palette if any is needed.
	boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> colour_palette;
	boost::optional< std::pair<double, double> > colour_palette_value_range;
	switch (render_parameters.get_render_mode())
	{
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE:
		switch (render_parameters.get_isosurface_colour_mode())
		{
		case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_SCALAR:
			colour_palette = GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(
					*render_parameters.get_scalar_colour_palette_parameters().get_colour_palette());
			colour_palette_value_range = render_parameters.get_scalar_colour_palette_parameters().get_palette_range();
			break;
		case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_GRADIENT:
			colour_palette = GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(
					*render_parameters.get_gradient_colour_palette_parameters().get_colour_palette());
			colour_palette_value_range = render_parameters.get_gradient_colour_palette_parameters().get_palette_range();
			break;
		default:
			break;
		}
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS:
		switch (render_parameters.get_cross_section_colour_mode())
		{
		case GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_SCALAR:
			colour_palette = GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(
					*render_parameters.get_scalar_colour_palette_parameters().get_colour_palette());
			colour_palette_value_range = render_parameters.get_scalar_colour_palette_parameters().get_palette_range();
			break;
		case GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_GRADIENT:
			colour_palette = GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(
					*render_parameters.get_gradient_colour_palette_parameters().get_colour_palette());
			colour_palette_value_range = render_parameters.get_gradient_colour_palette_parameters().get_palette_range();
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	// Convert to 'const' colour palette.
	boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type> const_colour_palette;
	if (colour_palette)
	{
		const_colour_palette = colour_palette.get();
	}

	// We have a regular, unreconstructed scalar field - although it can still be time-dependent.
	boost::optional<GLScalarField3D::non_null_ptr_type> scalar_field =
			scalar_field_layer_usage->get_scalar_field_3d(
				gl,
				const_colour_palette,
				colour_palette_value_range,
				d_list_objects->get_light(gl));

	// Render the scalar field if the runtime systems supports scalar field rendering.

	if (!scalar_field)
	{
		return cache_handle_type();
	}

	cache_handle_type cache_handle;

	const GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask &
			surface_polygons_mask = render_parameters.get_surface_polygons_mask();

	// If the surface polygons mask has been enabled then specify any mask geometries to the scalar field.
	boost::optional<GPlatesOpenGL::GLScalarField3D::SurfaceFillMask> surface_fill_mask;
	if (surface_polygons_mask.enable_surface_polygons_mask)
	{
		// Get the surface polygons mask geometries.
		GPlatesOpenGL::GLScalarField3D::surface_polygons_mask_seq_type surface_polygons_mask_geometries;
		resolved_scalar_field->get_scalar_field_3d_layer_proxy()->get_surface_polygons_mask(
				surface_polygons_mask_geometries,
				resolved_scalar_field->get_reconstruction_time());

		// Note: We specify a surface mask even if there are no polygon mask geometries.
		// In this case no scalar field will be rendered (it'll be masked away completely)
		// in which case it's up to the user to provide the surface polygons as a mask
		// (or else disable the mask so it's not applied).
		surface_fill_mask = boost::in_place(
				boost::cref(surface_polygons_mask_geometries),
				surface_polygons_mask.treat_polylines_as_polygons);
	}

	// Render scalar field...
	switch (render_parameters.get_render_mode())
	{
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE:
		if (surface_fill_mask)
		{
			// For iso-surfaces there's also the choice whether to show vertically extruded polygon walls.
			if (surface_polygons_mask.show_polygon_walls)
			{
				surface_fill_mask->show_walls =
						boost::in_place(surface_polygons_mask.only_show_boundary_walls);
			}
		}

		scalar_field.get()->render_iso_surface(
				gl,
				view_projection,
				cache_handle,
				render_parameters.get_isosurface_deviation_window_mode(),
				render_parameters.get_isosurface_colour_mode(),
				render_parameters.get_isovalue_parameters(),
				render_parameters.get_deviation_window_render_options(),
				render_parameters.get_depth_restriction(),
				render_parameters.get_quality_performance(),
				render_parameters.get_shader_test_variables(),
				surface_fill_mask);
		break;

	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS:
		{
			// We can only render cross-sections if we have cross section geometries.
			GPlatesOpenGL::GLScalarField3D::cross_sections_seq_type cross_section_geometries;
			if (resolved_scalar_field->get_scalar_field_3d_layer_proxy()->get_cross_sections(
					cross_section_geometries,
					resolved_scalar_field->get_reconstruction_time()))
			{
				scalar_field.get()->render_cross_sections(
						gl,
						view_projection,
						cache_handle,
						cross_section_geometries,
						render_parameters.get_cross_section_colour_mode(),
						render_parameters.get_depth_restriction(),
						render_parameters.get_shader_test_variables(),
						surface_fill_mask);
			}
		}
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	return cache_handle;
}


void
GPlatesOpenGL::GLVisualLayers::render_filled_polygons(
		GL &gl,
		const GLViewProjection &view_projection,
		const GLFilledPolygonsGlobeView::filled_drawables_type &filled_polygons)
{
	d_list_objects->get_filled_polygons_globe_view(gl)->render(gl, view_projection, filled_polygons);
}


void
GPlatesOpenGL::GLVisualLayers::render_filled_polygons(
		GL &gl,
		const GLViewProjection &view_projection,
		const GLFilledPolygonsMapView::filled_drawables_type &filled_polygons)
{
	d_list_objects->get_filled_polygons_map_view(gl)->render(gl, view_projection, filled_polygons);
}


void
GPlatesOpenGL::GLVisualLayers::handle_layer_about_to_be_removed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
	GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type layer_proxy_handle = layer.get_layer_proxy_handle();

	d_list_objects->gl_layers.remove_layer(layer_proxy_handle);
}


GPlatesOpenGL::GLVisualLayers::ScalarField3DLayerUsage::ScalarField3DLayerUsage(
		const GPlatesAppLogic::ScalarField3DLayerProxy::non_null_ptr_type &scalar_field_layer_proxy) :
	d_scalar_field_layer_proxy(scalar_field_layer_proxy)
{
}


boost::optional<GPlatesOpenGL::GLScalarField3D::non_null_ptr_type>
GPlatesOpenGL::GLVisualLayers::ScalarField3DLayerUsage::get_scalar_field_3d(
		GL &gl,
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type> colour_palette,
		boost::optional< std::pair<double, double> > colour_palette_value_range,
		boost::optional<GLLight::non_null_ptr_type> light)
{
	PROFILE_FUNC();

	if (!d_scalar_field_layer_proxy->get_scalar_field_filename() ||
		!light)
	{
		d_scalar_field = boost::none;

		// There's no scalar field or light source so nothing we can do.
		return boost::none;
	}

	// If we're not up-to-date with respect to the scalar field in the layer proxy...
	// This can happen for time-dependent scalar fields when the time changes.
	if (!d_scalar_field_layer_proxy->get_scalar_field_subject_token().is_observer_up_to_date(
				d_scalar_field_observer_token))
	{
		// Attempt to change the scalar field first.
		// This should succeed if has same field dimensions which is likely the case for a time-dependent field.
		// It's cheaper than rebuilding the scalar field.
		if (d_scalar_field)
		{
			if (!d_scalar_field.get()->change_scalar_field(
					gl,
					GPlatesUtils::make_qstring(*d_scalar_field_layer_proxy->get_scalar_field_filename())))
			{
				// Change scalar field was unsuccessful, so rebuild.
				d_scalar_field = boost::none;
			}
		}

		// We have taken measures to be up-to-date with respect to the scalar field in the layer proxy.
		d_scalar_field_layer_proxy->get_scalar_field_subject_token().update_observer(
				d_scalar_field_observer_token);
	}

	// If we're not up-to-date with respect to the scalar field feature in the layer proxy then rebuild.
	if (!d_scalar_field_layer_proxy->get_scalar_field_feature_subject_token().is_observer_up_to_date(
			d_scalar_field_feature_observer_token))
	{
		d_scalar_field = boost::none;

		// We have taken measures to be up-to-date with respect to the scalar field feature in the layer proxy.
		d_scalar_field_layer_proxy->get_scalar_field_feature_subject_token().update_observer(
				d_scalar_field_feature_observer_token);
	}

	// Determine whether the colour palette needs loading into the scalar field.
	bool load_colour_palette = false;
	if (colour_palette && colour_palette_value_range)
	{
		if (colour_palette != d_colour_palette)
		{
			// Colour palette has changed.
			d_colour_palette = colour_palette;
			d_colour_palette_value_range = colour_palette_value_range;
			load_colour_palette = true;
		}

		if (!d_scalar_field)
		{
			// Always load the colour palette after creating a new scalar field.
			load_colour_palette = true;
		}
	}

	// Rebuild the scalar field if necessary.
	if (!d_scalar_field)
	{
		//qDebug() << "Rebuilding GLScalarField3D.";

		const GLScalarField3D::non_null_ptr_type scalar_field =
				GLScalarField3D::create(
						gl,
						GPlatesUtils::make_qstring(*d_scalar_field_layer_proxy->get_scalar_field_filename()),
						light.get());

		d_scalar_field = scalar_field;
	}

	// Update the colour palette if necessary.
	if (load_colour_palette)
	{
		d_scalar_field.get()->set_colour_palette(
				gl,
				d_colour_palette.get(),
				d_colour_palette_value_range.get());
	}

	return d_scalar_field.get();
}


bool
GPlatesOpenGL::GLVisualLayers::ScalarField3DLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return layer_proxy_handle == d_scalar_field_layer_proxy;
}


GPlatesOpenGL::GLVisualLayers::RasterLayerUsage::RasterLayerUsage(
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &raster_layer_proxy) :
	d_raster_layer_proxy(raster_layer_proxy),
	d_raster_colour_palette_dirty(true),
	d_raster_modulate_colour(GPlatesGui::Colour::get_white()),
	d_raster_modulate_colour_dirty(true)
{
}


void
GPlatesOpenGL::GLVisualLayers::RasterLayerUsage::set_raster_colour_palette(
		GL &gl,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette)
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
GPlatesOpenGL::GLVisualLayers::RasterLayerUsage::set_raster_modulate_colour(
		GL &gl,
		const GPlatesGui::Colour &raster_modulate_colour)
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
GPlatesOpenGL::GLVisualLayers::RasterLayerUsage::get_multi_resolution_raster(
		GL &gl)
{
	PROFILE_FUNC();

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
					gl,
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

		//qDebug() << "Rebuilding GLVisualRasterSource.";

		d_visual_raster_source = GLVisualRasterSource::create(
				gl,
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
		d_visual_raster_source.get()->change_modulate_colour(gl, d_raster_modulate_colour);
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
		//qDebug() << "Rebuilding GLMultiResolutionRaster.";

		const GLMultiResolutionRaster::non_null_ptr_type multi_resolution_raster =
				GLMultiResolutionRaster::create(
						gl,
						d_raster_layer_proxy->get_georeferencing().get(),
						d_raster_layer_proxy->get_coordinate_transformation(),
						d_visual_raster_source.get());

		d_multi_resolution_raster = multi_resolution_raster;
	}

	return d_multi_resolution_raster.get();
}


bool
GPlatesOpenGL::GLVisualLayers::RasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return layer_proxy_handle == d_raster_layer_proxy;
}


GPlatesOpenGL::GLVisualLayers::CubeRasterLayerUsage::CubeRasterLayerUsage(
		const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> &raster_layer_usage) :
	d_raster_layer_usage(raster_layer_usage)
{
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesOpenGL::GLVisualLayers::CubeRasterLayerUsage::get_multi_resolution_cube_raster(
		GL &gl)
{
	PROFILE_FUNC();

	// Get the source multi-resolution raster.
	const boost::optional<GLMultiResolutionRaster::non_null_ptr_type> multi_resolution_raster =
			d_raster_layer_usage->get_multi_resolution_raster(gl);

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

		//qDebug() << "Rebuilding GLMultiResolutionCubeRaster.";

		// Attempt to create the multi-resolution cube raster.
		d_multi_resolution_cube_raster =
				GLMultiResolutionCubeRaster::create(
						gl,
						d_multi_resolution_raster.get());
	}

	return d_multi_resolution_cube_raster;
}


bool
GPlatesOpenGL::GLVisualLayers::CubeRasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return d_raster_layer_usage->is_required_direct_or_indirect_dependency(layer_proxy_handle);
}


GPlatesOpenGL::GLVisualLayers::AgeGridLayerUsage::AgeGridLayerUsage(
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &age_grid_raster_layer_proxy) :
	d_age_grid_raster_layer_proxy(age_grid_raster_layer_proxy)
{
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesOpenGL::GLVisualLayers::AgeGridLayerUsage::get_multi_resolution_age_grid_mask(
		GL &gl)
{
	return d_age_grid_raster_layer_proxy->get_multi_resolution_age_grid_mask(gl);
}


bool
GPlatesOpenGL::GLVisualLayers::AgeGridLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return layer_proxy_handle == d_age_grid_raster_layer_proxy;
}


GPlatesOpenGL::GLVisualLayers::NormalMapLayerUsage::NormalMapLayerUsage(
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &normal_map_raster_layer_proxy) :
	d_raster_layer_proxy(normal_map_raster_layer_proxy)
{
}


GPlatesOpenGL::GLVisualLayers::NormalMapLayerUsage::NormalRaster::shared_ptr_type
GPlatesOpenGL::GLVisualLayers::NormalMapLayerUsage::get_normal_map(
		GL &gl,
		float height_field_scale_factor)
{
	// First go through the sequence of mapped normal rasters and remove any expired entries.
	// This is to prevent the accumulation of expired entries over time.
	// The entries expire when client no longer reference a (shared) normal raster.
	normal_raster_map_type::iterator normal_raster_map_iter = d_normal_raster_map.begin();
	while (normal_raster_map_iter != d_normal_raster_map.end())
	{
		normal_raster_map_type::iterator current_normal_raster_map_iter = normal_raster_map_iter;
		++normal_raster_map_iter;

		if (current_normal_raster_map_iter->second.expired())
		{
			d_normal_raster_map.erase(current_normal_raster_map_iter);
		}
	}

	// Create a new normal map if there is not already one associated with the height field scale factor.
	// Note that due to the above clearing away of expired entries we should have no expired entries left.
	normal_raster_map_iter = d_normal_raster_map.find(GPlatesMaths::real_t(height_field_scale_factor));
	if (normal_raster_map_iter == d_normal_raster_map.end())
	{
		NormalRaster::shared_ptr_type normal_raster =
				NormalRaster::create(d_raster_layer_proxy, height_field_scale_factor);

		d_normal_raster_map[GPlatesMaths::real_t(height_field_scale_factor)] = normal_raster;

		return normal_raster;
	}

	// Convert weak_ptr to shared_ptr.
	NormalRaster::shared_ptr_type normal_raster(normal_raster_map_iter->second);

	return normal_raster;
}


bool
GPlatesOpenGL::GLVisualLayers::NormalMapLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return layer_proxy_handle == d_raster_layer_proxy;
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesOpenGL::GLVisualLayers::NormalMapLayerUsage::NormalRaster::get_normal_map(
		GL &gl)
{
	if (!d_raster_layer_proxy->get_proxied_raster())
	{
		d_normal_map_raster_source = boost::none;

		// There's no proxied raster so nothing we can do.
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
		// If we have a normal map raster source then attempt to change the raster first
		// since it's cheaper than rebuilding the multi-resolution raster.
		if (d_normal_map_raster_source)
		{
			if (!d_normal_map_raster_source.get()->change_raster(
					gl,
					d_raster_layer_proxy->get_proxied_raster().get(),
					d_height_field_scale_factor))
			{
				// Change raster was unsuccessful, so rebuild visual raster source.
				d_normal_map_raster_source = boost::none;
			}
		}

		// We have taken measures to be up-to-date with respect to the proxied raster in the raster layer proxy.
		d_raster_layer_proxy->get_proxied_raster_subject_token().update_observer(
				d_proxied_raster_observer_token);
	}

	// Rebuild the normal map raster source if necessary.
	if (!d_normal_map_raster_source)
	{
		// NOTE: We also invalidate the multi-resolution raster since it must link
		// to the normal map raster source and hence must also be rebuilt.
		d_multi_resolution_raster = boost::none;

		//qDebug() << "Rebuilding GLNormalMapSource.";

		d_normal_map_raster_source = GLNormalMapSource::create(
				gl,
				d_raster_layer_proxy->get_proxied_raster().get(),
				GLNormalMapSource::DEFAULT_TILE_TEXEL_DIMENSION,
				d_height_field_scale_factor);
		if (!d_normal_map_raster_source)
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

		// We have taken measures to be up-to-date with respect to the raster feature in the raster layer proxy.
		d_raster_layer_proxy->get_raster_feature_subject_token().update_observer(
				d_raster_feature_observer_token);
	}

	// Rebuild the multi-resolution raster if necessary.
	if (!d_multi_resolution_raster)
	{
		// We need to rebuild the multi-resolution cube raster.
		d_multi_resolution_cube_raster = boost::none;

		//qDebug() << "Rebuilding GLMultiResolutionRaster for normal map.";

		const GLMultiResolutionRaster::non_null_ptr_type multi_resolution_raster =
				GLMultiResolutionRaster::create(
						gl,
						d_raster_layer_proxy->get_georeferencing().get(),
						d_raster_layer_proxy->get_coordinate_transformation(),
						d_normal_map_raster_source.get());

		d_multi_resolution_raster = multi_resolution_raster;
	}

	if (!d_multi_resolution_cube_raster)
	{
		if (!d_multi_resolution_raster)
		{
			// There's no multi-resolution raster so nothing we can do.
			return boost::none;
		}

		//qDebug() << "Rebuilding GLMultiResolutionCubeRaster for normal map.";

		// Attempt to create the multi-resolution cube raster.
		d_multi_resolution_cube_raster =
				GLMultiResolutionCubeRaster::create(
						gl,
						d_multi_resolution_raster.get());
	}

	return d_multi_resolution_cube_raster.get();
}


GPlatesOpenGL::GLVisualLayers::ReconstructedStaticPolygonMeshesLayerUsage::ReconstructedStaticPolygonMeshesLayerUsage(
		const GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type &reconstructed_static_polygon_meshes_layer_proxy) :
	d_reconstructed_static_polygon_meshes_layer_proxy(reconstructed_static_polygon_meshes_layer_proxy)
{
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type
GPlatesOpenGL::GLVisualLayers::ReconstructedStaticPolygonMeshesLayerUsage::get_reconstructed_static_polygon_meshes(
		GL &gl,
		bool reconstructing_with_age_grid,
		const double &reconstruction_time)
{
	return d_reconstructed_static_polygon_meshes_layer_proxy->get_reconstructed_static_polygon_meshes(
			gl,
			reconstructing_with_age_grid,
			reconstruction_time);
}


bool
GPlatesOpenGL::GLVisualLayers::ReconstructedStaticPolygonMeshesLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	return layer_proxy_handle == d_reconstructed_static_polygon_meshes_layer_proxy;
}


GPlatesOpenGL::GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage::StaticPolygonReconstructedRasterLayerUsage(
		const GPlatesUtils::non_null_intrusive_ptr<CubeRasterLayerUsage> &cube_raster_layer_usage) :
	d_cube_raster_layer_usage(cube_raster_layer_usage)
{
}


void
GPlatesOpenGL::GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage::set_reconstructing_layer_inputs(
		GL &gl,
		const std::vector< GPlatesUtils::non_null_intrusive_ptr<ReconstructedStaticPolygonMeshesLayerUsage> > &
				reconstructed_polygon_meshes_layer_usages,
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > &age_grid_layer_usage,
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<NormalMapLayerUsage> > &normal_map_layer_usage,
		float height_field_scale_factor,
		boost::optional<GLLight::non_null_ptr_type> light)
{
	// Only used when *not* reconstructing raster.
	d_multi_resolution_cube_mesh = boost::none;

	// See if we've switched layer usages (this includes switching over from *not* reconstructing the raster).
	if (d_reconstructed_polygon_meshes_layer_usages != reconstructed_polygon_meshes_layer_usages)
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		d_reconstructed_polygon_meshes.clear();
		d_reconstructed_polygon_meshes_layer_usages = reconstructed_polygon_meshes_layer_usages;
	}

	set_other_inputs(gl, age_grid_layer_usage, normal_map_layer_usage, height_field_scale_factor, light);
}


void
GPlatesOpenGL::GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage::set_non_reconstructing_layer_inputs(
		GL &gl,
		const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh,
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > &age_grid_layer_usage,
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<NormalMapLayerUsage> > &normal_map_layer_usage,
		float height_field_scale_factor,
		boost::optional<GLLight::non_null_ptr_type> light)
{
	// Only used when reconstructing raster.
	d_reconstructed_polygon_meshes_layer_usages.clear();

	// See if we've switched over from reconstructing a raster to *not* reconstructing it.
	if (!d_multi_resolution_cube_mesh)
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		d_multi_resolution_cube_mesh = multi_resolution_cube_mesh;
	}

	set_other_inputs(gl, age_grid_layer_usage, normal_map_layer_usage, height_field_scale_factor, light);
}


void
GPlatesOpenGL::GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage::set_other_inputs(
		GL &gl,
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > &age_grid_layer_usage,
		const boost::optional<GPlatesUtils::non_null_intrusive_ptr<NormalMapLayerUsage> > &normal_map_layer_usage,
		float height_field_scale_factor,
		boost::optional<GLLight::non_null_ptr_type> light)
{
	if (d_age_grid_layer_usage != age_grid_layer_usage)
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		d_age_grid_mask_cube_raster = boost::none;
		d_age_grid_layer_usage = age_grid_layer_usage;
	}

	if (d_normal_map_layer_usage != normal_map_layer_usage)
	{
		// Then we need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;

		d_normal_map_cube_raster = boost::none;
		d_normal_map_normal_raster = boost::none;
		d_normal_map_layer_usage = normal_map_layer_usage;
	}

	// If we have a normal map layer then get the normal map associated with the
	// specified height field scale factor.
	// If we already have a normal map (from the same normal map layer) with the same
	// scale factor (or some other layer does) we this will just return the existing (shared)
	// normal map for that scale factor.
	if (d_normal_map_layer_usage)
	{
		d_normal_map_normal_raster =
				d_normal_map_layer_usage.get()->get_normal_map(gl, height_field_scale_factor);
	}

	d_light = light;
}


boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
GPlatesOpenGL::GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage::get_static_polygon_reconstructed_raster(
		GL &gl,
		const double &reconstruction_time)
{
	PROFILE_FUNC();

	// If:
	//  (1) we don't have reconstructed polygon meshes, and
	//  (2) we don't have an age grid, and
	//  (3) we don't have a normal map, and
	//  (4) surface lighting of rasters (with or without normal maps) is not enabled
	// ...then a regular "unreconstructed" raster should be used instead (it's faster and uses less memory).
	// Note that we don't require reconstructed polygon meshes to continue past this point.
	// Also note that in (4) we delegate all lighting tasks to "reconstructed" raster even if we're
	// not reconstructing a raster - this makes the implementation much simpler since we don't have
	// to worry about issues related to incorrectly applying lighting twice to reconstructed rasters
	// (both at the unreconstructed stage and reconstructed stage) - and besides, we're already
	// delegating all "normal map" lighting to "reconstructed" raster anyway.
	if (d_reconstructed_polygon_meshes_layer_usages.empty() &&
		!d_age_grid_layer_usage &&
		!d_normal_map_layer_usage &&
		!(d_light && d_light.get()->get_scene_lighting_parameters().is_lighting_enabled(
				GPlatesGui::SceneLightingParameters::LIGHTING_RASTER)))
	{
		return boost::none;
	}

	// Get the source multi-resolution cube raster.
	const boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> multi_resolution_cube_raster =
			d_cube_raster_layer_usage->get_multi_resolution_cube_raster(gl);

	// If source multi-resolution cube raster is a different object...
	if (d_multi_resolution_cube_raster != multi_resolution_cube_raster)
	{
		d_multi_resolution_cube_raster = multi_resolution_cube_raster;

		// We need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;
	}

	if (!d_multi_resolution_cube_raster)
	{
		// There's no multi-resolution cube raster so nothing we can do.
		return boost::none;
	}

	// Get the reconstructed polygon meshes (if any).
	std::vector<GLReconstructedStaticPolygonMeshes::non_null_ptr_type> reconstructed_polygon_meshes;
	if (!d_reconstructed_polygon_meshes_layer_usages.empty())
	{
		for (const auto &reconstructed_polygon_meshes_layer_usage : d_reconstructed_polygon_meshes_layer_usages)
		{
			GLReconstructedStaticPolygonMeshes::non_null_ptr_type reconstructed_polygon_meshes_in_layer =
					reconstructed_polygon_meshes_layer_usage->get_reconstructed_static_polygon_meshes(
							gl,
							static_cast<bool>(d_age_grid_layer_usage)/*reconstructing_with_age_grid*/,
							reconstruction_time);

			reconstructed_polygon_meshes.push_back(reconstructed_polygon_meshes_in_layer);
		}
	}

	// If reconstructed polygon meshes is a different object...
	if (d_reconstructed_polygon_meshes != reconstructed_polygon_meshes)
	{
		d_reconstructed_polygon_meshes = reconstructed_polygon_meshes;

		// We need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;
	}

	// If we are using an age grid to assist reconstruction...
	boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> age_grid_mask_cube_raster;
	if (d_age_grid_layer_usage)
	{
		// Get the age grid mask.
		age_grid_mask_cube_raster = d_age_grid_layer_usage.get()->get_multi_resolution_age_grid_mask(gl);

		if (!age_grid_mask_cube_raster)
		{
			qWarning() << "GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage::get_static_polygon_reconstructed_raster: "
				"Failed to obtain age grid.";
		}
	}

	// If the age grid cube rasters are different objects...
	if (d_age_grid_mask_cube_raster != age_grid_mask_cube_raster)
	{
		d_age_grid_mask_cube_raster = age_grid_mask_cube_raster;

		// We need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;
	}

	// If we are using a normal map to enhance surface lighting detail and normal maps are supported...
	boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> normal_map_cube_raster;
	if (d_normal_map_layer_usage)
	{
		// Get the normal map.
		normal_map_cube_raster = d_normal_map_normal_raster.get()->get_normal_map(gl);

		if (!normal_map_cube_raster)
		{
			qWarning() << "GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage::get_static_polygon_reconstructed_raster: "
				"Failed to obtain normal map.";
		}
	}

	// If the normal map cube rasters are different objects...
	if (d_normal_map_cube_raster != normal_map_cube_raster)
	{
		d_normal_map_cube_raster = normal_map_cube_raster;

		// We need to rebuild the reconstructed raster.
		d_reconstructed_raster = boost::none;
	}

	if (!d_reconstructed_raster)
	{
		//qDebug() << "Rebuilding GLMultiResolutionStaticPolygonReconstructedRaster "
		//		<< (d_age_grid_mask_cube_raster ? "with" : "without") << " age grid and "
		//		<< (d_normal_map_cube_raster ? "with" : "without") << " normal map.";

		// Create a reconstructed raster.
		if (!d_reconstructed_polygon_meshes.empty())
		{
			d_reconstructed_raster =
					GLMultiResolutionStaticPolygonReconstructedRaster::create(
							gl,
							reconstruction_time,
							d_multi_resolution_cube_raster.get(),
							d_reconstructed_polygon_meshes,
							d_age_grid_mask_cube_raster,
							d_normal_map_cube_raster,
							d_light);
		}
		else // *not* reconstructed ...
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_multi_resolution_cube_mesh,
					GPLATES_ASSERTION_SOURCE);

			d_reconstructed_raster =
					GLMultiResolutionStaticPolygonReconstructedRaster::create(
							gl,
							reconstruction_time,
							d_multi_resolution_cube_raster.get(),
							d_multi_resolution_cube_mesh.get(),
							d_age_grid_mask_cube_raster,
							d_normal_map_cube_raster,
							d_light);
		}
	}

	// Notify the reconstructed raster of the current reconstruction time.
	d_reconstructed_raster.get()->update(reconstruction_time);

	return d_reconstructed_raster;
}


bool
GPlatesOpenGL::GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	// We require the source cube raster, but everything else is optional (including the
	// reconstructed polygon meshes).
	return d_cube_raster_layer_usage->is_required_direct_or_indirect_dependency(layer_proxy_handle);
}


void
GPlatesOpenGL::GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage::removing_layer(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle)
{
	// If we're using reconstructed polygons and it depends on the layer about to be removed then stop using it.
	if (!d_reconstructed_polygon_meshes_layer_usages.empty())
	{
		for (const auto &reconstructed_polygon_meshes_layer_usage : d_reconstructed_polygon_meshes_layer_usages)
		{
			if (reconstructed_polygon_meshes_layer_usage->is_required_direct_or_indirect_dependency(layer_proxy_handle))
			{
				// Stop using the reconstructed polygon meshs layer usages.
				d_reconstructed_polygon_meshes_layer_usages.clear();
				d_reconstructed_polygon_meshes.clear();

				// We'll need to rebuild our reconstructed raster.
				d_reconstructed_raster = boost::none;

				break;
			}
		}
	}

	// If we're using an age grid and it depends on the layer about to be removed then stop using it.
	if (d_age_grid_layer_usage &&
		d_age_grid_layer_usage.get()->is_required_direct_or_indirect_dependency(layer_proxy_handle))
	{
		// Stop using the age grid layer usage.
		d_age_grid_layer_usage = boost::none;
		d_age_grid_mask_cube_raster = boost::none;

		// We'll need to rebuild our reconstructed raster.
		d_reconstructed_raster = boost::none;
	}

	// If we're using a normal map and it depends on the layer about to be removed then stop using it.
	if (d_normal_map_layer_usage &&
		d_normal_map_layer_usage.get()->is_required_direct_or_indirect_dependency(layer_proxy_handle))
	{
		// Stop using the normal map layer usage.
		d_normal_map_layer_usage = boost::none;
		d_normal_map_cube_raster = boost::none;
		d_normal_map_normal_raster = boost::none;

		// We'll need to rebuild our reconstructed raster.
		d_reconstructed_raster = boost::none;
	}
}


GPlatesOpenGL::GLVisualLayers::MapRasterLayerUsage::MapRasterLayerUsage(
		const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> &raster_layer_usage,
		const GPlatesUtils::non_null_intrusive_ptr<StaticPolygonReconstructedRasterLayerUsage> &reconstructed_raster_layer_usage) :
	d_raster_layer_usage(raster_layer_usage),
	d_reconstructed_raster_layer_usage(reconstructed_raster_layer_usage)
{
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRasterMapView::non_null_ptr_type>
GPlatesOpenGL::GLVisualLayers::MapRasterLayerUsage::get_multi_resolution_raster_map_view(
		GL &gl,
		const GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type &multi_resolution_map_cube_mesh,
		const double &reconstruction_time)
{
	PROFILE_FUNC();

	// Try getting the reconstructed raster.
	const boost::optional<GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
			reconstructed_raster =
					d_reconstructed_raster_layer_usage->get_static_polygon_reconstructed_raster(
							gl,
							reconstruction_time);

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
			// NOTE: We create our own cube reconstructed raster because the world transform gets
			// set on it according to the central meridian of the map projection.
			// This means the input cube raster will get re-oriented and hence can no longer
			// be shared with the globe (non-map) view where the central meridian is always zero.
			// Actually, in this, case it wouldn't affect the globe view anyway since it doesn't
			// make use (or need) a cube reconstructed raster.
			GLMultiResolutionCubeRasterInterface::non_null_ptr_type multi_resolution_cube_reconstructed_raster =
					GLMultiResolutionCubeReconstructedRaster::create(
							gl,
							d_reconstructed_raster.get());

			//qDebug() << "Rebuilding GLMultiResolutionRasterMapView for reconstructed raster.";

			// Attempt to create the multi-resolution raster map view.
			d_multi_resolution_raster_map_view =
					GLMultiResolutionRasterMapView::create(
							gl,
							multi_resolution_cube_reconstructed_raster.get(),
							multi_resolution_map_cube_mesh.get());
		}

		return d_multi_resolution_raster_map_view;
	}

	// Try getting the regular (unreconstructed) raster.
	const boost::optional<GLMultiResolutionRaster::non_null_ptr_type> raster =
			d_raster_layer_usage->get_multi_resolution_raster(gl);

	// If (unreconstructed) raster is a different object...
	if (d_raster != raster)
	{
		d_raster = raster;

		// We need to rebuild the raster map view.
		d_multi_resolution_raster_map_view = boost::none;
	}

	// If we have a (unreconstructed) raster then fallback to that.
	if (d_raster)
	{
		if (!d_multi_resolution_raster_map_view)
		{
			// NOTE: We create our own cube raster because the world transform gets set on it
			// according to the central meridian of the map projection.
			// This means the input cube raster will get re-oriented and hence can no longer
			// be shared with the globe (non-map) view where the central meridian is always zero.
			GLMultiResolutionCubeRasterInterface::non_null_ptr_type multi_resolution_cube_raster =
					GLMultiResolutionCubeRaster::create(
							gl,
							d_raster.get());

			//qDebug() << "Rebuilding GLMultiResolutionRasterMapView for raster.";

			// Attempt to create the multi-resolution raster map view.
			d_multi_resolution_raster_map_view =
					GLMultiResolutionRasterMapView::create(
							gl,
							multi_resolution_cube_raster.get(),
							multi_resolution_map_cube_mesh.get());
		}
	}

	return d_multi_resolution_raster_map_view;
}


bool
GPlatesOpenGL::GLVisualLayers::MapRasterLayerUsage::is_required_direct_or_indirect_dependency(
		const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle) const
{
	// We require the source raster and the reconstructed source raster.
	// They both essentially refer to the same source raster anyway.
	return d_raster_layer_usage->is_required_direct_or_indirect_dependency(layer_proxy_handle) ||
		d_reconstructed_raster_layer_usage->is_required_direct_or_indirect_dependency(layer_proxy_handle);
}


GPlatesOpenGL::GLVisualLayers::GLLayer::GLLayer(
		const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy) :
	d_layer_proxy(layer_proxy),
	d_layer_usages(LayerUsage::NUM_TYPES)
{
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesOpenGL::GLVisualLayers::ScalarField3DLayerUsage>
GPlatesOpenGL::GLVisualLayers::GLLayer::get_scalar_field_3d_layer_usage()
{
	const LayerUsage::Type layer_usage_type = LayerUsage::SCALAR_FIELD_3D;

	if (!d_layer_usages[layer_usage_type])
	{
		// This will throw an exception (or abort in debug mode) if the dynamic cast fails
		// but that's because it's a program error if it fails.
		const GPlatesAppLogic::ScalarField3DLayerProxy::non_null_ptr_type scalar_field_layer_proxy =
				GPlatesUtils::dynamic_pointer_cast<GPlatesAppLogic::ScalarField3DLayerProxy>(d_layer_proxy);

		// Create a new ScalarField3DLayerUsage object.
		d_layer_usages[layer_usage_type] = 
				GPlatesUtils::non_null_intrusive_ptr<ScalarField3DLayerUsage>(
						new ScalarField3DLayerUsage(scalar_field_layer_proxy));
	}

	return GPlatesUtils::dynamic_pointer_cast<ScalarField3DLayerUsage>(d_layer_usages[layer_usage_type].get());
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesOpenGL::GLVisualLayers::RasterLayerUsage>
GPlatesOpenGL::GLVisualLayers::GLLayer::get_raster_layer_usage()
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


GPlatesUtils::non_null_intrusive_ptr<GPlatesOpenGL::GLVisualLayers::CubeRasterLayerUsage>
GPlatesOpenGL::GLVisualLayers::GLLayer::get_cube_raster_layer_usage()
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


GPlatesUtils::non_null_intrusive_ptr<GPlatesOpenGL::GLVisualLayers::AgeGridLayerUsage>
GPlatesOpenGL::GLVisualLayers::GLLayer::get_age_grid_layer_usage()
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


GPlatesUtils::non_null_intrusive_ptr<GPlatesOpenGL::GLVisualLayers::NormalMapLayerUsage>
GPlatesOpenGL::GLVisualLayers::GLLayer::get_normal_map_layer_usage()
{
	const LayerUsage::Type layer_usage_type = LayerUsage::NORMAL_MAP;

	if (!d_layer_usages[layer_usage_type])
	{
		// This will throw an exception (or abort in debug mode) if the dynamic cast fails
		// but that's because it's a program error if it fails.
		const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type normal_map_layer_proxy =
				GPlatesUtils::dynamic_pointer_cast<GPlatesAppLogic::RasterLayerProxy>(d_layer_proxy);

		// Create a new RasterLayerUsage object.
		d_layer_usages[layer_usage_type] = 
				GPlatesUtils::non_null_intrusive_ptr<NormalMapLayerUsage>(
						new NormalMapLayerUsage(
								normal_map_layer_proxy));
	}

	return GPlatesUtils::dynamic_pointer_cast<NormalMapLayerUsage>(d_layer_usages[layer_usage_type].get());
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesOpenGL::GLVisualLayers::ReconstructedStaticPolygonMeshesLayerUsage>
GPlatesOpenGL::GLVisualLayers::GLLayer::get_reconstructed_static_polygon_meshes_layer_usage()
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


GPlatesUtils::non_null_intrusive_ptr<GPlatesOpenGL::GLVisualLayers::StaticPolygonReconstructedRasterLayerUsage>
GPlatesOpenGL::GLVisualLayers::GLLayer::get_static_polygon_reconstructed_raster_layer_usage()
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


GPlatesUtils::non_null_intrusive_ptr<GPlatesOpenGL::GLVisualLayers::MapRasterLayerUsage>
GPlatesOpenGL::GLVisualLayers::GLLayer::get_map_raster_layer_usage()
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
GPlatesOpenGL::GLVisualLayers::GLLayer::remove_references_to_layer(
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
	for (
			// NOTE: This must be a reference and not a copy...
			auto &layer_usage_opt : d_layer_usages)
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


GPlatesOpenGL::GLVisualLayers::GLLayer &
GPlatesOpenGL::GLVisualLayers::GLLayers::get_layer(
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
GPlatesOpenGL::GLVisualLayers::GLLayers::remove_layer(
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
	for (const auto &gl_layer_map_entry : d_layer_map)
	{
		const GLLayer::non_null_ptr_type &gl_layer = gl_layer_map_entry.second;
		gl_layer->remove_references_to_layer(layer_proxy_to_be_removed);
	}
}


GPlatesOpenGL::GLVisualLayers::ListObjects::ListObjects(
		const boost::shared_ptr<GLContext::SharedState> &opengl_shared_state_input,
		const NonListObjects &non_list_objects) :
	opengl_shared_state(opengl_shared_state_input),
	d_non_list_objects(non_list_objects)
{
}


GPlatesOpenGL::GLMultiResolutionCubeMesh::non_null_ptr_to_const_type
GPlatesOpenGL::GLVisualLayers::ListObjects::get_multi_resolution_cube_mesh(
		GL &gl) const
{
	if (!d_multi_resolution_cube_mesh)
	{
		d_multi_resolution_cube_mesh =
				GLMultiResolutionCubeMesh::non_null_ptr_to_const_type(
						GLMultiResolutionCubeMesh::create(gl));
	}

	return d_multi_resolution_cube_mesh.get();
}


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type
GPlatesOpenGL::GLVisualLayers::ListObjects::get_multi_resolution_map_cube_mesh(
		GL &gl,
		const GPlatesGui::MapProjection &map_projection) const
{
	if (!d_multi_resolution_map_cube_mesh)
	{
		d_multi_resolution_map_cube_mesh =
				GLMultiResolutionMapCubeMesh::non_null_ptr_type(
						GLMultiResolutionMapCubeMesh::create(
								gl,
								map_projection));
	}

	// Update the map projection if it's changed.
	d_multi_resolution_map_cube_mesh.get()->update_map_projection(gl, map_projection);

	return d_multi_resolution_map_cube_mesh.get();
}


GPlatesOpenGL::GLFilledPolygonsGlobeView::non_null_ptr_type
GPlatesOpenGL::GLVisualLayers::ListObjects::get_filled_polygons_globe_view(
		GL &gl) const
{
	if (!d_filled_polygons_globe_view)
	{
		d_filled_polygons_globe_view =
				GLFilledPolygonsGlobeView::create(
						gl,
						get_multi_resolution_cube_mesh(gl),
						get_light(gl));
	}

	return d_filled_polygons_globe_view.get();
}


GPlatesOpenGL::GLFilledPolygonsMapView::non_null_ptr_type
GPlatesOpenGL::GLVisualLayers::ListObjects::get_filled_polygons_map_view(
		GL &gl) const
{
	if (!d_filled_polygons_map_view)
	{
		d_filled_polygons_map_view = GLFilledPolygonsMapView::create(gl);
	}

	return d_filled_polygons_map_view.get();
}


GPlatesOpenGL::GLLight::non_null_ptr_type
GPlatesOpenGL::GLVisualLayers::ListObjects::get_light(
		GL &gl) const
{
	// Create light if first time called.
	if (!d_light)
	{
		d_light = GLLight::create(gl);
	}

	return d_light.get();
}
