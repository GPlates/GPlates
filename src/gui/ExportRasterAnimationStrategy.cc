/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <cmath>
#include <exception>
#include <limits>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>
#include <QString>

#include "ExportRasterAnimationStrategy.h"

#include "AnimationController.h"
#include "ExportAnimationContext.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/LayerProxyUtils.h"
#include "app-logic/LayerTaskType.h"
#include "app-logic/RasterLayerProxy.h"
#include "app-logic/ReconstructGraph.h"

#include "file-io/RasterWriter.h"

#include "global/GPlatesAssert.h"
#include "global/LogException.h"

#include "gui/Colour.h"
#include "gui/MapProjection.h"
#include "gui/RasterColourPalette.h"

#include "maths/MathsUtils.h"

#include "opengl/GLBuffer.h"
#include "opengl/GLCapabilities.h"
#include "opengl/GLContext.h"
#include "opengl/GLFrameBufferObject.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLMultiResolutionCubeRaster.h"
#include "opengl/GLMultiResolutionMapCubeMesh.h"
#include "opengl/GLMultiResolutionRasterMapView.h"
#include "opengl/GLPixelBuffer.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLRenderTarget.h"
#include "opengl/GLTexture.h"
#include "opengl/GLTileRender.h"
#include "opengl/GLVisualLayers.h"

#include "presentation/RasterVisualLayerParams.h"
#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayers.h"

#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/XsString.h"

#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/ViewportWindow.h"


namespace
{
	QString
	substitute_placeholder(
			const QString &output_filebasename,
			const QString &placeholder,
			const QString &placeholder_replacement)
	{
		return QString(output_filebasename).replace(placeholder, placeholder_replacement);
	}


	QString
	calculate_output_basename(
			const QString &output_filename_prefix,
			const QString &layer_name)
	{
		const QString output_basename = substitute_placeholder(
				output_filename_prefix,
				GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
				layer_name);

		return output_basename;
	}


	/**
	 * Typedef for sequence of raster visual layer.
	 */
	typedef std::vector< boost::shared_ptr<const GPlatesPresentation::VisualLayer> > raster_visual_layer_seq_type;


	/**
	 * Information about a colour raster.
	 */
	struct ColourRaster
	{

		ColourRaster(
				const QString &layer_name_,
				const GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type &resolved_raster_,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette_,
				const GPlatesGui::Colour &raster_modulate_colour_,
				float surface_relief_scale_) :
			layer_name(layer_name_),
			resolved_raster(resolved_raster_),
			raster_colour_palette(raster_colour_palette_),
			raster_modulate_colour(raster_modulate_colour_),
			surface_relief_scale(surface_relief_scale_)
		{  }

		// The name of the raster (visual) layer.
		QString layer_name;

		// The information needed to render a raster as colours (using GLVisualLayers to render).

		GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type resolved_raster;
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette;
		GPlatesGui::Colour raster_modulate_colour;
		float surface_relief_scale;
	};

	/**
	 * Typedef for a sequence of (reconstructed) colour rasters.
	 */
	typedef std::vector<ColourRaster> colour_raster_seq_type;


	/**
	 * Information about a numerical raster and its bands.
	 */
	struct NumericalRaster
	{
		struct Band
		{
			explicit
			Band(
					const GPlatesPropertyValues::TextContent &name_) :
				name(name_)
			{  }

			GPlatesPropertyValues::TextContent name;
		};

		NumericalRaster(
				const QString &layer_name_,
				const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &layer_proxy_,
				const std::vector<Band> &numerical_bands_) :
			layer_name(layer_name_),
			layer_proxy(layer_proxy_),
			numerical_bands(numerical_bands_)
		{  }

		// The name of the raster (visual) layer.
		QString layer_name;

		// The layer proxy to get the raster band data from.
		//
		// NOTE: The raster band data *must* be obtained just before rendering because different
		// raster layers can request age-grid smoothed polygons from the same Reconstructed Geometries
		// layer while others request non-age-grid smoothed polygons and they can interfere with
		// each other - by requesting just before rendering it will update for us if needed.
		GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type layer_proxy;

		// Only the bands containing numerical (non-colour) data.
		std::vector<Band> numerical_bands;
	};

	/**
	 * Typedef for a sequence of (reconstructed) numerical rasters.
	 */
	typedef std::vector<NumericalRaster> numerical_raster_seq_type;


	/**
	 * Get the visible raster layers.
	 */
	void
	get_visible_raster_visual_layers(
			raster_visual_layer_seq_type &visible_raster_visual_layers,
			GPlatesPresentation::ViewState &view_state)
	{
		GPlatesPresentation::VisualLayers &visual_layers = view_state.get_visual_layers();

		// Iterate over the *visible* visual layers.
		const unsigned int num_layers = visual_layers.size();
		for (unsigned int n = 0; n < num_layers; ++n)
		{
			boost::shared_ptr<const GPlatesPresentation::VisualLayer> visual_layer =
					visual_layers.visual_layer_at(n).lock();
			if (visual_layer &&
				visual_layer->is_visible() &&
				visual_layer->get_layer_type() == static_cast<unsigned int>(GPlatesAppLogic::LayerTaskType::RASTER))
			{
				visible_raster_visual_layers.push_back(visual_layer);
			}
		}
	}


	/**
	 * Get the all rasters from the set of visible layers.
	 *
	 * We include numerical rasters because they can be converted to colour using their layer's palette.
	 */
	void
	get_visible_colour_rasters(
			GPlatesOpenGL::GLRenderer &renderer,
			GPlatesPresentation::ViewState &view_state,
			colour_raster_seq_type &colour_rasters)
	{
		raster_visual_layer_seq_type visible_raster_visual_layers;
		get_visible_raster_visual_layers(visible_raster_visual_layers, view_state);

		// Iterate over the raster layers.
		for (unsigned int n = 0; n < visible_raster_visual_layers.size(); ++n)
		{
			const boost::shared_ptr<const GPlatesPresentation::VisualLayer> visible_raster_visual_layer =
					visible_raster_visual_layers[n];

			const GPlatesAppLogic::Layer &raster_layer =
					visible_raster_visual_layer->get_reconstruct_graph_layer();
			boost::optional<GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type> raster_output_opt =
					raster_layer.get_layer_output<GPlatesAppLogic::RasterLayerProxy>();
			if (!raster_output_opt)
			{
				// This shouldn't happen since the visual layers should all be raster layers.
				continue;
			}
			GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type raster_output = raster_output_opt.get();

			boost::optional<GPlatesAppLogic::ResolvedRaster::non_null_ptr_type> resolved_raster =
					raster_output->get_resolved_raster();
			if (!resolved_raster)
			{
				continue;
			}

			// All raster layers should be able to generate colour - including numerical rasters.

			GPlatesPresentation::VisualLayerParams::non_null_ptr_to_const_type visual_layer_params =
					visible_raster_visual_layer->get_visual_layer_params();
			if (const GPlatesPresentation::RasterVisualLayerParams *raster_layer_params =
				dynamic_cast<const GPlatesPresentation::RasterVisualLayerParams *>(visual_layer_params.get()))
			{
				colour_rasters.push_back(
						ColourRaster(
								visible_raster_visual_layer->get_name()/*layer_name*/,
								resolved_raster.get(),
								raster_layer_params->get_colour_palette_parameters().get_colour_palette(),
								raster_layer_params->get_modulate_colour(),
								raster_layer_params->get_surface_relief_scale()));
			}
		}
	}


	/**
	 * Get the rasters containing numerical bands from the set of visible layers.
	 */
	void
	get_visible_numerical_rasters(
			GPlatesOpenGL::GLRenderer &renderer,
			GPlatesPresentation::ViewState &view_state,
			numerical_raster_seq_type &numerical_rasters)
	{
		raster_visual_layer_seq_type visible_raster_visual_layers;
		get_visible_raster_visual_layers(visible_raster_visual_layers, view_state);

		// Iterate over the raster layers.
		for (unsigned int n = 0; n < visible_raster_visual_layers.size(); ++n)
		{
			const boost::shared_ptr<const GPlatesPresentation::VisualLayer> visible_raster_visual_layer =
					visible_raster_visual_layers[n];

			const GPlatesAppLogic::Layer &raster_layer =
					visible_raster_visual_layer->get_reconstruct_graph_layer();
			boost::optional<GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type> raster_output_opt =
					raster_layer.get_layer_output<GPlatesAppLogic::RasterLayerProxy>();
			if (!raster_output_opt)
			{
				// This shouldn't happen since the visual layers should all be raster layers.
				continue;
			}
			GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type raster_output = raster_output_opt.get();

			std::vector<NumericalRaster::Band> numerical_bands;

			// Iterate over the raster bands and find those that are numerical bands.
			const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &raster_band_names =
					raster_output->get_raster_band_names();
			BOOST_FOREACH(
					const GPlatesPropertyValues::XsString::non_null_ptr_to_const_type &raster_band_name,
					raster_band_names)
			{
				const GPlatesPropertyValues::TextContent &band_name = raster_band_name->value();

				// Exclude colour raster bands.
				if (!raster_output->does_raster_band_contain_numerical_data(band_name))
				{
					continue;
				}

				numerical_bands.push_back(NumericalRaster::Band(band_name));
			}

			if (numerical_bands.empty())
			{
				continue;
			}

			const QString layer_name = visible_raster_visual_layer->get_name();

			numerical_rasters.push_back(NumericalRaster(layer_name, raster_output, numerical_bands));
		}
	}


	/**
	 * Determines the raster map projection and calculates the export raster dimensions from resolution
	 * and lat/lon extents.
	 */
	GPlatesGui::MapProjection::non_null_ptr_type
	get_export_raster_projection_and_parameters(
			const GPlatesGui::ExportRasterAnimationStrategy::Configuration &configuration,
			unsigned int &raster_width,
			unsigned int &raster_height,
			GPlatesPropertyValues::Georeferencing::lat_lon_extents_type &lat_lon_extents)
	{
		lat_lon_extents = configuration.lat_lon_extents;

		// Clamp latitudes.
		if (lat_lon_extents.top > 90)
		{
			lat_lon_extents.top	= 90;
		}
		else if (lat_lon_extents.top < -90)
		{
			lat_lon_extents.top	= -90;
		}
		if (lat_lon_extents.bottom > 90)
		{
			lat_lon_extents.bottom	= 90;
		}
		else if (lat_lon_extents.bottom < -90)
		{
			lat_lon_extents.bottom	= -90;
		}

		// Get the lat/lon extents.
		// And restrict the lon extents to 360 degrees if necessary (raster need only cover entire globe).
		const double lat_extent = lat_lon_extents.top - lat_lon_extents.bottom;
		double lon_extent = lat_lon_extents.right - lat_lon_extents.left;
		if (lon_extent > 360)
		{
			lon_extent = 360;
		}
		else if (lon_extent < -360)
		{
			lon_extent = -360;
		}

		// Ensure longitude is within range [-360, 360] so we can use GPlatesMaths::LatLonPoint.
		if (lat_lon_extents.left < lat_lon_extents.right)
		{
			// Make sure 'left' is in range [-360,0] so that 'right' will be in range [-360,360].
			while (lat_lon_extents.left < -360)
			{
				lat_lon_extents.left += 360;
			}
			while (lat_lon_extents.left > 0)
			{
				lat_lon_extents.left -= 360;
			}
		}
		else
		{
			// Make sure 'left' is in range [0,360] so that 'right' will be in range [-360,360].
			while (lat_lon_extents.left < 0)
			{
				lat_lon_extents.left += 360;
			}
			while (lat_lon_extents.left > 360)
			{
				lat_lon_extents.left -= 360;
			}
		}

		// Fixup 'right' in case 'left' or 'lon_extent' were modified.
		lat_lon_extents.right = lat_lon_extents.left + lon_extent;

		// Avoid zero width or height exported raster.
		// Thrown exception will get caught and report error (and update status message).
		GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
				!GPlatesMaths::are_almost_exactly_equal(lat_extent, 0.0) &&
					!GPlatesMaths::are_almost_exactly_equal(lon_extent, 0.0),
				GPLATES_EXCEPTION_SOURCE,
				QObject::tr("latitude/longitude extents must have finite extents"));

		// Avoid divide by zero.
		// Thrown exception will get caught and report error (and update status message).
		GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
				!GPlatesMaths::are_almost_exactly_equal(configuration.resolution_in_degrees, 0.0),
				GPLATES_EXCEPTION_SOURCE,
				QObject::tr("raster resolution cannot be zero"));

		// We use absolute value in case user swapped top/bottom or left/right to flip exported raster.
		// We also round to the nearest integer.
		raster_width = static_cast<unsigned int>(std::fabs(lon_extent / configuration.resolution_in_degrees) + 0.5);
		raster_height = static_cast<unsigned int>(std::fabs(lat_extent / configuration.resolution_in_degrees) + 0.5);
		if (raster_width == 0)
		{
			raster_width = 1;
		}
		if (raster_height == 0)
		{
			raster_height = 1;
		}

		// Grid registration uses an extra row and column of pixels (data points) since data points are
		// *on* the grid lines instead of at the centre of grid cells (area between grid lines).
		// For example...
		//
		//   +-+-+    -----
		//   | | |    |+|+|
		//   +-+-+    -----
		//   | | |    |+|+|
		//   +-+-+    -----
		//
		// ...the '+' symbols are data points.
		// The left is grid line registration with 2x2 data points.
		// The right is pixel registration with 3x3 data points.
		//
		// However note that the grid resolution (spacing between data points) remains the same.
		//
		if (configuration.use_grid_line_registration)
		{
			raster_width += 1;
			raster_height += 1;
		}

		const GPlatesGui::MapProjection::non_null_ptr_type map_projection =
				GPlatesGui::MapProjection::create(GPlatesGui::MapProjection::RECTANGULAR);

		// Set the central meridian (longitude) - the latitude doesn't matter here.
		//
		// We do this because the map view renders [-180,180] about its central meridian.
		// Anything outside that range does not get rendered.
		// So we need to make sure the full [left,right] range gets rendered.
		const double central_lon = 0.5 * (lat_lon_extents.left + lat_lon_extents.right);
		map_projection->set_central_llp(GPlatesMaths::LatLonPoint(0, central_lon));

		return map_projection;
	}


	GPlatesOpenGL::GLRenderer::non_null_ptr_type
	create_gl_renderer(
			GPlatesGui::ExportAnimationContext &export_animation_context)
	{
		// Get an OpenGL context.
		GPlatesOpenGL::GLContext::non_null_ptr_type gl_context =
				export_animation_context.viewport_window().reconstruction_view_widget()
						.globe_and_map_widget().get_active_gl_context();

		// Make sure the context is currently active.
		gl_context->make_current();

		// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
		return gl_context->create_renderer();
	}


	/**
	 * Setup a tile for rendering.
	 */
	void
	setup_tile_for_rendering(
			const unsigned int export_raster_width,
			const unsigned int export_raster_height,
			const bool export_raster_grid_line_registration,
			const GPlatesPropertyValues::Georeferencing::lat_lon_extents_type &pixel_rendering_lat_lon_extents,
			GPlatesOpenGL::GLRenderer &renderer,
			GPlatesOpenGL::GLTileRender &tile_render)
	{
		GPlatesOpenGL::GLViewport current_tile_render_target_viewport;
		tile_render.get_tile_render_target_viewport(current_tile_render_target_viewport);

		GPlatesOpenGL::GLViewport current_tile_render_target_scissor_rect;
		tile_render.get_tile_render_target_scissor_rectangle(current_tile_render_target_scissor_rect);

		// Mask off rendering outside the current tile region.
		// This includes 'gl_clear()' calls which clear the entire framebuffer.
		//
		// This is not really necessary since there is no border region around the tiles
		// because we are not rendering any fat point or wide line primitives.
		renderer.gl_enable(GL_SCISSOR_TEST);
		renderer.gl_scissor(
				current_tile_render_target_scissor_rect.x(),
				current_tile_render_target_scissor_rect.y(),
				current_tile_render_target_scissor_rect.width(),
				current_tile_render_target_scissor_rect.height());
		renderer.gl_viewport(
				current_tile_render_target_viewport.x(),
				current_tile_render_target_viewport.y(),
				current_tile_render_target_viewport.width(),
				current_tile_render_target_viewport.height());

		// Clear the colour buffer (and we don't have a depth/stencil buffer).
		renderer.gl_clear_color(); // Clear colour to (0,0,0,0).
		renderer.gl_clear(GL_COLOR_BUFFER_BIT);

		// Adjust the projection transform for the current tile.
		const GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type projection_transform_tile =
				tile_render.get_tile_projection_transform();
		GPlatesOpenGL::GLMatrix projection_matrix_tile = projection_transform_tile->get_matrix();
		// The regular projection transform maps to the lat/lon georeferencing region of exported raster.
		// These lat-lon extents should be using pixel registration since we are rendering pixels which have
		// a pixel area (box) - we want to map the view frustum to the *corners/edges* of the border pixels
		// - for grid line registration this means border pixel *centres* are at the original lat-lon extents
		// (specified by the user) and so our pixel-rendering lat-lon extents got expanded by half a pixel.
		projection_matrix_tile.gl_ortho(
				pixel_rendering_lat_lon_extents.left, pixel_rendering_lat_lon_extents.right,
				// NOTE: Invert top and bottom since OpenGL inverts the coordinate system (along y-axis)...
				pixel_rendering_lat_lon_extents.top/*bottom*/, pixel_rendering_lat_lon_extents.bottom/*top*/,
				-999999, 999999);

		// Model-view matrix defaults to identity for all cases except *grid line* registration where
		// at least one of the top/bottom/left/right extents is near the global lat-lon boundary
		// (for reasons explained below).
		GPlatesOpenGL::GLMatrix model_view_matrix_tile;
		// Grid registration places data points *on* the grid lines instead of at the centre of
		// grid cells (area between grid lines). For example...
		//
		//   -------------             
		//   | + | + | + |    +---+---+
		//   -------------    |   |   |
		//   | + | + | + |    +---+---+
		//   -------------    |   |   |
		//   | + | + | + |    +---+---+
		//   -------------             
		//
		// ...the '+' symbols are data points.
		// On the left is the pixel registration we are using for rendering.
		// On the right is the original grid line registration.
		//
		if (export_raster_grid_line_registration)
		{
			// The number of bits of fixed-point precision used in the OpenGL pixel rasterization engine
			// determines the accuracy of vertex position coordinates in that there are
			// "2 ^ sub_pixel_bits" fixed-point positions between adjacent pixels that vertex coordinates
			// can get snapped (quantized) to.
			//
			// Note: The '1.01' is to give a little extra headroom (eg, for finite precision of floating-point).
			const double sub_pixel_precision = 1.01 / (1 << renderer.get_capabilities().framebuffer.gl_sub_pixel_bits);

			// The increment in vertex coordinates between two adjacent pixels in the x and y directions
			// (from left to right, and from bottom to top respectively).
			//
			// Note that *pixel* registration coordinates cover an extra pixel
			// (hence division by dimension instead of 'dimension - 1').
			const double pixel_increment_y =
					(pixel_rendering_lat_lon_extents.top - pixel_rendering_lat_lon_extents.bottom) / export_raster_height;
			const double pixel_increment_x =
					(pixel_rendering_lat_lon_extents.right - pixel_rendering_lat_lon_extents.left) / export_raster_width;

			// The pixel *centres* of the four border pixels in the corners of the image are half a pixel
			// inside the *pixel* registration extents (which bound pixel *areas*).
			const double top_pixel_centre = pixel_rendering_lat_lon_extents.top - 0.5 * pixel_increment_y;
			const double bottom_pixel_centre = pixel_rendering_lat_lon_extents.bottom + 0.5 * pixel_increment_y;
			const double left_pixel_centre = pixel_rendering_lat_lon_extents.left + 0.5 * pixel_increment_x;
			const double right_pixel_centre = pixel_rendering_lat_lon_extents.right - 0.5 * pixel_increment_x;

			//
			// If any pixel *centre* of the four border pixels (in the corners of the image) is close enough
			// to the global lat-lon extents then it's possible that the associated border pixels will not
			// get rendered since a global map is rendered with minimum/maximum latitude extent of -90/90.
			// The 'close enough' part is due to the sub-pixel precision mentioned above which snaps/quantizes
			// vertex coordinates a maximum distance of 'sub_pixel_precision'.
			//

			int adjust_top_flag = 0;
			if (top_pixel_centre + sub_pixel_precision * pixel_increment_y > 90.0 ||
				top_pixel_centre + sub_pixel_precision * pixel_increment_y < -90.0)
			{
				// Top row of pixels might not get rendered unless adjusted.
				adjust_top_flag = 1;
			}

			int adjust_bottom_flag = 0;
			if (bottom_pixel_centre - sub_pixel_precision * pixel_increment_y > 90.0 ||
				bottom_pixel_centre - sub_pixel_precision * pixel_increment_y < -90.0)
			{
				// Bottom row of pixels might not get rendered unless adjusted.
				adjust_bottom_flag = 1;
			}

			int adjust_left_flag = 0;
			if (left_pixel_centre - sub_pixel_precision * pixel_increment_x > 180.0 ||
				left_pixel_centre - sub_pixel_precision * pixel_increment_x < -180.0)
			{
				// Left column of pixels might not get rendered unless adjusted.
				adjust_left_flag = 1;
			}

			int adjust_right_flag = 0;
			if (right_pixel_centre + sub_pixel_precision * pixel_increment_x > 180.0 ||
				right_pixel_centre + sub_pixel_precision * pixel_increment_x < -180.0)
			{
				// Right column of pixels might not get rendered unless adjusted.
				adjust_right_flag = 1;
			}

			//
			// We need to increase the scale of the rectangle map rendering slightly such that problematic
			// border pixel *centres* are now covered by the vertices/triangles of the map mesh.
			// In order to scale correctly we first need to translate such that the centre of scaling
			// is in the correct position, then do the scaling and then undo the translation.
			// If both latitude extents touch the global boundary then the centre of scaling
			// (in the y, or latitude, direction) should be in the middle
			// (the average of top and bottom latitudes) and the scaling should be twice as much so
			// that both top *and* bottom pixel centres are covered, otherwise if only one latitude extent
			// touches global boundary then centre of scaling is the opposite extent (such that it does
			// not move, and hence all scaling is applied to the extent requiring adjustment), otherwise
			// if neither latitude extent touches global boundary then there is no scaling (or translation).
			// The same applies in the x, or longitude, direction.
			//
			// This scaling does distort the exported raster a little bit but it's very small.
			// For example, a 1 degree resolution global raster of dimensions 361x181 and 8 sub-pixel bits renderer
			// is scaled by a factor of 1.0000434, but more importantly (for 8 sub-pixel bits) the largest
			// error in units of pixels does not exceed '2 ^ -8 = 0.004' of a pixel (regardless of resolution).
			//

			double translate_x = 0.0;
			double scale_x = 1.0;
			if (adjust_left_flag || adjust_right_flag)
			{
				translate_x = (adjust_left_flag * pixel_rendering_lat_lon_extents.right +
						adjust_right_flag * pixel_rendering_lat_lon_extents.left) /
						(adjust_left_flag + adjust_right_flag);
				scale_x = (export_raster_width - 1 + (adjust_left_flag + adjust_right_flag) * sub_pixel_precision) /
						(export_raster_width - 1);
			}

			double translate_y = 0.0;
			double scale_y = 1.0;
			if (adjust_top_flag || adjust_bottom_flag)
			{
				translate_x = (adjust_top_flag * pixel_rendering_lat_lon_extents.bottom +
						adjust_bottom_flag * pixel_rendering_lat_lon_extents.top) /
						(adjust_top_flag + adjust_bottom_flag);
				scale_y = (export_raster_height - 1 + (adjust_top_flag + adjust_bottom_flag) * sub_pixel_precision) /
						(export_raster_height - 1);
			}

			// First translate *to* the centre of scaling (using '-translate_x' and '-translate_y'), then
			// scale and finally translate *from* the centre of scaling (using 'translate_x' and 'translate_y').
			//
			// Note that if neither top nor bottom need adjusting then there is no translation
			// or scaling in the y direction. Similarly for left and right for the x direction.
			//
			// Also note that no scaling or translation is needed for the z direction at all.
			model_view_matrix_tile.gl_translate(translate_x, translate_y, 0.0);
			model_view_matrix_tile.gl_scale(scale_x, scale_y, 1.0);
			model_view_matrix_tile.gl_translate(-translate_x, -translate_y, 0.0);
		}

		renderer.gl_load_matrix(GL_MODELVIEW, model_view_matrix_tile);
		renderer.gl_load_matrix(GL_PROJECTION, projection_matrix_tile);
	}


	/**
	 * Reads coloured tile data and returns as an RGBA8 raw raster.
	 */
	GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type
	read_colour_tile_data(
			GPlatesOpenGL::GLRenderer &renderer,
			const GPlatesOpenGL::GLPixelBuffer::shared_ptr_type &tile_pixel_buffer,
			const unsigned int tile_width,
			const unsigned int tile_height)
	{
		// Floating-point raw raster to contain data in the tile region.
		GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type tile_data_raster =
				GPlatesPropertyValues::Rgba8RawRaster::create(
						tile_width,
						tile_height);

		// Map the pixel buffer to access its data.
		GPlatesOpenGL::GLBuffer::MapBufferScope map_tile_pixel_buffer_scope(
				renderer,
				*tile_pixel_buffer->get_buffer(),
				GPlatesOpenGL::GLBuffer::TARGET_PIXEL_PACK_BUFFER);

		// Map the pixel buffer data.
		const void *tile_data =
				map_tile_pixel_buffer_scope.gl_map_buffer_static(GPlatesOpenGL::GLBuffer::ACCESS_READ_ONLY);
		const GPlatesGui::rgba8_t *tile_pixel_data = static_cast<const GPlatesGui::rgba8_t *>(tile_data);

		// Read data from the pixel buffer into the raw raster.
		for (unsigned int y = 0; y < tile_height; ++y)
		{
			for (unsigned int x = 0; x < tile_width; ++x)
			{
				const GPlatesGui::rgba8_t &pixel_data = tile_pixel_data[(y * tile_width + x)];

				tile_data_raster->data()[y * tile_width + x] = pixel_data;
			}
		}

		return tile_data_raster;
	}


	/**
	 * Reads a numerical band's tile data and returns as a float raw raster.
	 */
	GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type
	read_numerical_band_tile_data(
			GPlatesOpenGL::GLRenderer &renderer,
			const GPlatesOpenGL::GLPixelBuffer::shared_ptr_type &tile_pixel_buffer,
			const unsigned int tile_width,
			const unsigned int tile_height)
	{
		// Floating-point raw raster to contain data in the tile region.
		GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type band_tile_data_raster =
				GPlatesPropertyValues::FloatRawRaster::create(
						tile_width,
						tile_height);
		// The no-data value for a floating-point raw raster.
		const float no_data_value = GPlatesMaths::quiet_nan<float>();

		// Map the pixel buffer to access its data.
		GPlatesOpenGL::GLBuffer::MapBufferScope map_tile_pixel_buffer_scope(
				renderer,
				*tile_pixel_buffer->get_buffer(),
				GPlatesOpenGL::GLBuffer::TARGET_PIXEL_PACK_BUFFER);

		// Map the pixel buffer data.
		const void *band_tile_data =
				map_tile_pixel_buffer_scope.gl_map_buffer_static(GPlatesOpenGL::GLBuffer::ACCESS_READ_ONLY);
		const GLfloat *band_tile_pixel_data = static_cast<const GLfloat *>(band_tile_data);

		// Read data from the pixel buffer into the raw raster.
		for (unsigned int y = 0; y < tile_height; ++y)
		{
			for (unsigned int x = 0; x < tile_width; ++x)
			{
				// Each pixel is four floats (RGBA) where the first float (red channel) is the data
				// and the second float (green channel) is the coverage.
				const GLfloat *pixel_data = band_tile_pixel_data + (y * tile_width + x) * 4;
				const GLfloat data = pixel_data[0];
				const GLfloat coverage = pixel_data[1];

				// If the coverage exceeds 0.5 then consider the pixel valid, otherwise invalid.
				// Invalid pixels are no-data values in the raw raster.
				band_tile_data_raster->data()[y * tile_width + x] =
						(coverage> 0.5) ? data : no_data_value;
			}
		}

		GPlatesPropertyValues::RawRasterUtils::add_no_data_value(*band_tile_data_raster, no_data_value);

		return band_tile_data_raster;
	}


	void
	export_colour_raster(
			const ColourRaster &raster,
			const QString &filename,
			const unsigned int export_raster_width,
			const unsigned int export_raster_height,
			const bool export_raster_grid_line_registration,
			const bool export_raster_compress,
			const GPlatesPropertyValues::Georeferencing::non_null_ptr_type &georeferencing,
			const GPlatesPropertyValues::Georeferencing::lat_lon_extents_type &pixel_registration_lat_lon_extents,
			const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
			GPlatesOpenGL::GLRenderer &renderer,
			const GPlatesGui::MapProjection::non_null_ptr_to_const_type &map_projection)
	{
		// The raster writer will be used to write each tile of exported raster.
		GPlatesFileIO::RasterWriter::non_null_ptr_type raster_writer =
				GPlatesFileIO::RasterWriter::create(
						filename,
						export_raster_width,
						export_raster_height,
						1/*num_raster_bands*/,
						GPlatesPropertyValues::RasterType::RGBA8,
						export_raster_compress);

		if (!raster_writer->can_write())
		{
			// Thrown exception will get caught and report error (and update status message).
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,
					QObject::tr("unable to write to raster internal buffer"));
		}

		// We will render the exported raster in tiles if it's larger than our tile render target size.
		//
		// If hardware does not support 2048x2048 textures then we'll lower it.
		// With RGBA 8-bit-per-channel texture this will be 16Mb.
		unsigned int tile_render_target_dimension = 2048;
		if (tile_render_target_dimension > renderer.get_capabilities().texture.gl_max_texture_size)
		{
			tile_render_target_dimension = renderer.get_capabilities().texture.gl_max_texture_size;
		}

		// Get a render target for rendering our tiles to.
		boost::optional<GPlatesOpenGL::GLRenderTarget::shared_ptr_type> tile_render_target =
				renderer.get_context().get_shared_state()->acquire_render_target(
						renderer,
						GL_RGBA8,
						false/*include_depth_buffer*/,
						false/*include_stencil_buffer*/,
						tile_render_target_dimension,
						tile_render_target_dimension);
		// Thrown exception will get caught and report error (and update status message).
		GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
				tile_render_target,
				GPLATES_ASSERTION_SOURCE,
				QObject::tr("graphics hardware does not support render targets"));

		// Get a pixel buffer so we can read the render target data from GPU to CPU.
		GPlatesOpenGL::GLPixelBuffer::shared_ptr_type tile_pixel_buffer =
				renderer.get_context().get_shared_state()->acquire_pixel_buffer(
						renderer,
						4/*RGBA*/ * sizeof(GLubyte) * tile_render_target_dimension * tile_render_target_dimension,
						GPlatesOpenGL::GLBuffer::USAGE_STREAM_READ);

		// Set up for rendering the exported raster into tiles.
		GPlatesOpenGL::GLTileRender tile_render(
				tile_render_target_dimension,
				tile_render_target_dimension,
				GPlatesOpenGL::GLViewport(
						0,
						0,
						export_raster_width,
						export_raster_height)/*destination_viewport*/);

		// We need to adjust the lat/lon extents used for rendering (as opposed to the extents
		// stored as georeferencing in the exported file) since the map view adjusts longitude
		// according to the map projection's central meridian.
		const double map_view_central_meridian =
				map_projection->get_projection_settings().get_central_llp().longitude();
		GPlatesPropertyValues::Georeferencing::lat_lon_extents_type pixel_rendering_lat_lon_extents =
				pixel_registration_lat_lon_extents;
		pixel_rendering_lat_lon_extents.left -= map_view_central_meridian;
		pixel_rendering_lat_lon_extents.right -= map_view_central_meridian;

		// Set up raster alpha blending for pre-multiplied alpha.
		// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
		// This is where the RGB channels have already been multiplied by the alpha channel.
		// See class GLVisualRasterSource for why this is done.
		//
		// Note: The render target is fixed-point RGBA (and not floating-point) so we don't need to
		// worry about alpha-blending not being available for floating-point render targets.
		renderer.gl_enable(GL_BLEND);
		renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		// Enable alpha testing as an optimisation for culling transparent raster pixels.
		renderer.gl_enable(GL_ALPHA_TEST);
		renderer.gl_alpha_func(GL_GREATER, GLclampf(0));

		// Render the current raster band tile-by-tile.
#if 0 // UPDATE: No longer caching since uses up too much memory...
		std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type> tile_cache_handles;
#endif
		for (tile_render.first_tile(); !tile_render.finished(); tile_render.next_tile())
		{
			// Within this scope we will render to the tile render target.
			GPlatesOpenGL::GLRenderTarget::RenderScope tile_render_target_scope(
					*tile_render_target.get(),
					renderer);

			// Setup for rendering to the current tile.
			setup_tile_for_rendering(
					export_raster_width,
					export_raster_height,
					export_raster_grid_line_registration,
					pixel_rendering_lat_lon_extents,
					renderer,
					tile_render);

			// Render the (possibly reconstructed) raster to the current tile.
			const GPlatesOpenGL::GLVisualLayers::cache_handle_type tile_cache_handle =
					gl_visual_layers->render_raster(
							renderer,
							raster.resolved_raster,
							raster.raster_colour_palette,
							raster.raster_modulate_colour,
							raster.surface_relief_scale,
							map_projection);
#if 0 // UPDATE: No longer caching since uses up too much memory...
			// Keep the cache handles alive over all the tiles since colour rasters do not cache
			// the entire multi-resolution pyramid (the GLMultiResolutionRaster in RasterLayerProxy).
			// This uses more memory but speeds things up since each tile does not have to rebuild as much.
			//
			// NOTE: This uses significantly more memory though (it won't appear under 'private' memory).
			// TODO: Remove this when conversion of float pixels to RGBA (via palette) is moved
			// from CPU to the GPU (currently this part of building the raster's colour tiles is
			// what slows down the rendering process so much - only really noticeable for large
			// rasters though, such as 1 minute resolution).
			tile_cache_handles.push_back(tile_cache_handle);
#endif

			GPlatesOpenGL::GLViewport current_tile_source_viewport;
			tile_render.get_tile_source_viewport(current_tile_source_viewport);

			// Bind the pixel buffer so that all subsequent 'gl_read_pixels()' calls go into that buffer.
			tile_pixel_buffer->gl_bind_pack(renderer);

			// Request asynchronous transfer of render target data into pixel buffer.
			// We (CPU) won't block until we actually map the pixel buffer.
			//
			// Note that the tile render target must currently be active since it's the source of our read.
			//
			// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
			// since our data is RGBA (already 4-byte aligned).
			tile_pixel_buffer->gl_read_pixels(
					renderer,
					current_tile_source_viewport.x(),
					current_tile_source_viewport.y(),
					current_tile_source_viewport.width(),
					current_tile_source_viewport.height(),
					GL_RGBA,
					GL_UNSIGNED_BYTE,
					0);

			GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type tile_data =
					read_colour_tile_data(
							renderer,
							tile_pixel_buffer,
							current_tile_source_viewport.width()/*tile_width*/,
							current_tile_source_viewport.height()/*tile_height*/);

			GPlatesOpenGL::GLViewport current_tile_destination_viewport;
			tile_render.get_tile_destination_viewport(current_tile_destination_viewport);

			// Write the tile data into the exported raster band.
			if (!raster_writer->write_region_data(
					tile_data,
					1, // only one band for colour rasters
					current_tile_destination_viewport.x()/*x_offset*/,
					current_tile_destination_viewport.y()/*y_offset*/))
			{
				// Thrown exception will get caught and report error (and update status message).
				throw GPlatesGlobal::LogException(
						GPLATES_EXCEPTION_SOURCE,
						QObject::tr("error writing tile region to raster"));
			}
		}

		// Set the exported raster's georeferencing.
		// This will get ignored by those colour file formats that do not support georeferencing.
		raster_writer->set_georeferencing(georeferencing);

		// Set the exported raster's spatial reference system.
		// This is needed by ArcGIS otherwise the user has to manually set the SRS in ArcGIS
		// if they want the raster to display correctly in other projections (such as MollWeide).
		// For now we just set it to WGS84.
		raster_writer->set_spatial_reference_system(
				GPlatesPropertyValues::SpatialReferenceSystem::get_WGS84());

		// Write the entire raster (all the tiles) to the export raster file.
		if (!raster_writer->write_file())
		{
			// Thrown exception will get caught and report error (and update status message).
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,
					QObject::tr("error writing to raster file"));
		}
	}


	void
	export_numerical_raster_band(
			const GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type &band_data,
			const unsigned int raster_band_index,
			const unsigned int export_raster_width,
			const unsigned int export_raster_height,
			const bool export_raster_grid_line_registration,
			const GPlatesPropertyValues::Georeferencing::lat_lon_extents_type &pixel_registration_lat_lon_extents,
			GPlatesFileIO::RasterWriter &raster_writer,
			GPlatesOpenGL::GLRenderer &renderer,
			const GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_type &map_cube_mesh)
	{
		// We need support for floating-point textures (for *numerical* raster data).
		// Thrown exception will get caught and report error (and update status message).
		GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
				renderer.get_capabilities().texture.gl_ARB_texture_float,
				GPLATES_ASSERTION_SOURCE,
				QObject::tr("graphics hardware does not support floating-point textures"));

		// We will render the exported raster in tiles if it's larger than our tile render target size.
		//
		// If hardware does not support 1024x1024 textures then we'll lower it.
		// With RGBA floating-point texture this will be 16Mb.
		unsigned int tile_render_target_dimension = 1024;
		if (tile_render_target_dimension > renderer.get_capabilities().texture.gl_max_texture_size)
		{
			tile_render_target_dimension = renderer.get_capabilities().texture.gl_max_texture_size;
		}

		// Get a render target for rendering our tiles to.
		boost::optional<GPlatesOpenGL::GLRenderTarget::shared_ptr_type> tile_render_target =
				renderer.get_context().get_shared_state()->acquire_render_target(
						renderer,
						GL_RGBA32F_ARB,
						false/*include_depth_buffer*/,
						false/*include_stencil_buffer*/,
						tile_render_target_dimension,
						tile_render_target_dimension);
		// Thrown exception will get caught and report error (and update status message).
		GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
				tile_render_target,
				GPLATES_ASSERTION_SOURCE,
				QObject::tr("graphics hardware does not support render targets"));

		// Get a pixel buffer so we can read the render target data from GPU to CPU.
		GPlatesOpenGL::GLPixelBuffer::shared_ptr_type tile_pixel_buffer =
				renderer.get_context().get_shared_state()->acquire_pixel_buffer(
						renderer,
						4/*RGBA*/ * sizeof(GLfloat) * tile_render_target_dimension * tile_render_target_dimension,
						GPlatesOpenGL::GLBuffer::USAGE_STREAM_READ);

		// Set up for rendering the exported raster into tiles.
		GPlatesOpenGL::GLTileRender tile_render(
				tile_render_target_dimension,
				tile_render_target_dimension,
				GPlatesOpenGL::GLViewport(
						0,
						0,
						export_raster_width,
						export_raster_height)/*destination_viewport*/);

		// Create the multi-resolution raster map view to render the raster data
		// to the equirectangular map projection.
		GPlatesOpenGL::GLMultiResolutionRasterMapView::non_null_ptr_type raster_band_map_view =
				GPlatesOpenGL::GLMultiResolutionRasterMapView::create(
						renderer,
						band_data,
						map_cube_mesh);

		// We need to adjust the lat/lon extents used for rendering (as opposed to the extents
		// stored as georeferencing in the exported file) since the map view adjusts longitude
		// according to the map projection's central meridian.
		const double map_view_central_meridian =
				map_cube_mesh->get_current_map_projection_settings().get_central_llp().longitude();
		GPlatesPropertyValues::Georeferencing::lat_lon_extents_type pixel_rendering_lat_lon_extents =
				pixel_registration_lat_lon_extents;
		pixel_rendering_lat_lon_extents.left -= map_view_central_meridian;
		pixel_rendering_lat_lon_extents.right -= map_view_central_meridian;

		// Render the current raster band tile-by-tile.
		for (tile_render.first_tile(); !tile_render.finished(); tile_render.next_tile())
		{
			// Within this scope we will render to the tile render target.
			GPlatesOpenGL::GLRenderTarget::RenderScope tile_render_target_scope(
					*tile_render_target.get(),
					renderer);

			// Setup for rendering to the current tile.
			setup_tile_for_rendering(
					export_raster_width,
					export_raster_height,
					export_raster_grid_line_registration,
					pixel_rendering_lat_lon_extents,
					renderer,
					tile_render);

			// Render the (possibly reconstructed) raster to the current tile.
			//
			// Multi-resolution *data* rasters have their entire raster cached so we don't need to
			// hold onto the cache handles across tiles. This enables us to use less memory since
			// the *cube* data raster wrapping the regular data raster will not also cache the
			// entire raster (at the level-of-detail we are exporting at anyway).
			GPlatesOpenGL::GLMultiResolutionRasterMapView::cache_handle_type tile_cache_handle;
			raster_band_map_view->render(renderer, tile_cache_handle);

			GPlatesOpenGL::GLViewport current_tile_source_viewport;
			tile_render.get_tile_source_viewport(current_tile_source_viewport);

			// Bind the pixel buffer so that all subsequent 'gl_read_pixels()' calls go into that buffer.
			tile_pixel_buffer->gl_bind_pack(renderer);

			// Request asynchronous transfer of render target data into pixel buffer.
			// We (CPU) won't block until we actually map the pixel buffer.
			//
			// Note that the tile render target must currently be active since it's the source of our read.
			//
			// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
			// since our data is floats (each float is already 4-byte aligned).
			tile_pixel_buffer->gl_read_pixels(
					renderer,
					current_tile_source_viewport.x(),
					current_tile_source_viewport.y(),
					current_tile_source_viewport.width(),
					current_tile_source_viewport.height(),
					GL_RGBA,
					GL_FLOAT,
					0);

			GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type band_tile_data =
					read_numerical_band_tile_data(
							renderer,
							tile_pixel_buffer,
							current_tile_source_viewport.width()/*tile_width*/,
							current_tile_source_viewport.height()/*tile_height*/);

			GPlatesOpenGL::GLViewport current_tile_destination_viewport;
			tile_render.get_tile_destination_viewport(current_tile_destination_viewport);

			// Write the tile data into the exported raster band.
			if (!raster_writer.write_region_data(
					band_tile_data,
					raster_band_index + 1, // band number starts at 1
					current_tile_destination_viewport.x()/*x_offset*/,
					current_tile_destination_viewport.y()/*y_offset*/))
			{
				// Thrown exception will get caught and report error (and update status message).
				throw GPlatesGlobal::LogException(
						GPLATES_EXCEPTION_SOURCE,
						QObject::tr("error writing tile region to raster"));
			}
		}
	}


	void
	export_numerical_raster(
			const NumericalRaster &raster,
			const QString &filename,
			const unsigned int export_raster_width,
			const unsigned int export_raster_height,
			const bool export_raster_grid_line_registration,
			const bool export_raster_compress,
			const GPlatesPropertyValues::Georeferencing::non_null_ptr_type &georeferencing,
			const GPlatesPropertyValues::Georeferencing::lat_lon_extents_type &pixel_registration_lat_lon_extents,
			GPlatesOpenGL::GLRenderer &renderer,
			const GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_type &map_cube_mesh)
	{
		// The raster writer will be used to write each tile of exported raster to
		// each band of the exported raster.
		GPlatesFileIO::RasterWriter::non_null_ptr_type raster_writer =
				GPlatesFileIO::RasterWriter::create(
						filename,
						export_raster_width,
						export_raster_height,
						raster.numerical_bands.size(),
						GPlatesPropertyValues::RasterType::FLOAT,
						export_raster_compress);

		if (!raster_writer->can_write())
		{
			// Thrown exception will get caught and report error (and update status message).
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,
					QObject::tr("unable to write to raster internal buffer"));
		}

		// Write out the bands of the current raster.
		for (unsigned int band_index = 0; band_index < raster.numerical_bands.size(); ++band_index)
		{
			const NumericalRaster::Band &band = raster.numerical_bands[band_index];

			// Get the band data.
			//
			// NOTE: The raster band data *must* be obtained just before rendering because different
			// raster layers can request age-grid smoothed polygons from the same Reconstructed Geometries
			// layer while others request non-age-grid smoothed polygons and they can interfere with
			// each other - by requesting just before rendering it will update for us if needed.
			boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
					band_data = raster.layer_proxy->get_multi_resolution_data_cube_raster(
							renderer,
							band.name);
			// If this fails it most likely means OpenGL support was insufficient.
			// Thrown exception will get caught and report error (and update status message).
			// Note that we've already checked that the raster contains numerical data so this
			// should have already caught most of these types of errors.
			GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
					band_data,
					GPLATES_EXCEPTION_SOURCE,
					QObject::tr("graphics hardware must support floating-point textures and shader programs"));

			export_numerical_raster_band(
					band_data.get(),
					band_index,
					export_raster_width,
					export_raster_height,
					export_raster_grid_line_registration,
					pixel_registration_lat_lon_extents,
					*raster_writer,
					renderer,
					map_cube_mesh);
		}

		// Set the exported raster's georeferencing.
		raster_writer->set_georeferencing(georeferencing);

		// Set the exported raster's spatial reference system.
		// This is needed by ArcGIS otherwise the user has to manually set the SRS in ArcGIS
		// if they want the raster to display correctly in other projections (such as MollWeide).
		// For now we just set it to WGS84.
		raster_writer->set_spatial_reference_system(
				GPlatesPropertyValues::SpatialReferenceSystem::get_WGS84());

		// Write the entire raster (all the tiles) to the export raster file.
		if (!raster_writer->write_file())
		{
			// Thrown exception will get caught and report error (and update status message).
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,
					QObject::tr("error writing to raster file"));
		}
	}
}


GPlatesGui::ExportRasterAnimationStrategy::ExportRasterAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &configuration):
	ExportAnimationStrategy(export_animation_context),
	d_configuration(configuration)
{
	set_template_filename(d_configuration->get_filename_template());
}

bool
GPlatesGui::ExportRasterAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{	
	GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Figure out a filename from the template filename sequence.
	const QString basename = *filename_it++;
	// The exported filename is not known until we visit the raster layers.
	boost::optional<QString> export_raster_filename;

	try
	{
		// Reconstructed raster export requires an OpenGL renderer (to reconstruct floating-point raster data).
		GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer =
				create_gl_renderer(*d_export_animation_context_ptr);

		// Calculate the exported raster dimensions.
		unsigned int export_raster_width;
		unsigned int export_raster_height;
		GPlatesPropertyValues::Georeferencing::lat_lon_extents_type lat_lon_extents;
		const GPlatesGui::MapProjection::non_null_ptr_type map_projection =
				get_export_raster_projection_and_parameters(
						*d_configuration,
						export_raster_width,
						export_raster_height,
						lat_lon_extents);

		const bool export_raster_grid_line_registration = d_configuration->use_grid_line_registration;

		// Create georeferencing from the original lat-lon extents (specified by user),
		// the grid line registration option (specified by user) and the resultant raster dimensions
		// (also derived from user-specified pixel resolution).
		GPlatesPropertyValues::Georeferencing::non_null_ptr_type georeferencing =
				GPlatesPropertyValues::Georeferencing::create(
						lat_lon_extents,
						export_raster_width,
						export_raster_height,
						export_raster_grid_line_registration);

		// Retrieve the lat-lon extents in *pixel* registration format since we are rendering pixels which have
		// a pixel area (box) - we want to map the view frustum to the *corners/edges* of the border pixels
		// - for grid line registration this means border pixel *centres* are at the original lat-lon extents
		// (specified by the user) and so our pixel-rendering lat-lon extents got expanded by half a pixel.
		boost::optional<GPlatesPropertyValues::Georeferencing::lat_lon_extents_type>
				pixel_registration_lat_lon_extents_opt = georeferencing->get_lat_lon_extents(
						export_raster_width,
						export_raster_height,
						false/*convert_to_grid_line_registration*/);
		// This assertion shouldn't happen since we've restricted the range of latitudes already.
		GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
				pixel_registration_lat_lon_extents_opt,
				GPLATES_EXCEPTION_SOURCE,
				QObject::tr("latitude exceeded range [-90, 90]"));
		const GPlatesPropertyValues::Georeferencing::lat_lon_extents_type
				pixel_registration_lat_lon_extents = pixel_registration_lat_lon_extents_opt.get();

		// Compress raster if it is supported and has been turned on.
		const bool export_raster_compress = d_configuration->compress && d_configuration->compress.get();

		if (d_configuration->raster_type == Configuration::COLOUR)
		{
			// Start an explicit render scope.
			renderer->begin_render();

			// Get all rasters from the set of visible layers.
			// We include numerical rasters because they can be converted to colour using their layer's palette.
			colour_raster_seq_type colour_rasters;
			get_visible_colour_rasters(
					*renderer,
					d_export_animation_context_ptr->view_state(),
					colour_rasters);

			// End an explicit render scope to exclude any direct modifications of OpenGL via Qt
			// such as 'update_status_message()' below.
			renderer->end_render();

			// This will be used to render rasters as colour.
			const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type gl_visual_layers =
					d_export_animation_context_ptr->viewport_window().reconstruction_view_widget()
						.globe_and_map_widget().get_active_gl_visual_layers();

			// Iterate over the colour rasters and export them.
			for (unsigned int raster_index = 0; raster_index < colour_rasters.size(); ++raster_index)
			{
				const ColourRaster &raster = colour_rasters[raster_index];

				// Substitute the '%P' placeholder with the raster layer to get the exported raster filename.
				const QString export_raster_basename = calculate_output_basename(basename, raster.layer_name);
				// Add the target dir to that to figure out the absolute path + name.
				export_raster_filename = d_export_animation_context_ptr->target_dir()
						.absoluteFilePath(export_raster_basename);

				// Notify user which raster we're currently exporting.
				d_export_animation_context_ptr->update_status_message(
						QObject::tr("Writing colour raster at frame %2 to file \"%1\"...")
						.arg(export_raster_filename.get())
						.arg(frame_index) );

				// Start a begin_render/end_render scope.
				//
				// NOTE: We *don't* include the above 'update_status_message()' inside this render scope
				// because it gets Qt to paint which modifies the OpenGL state which confuses GLRenderer.
				// Previously this caused a bug that was *very* difficult to track down - the bug
				// showed up as missing cube map tiles in the (reconstructed) raster image and,
				// strangely enough, even some rendering of parts of the actual map canvas.
				// The client's contract to GLRenderer (within a render scope) is to never modify
				// the OpenGL state directly (to only make changes via GLRenderer) because GLRenderer
				// shadows the OpenGL state.
				GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);

				export_colour_raster(
						raster,
						export_raster_filename.get(),
						export_raster_width,
						export_raster_height,
						export_raster_grid_line_registration,
						export_raster_compress,
						georeferencing,
						pixel_registration_lat_lon_extents,
						gl_visual_layers,
						*renderer,
						map_projection);

			}
		}
		else if (d_configuration->raster_type == Configuration::NUMERICAL)
		{
			// Start an explicit render scope.
			renderer->begin_render();

			// Get the rasters containing numerical bands from the set of visible layers.
			numerical_raster_seq_type numerical_rasters;
			get_visible_numerical_rasters(
					*renderer,
					d_export_animation_context_ptr->view_state(),
					numerical_rasters);

			GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_type map_cube_mesh =
					GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_type(
							GPlatesOpenGL::GLMultiResolutionMapCubeMesh::create(
									*renderer,
									*map_projection));

			// End an explicit render scope to exclude any direct modifications of OpenGL via Qt
			// such as 'update_status_message()' below.
			renderer->end_render();

			// Iterate over the numerical rasters and export them.
			for (unsigned int raster_index = 0; raster_index < numerical_rasters.size(); ++raster_index)
			{
				const NumericalRaster &raster = numerical_rasters[raster_index];

				// Substitute the '%P' placeholder with the raster layer to get the exported raster filename.
				const QString export_raster_basename = calculate_output_basename(basename, raster.layer_name);
				// Add the target dir to that to figure out the absolute path + name.
				export_raster_filename = d_export_animation_context_ptr->target_dir()
						.absoluteFilePath(export_raster_basename);

				// Notify user which raster we're currently exporting.
				d_export_animation_context_ptr->update_status_message(
						QObject::tr("Writing numerical raster at frame %2 to file \"%1\"...")
						.arg(export_raster_filename.get())
						.arg(frame_index) );

				// Start a begin_render/end_render scope.
				//
				// NOTE: We *don't* include the above 'update_status_message()' inside this render scope
				// because it gets Qt to paint which modifies the OpenGL state which confuses GLRenderer.
				// Previously this caused a bug that was *very* difficult to track down.
				// The client's contract to GLRenderer (within a render scope) is to never modify
				// the OpenGL state directly (to only make changes via GLRenderer) because GLRenderer
				// shadows the OpenGL state.
				GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);

				export_numerical_raster(
						raster,
						export_raster_filename.get(),
						export_raster_width,
						export_raster_height,
						export_raster_grid_line_registration,
						export_raster_compress,
						georeferencing,
						pixel_registration_lat_lon_extents,
						*renderer,
						map_cube_mesh);
			}
		}
	}
	catch (const GPlatesGlobal::LogException &exc)
	{
		// Print error message without source code line number by extracting QString message
		// instead of calling virtual 'what()'. Makes it easier for the user to read.

		if (export_raster_filename)
		{
			// The error occurred while exporting a raster.
			d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error exporting to raster file \"%1\": %2")
						.arg(export_raster_filename.get())
						.arg(exc.get_message()));
		}
		else
		{
			// The error occurred before exporting any rasters.
			d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error during setup for exporting to raster file(s): %1")
						.arg(exc.get_message()));
		}

		return false;
	}
	catch (const std::exception &exc)
	{
		if (export_raster_filename)
		{
			// The error occurred while exporting a raster.
			d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error exporting to raster file \"%1\": %2")
						.arg(export_raster_filename.get())
						.arg(exc.what()));
		}
		else
		{
			// The error occurred before exporting any rasters.
			d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error during setup for exporting to raster file(s): %1")
						.arg(exc.what()));
		}

		return false;
	}
	
	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}
