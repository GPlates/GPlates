/* $Id$ */

/**
 * \file Draws @a RenderedGeometry objects onto the globe (orthographic view).
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "GlobeRenderedGeometryCollectionPainter.h"

#include "GlobeRenderedGeometryLayerPainter.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryUtils.h"


namespace GPlatesGui
{
	namespace
	{
		/**
		 * Visits a @a RenderedGeometryCollection to determine if rendered layers contain sub-surface geometries.
		 */
		class HasSubSurfaceLayers :
				public GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor<>
		{
		public:

			explicit
			HasSubSurfaceLayers(
					GPlatesOpenGL::GL &gl) :
				d_gl(gl),
				d_has_sub_surface_layers(false)
			{  }

			bool
			has_sub_surface_layers() const
			{
				return d_has_sub_surface_layers;
			}

			virtual
			bool
			visit_rendered_geometry_layer(
					const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer)
			{
				// Only visit if layer is active and we haven't found a sub-surface geometry yet.
				return rendered_geometry_layer.is_active() && !d_has_sub_surface_layers;
			}

			virtual
			void
			visit_rendered_resolved_scalar_field_3d(
					const GPlatesViewOperations::RenderedResolvedScalarField3D &rrsf)
			{
				d_has_sub_surface_layers = true;
			}

		private:

			GPlatesOpenGL::GL &d_gl;
			bool d_has_sub_surface_layers;
		};
	}
}


GPlatesGui::GlobeRenderedGeometryCollectionPainter::GlobeRenderedGeometryCollectionPainter(
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		const GPlatesPresentation::VisualLayers &visual_layers,
		int device_pixel_ratio) :
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_gl_visual_layers(gl_visual_layers),
	d_visual_layers(visual_layers),
	d_layer_painter(gl_visual_layers, device_pixel_ratio),
	d_scale(1.0f),
	d_visual_layers_reversed(false)
{  }


void
GPlatesGui::GlobeRenderedGeometryCollectionPainter::initialise_gl(
		GPlatesOpenGL::GL &gl)
{
	d_layer_painter.initialise_gl(gl);
}


void
GPlatesGui::GlobeRenderedGeometryCollectionPainter::shutdown_gl(
		GPlatesOpenGL::GL &gl)
{
	d_layer_painter.shutdown_gl(gl);
}


bool
GPlatesGui::GlobeRenderedGeometryCollectionPainter::has_sub_surface_geometries(
		GPlatesOpenGL::GL &gl) const
{
	HasSubSurfaceLayers visitor(gl);
	d_rendered_geometry_collection.accept_visitor(visitor);

	return visitor.has_sub_surface_layers();
}


GPlatesGui::GlobeRenderedGeometryCollectionPainter::cache_handle_type
GPlatesGui::GlobeRenderedGeometryCollectionPainter::paint_surface(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		const GPlatesOpenGL::GLIntersect::Plane &globe_horizon_plane,
		const double &viewport_zoom_factor,
		boost::optional<Colour> vector_geometries_override_colour)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_globe_state_scope(gl);

	// Initialise our paint parameters so our visit methods can access them.
	d_paint_params = PaintParams(
			gl,
			view_projection,
			viewport_zoom_factor,
			GlobeRenderedGeometryLayerPainter::PAINT_SURFACE,
			globe_horizon_plane,
			vector_geometries_override_colour);

	// Draw the layers.
	d_rendered_geometry_collection.accept_visitor(*this);

	// Get the cache handle for all the rendered layers.
	const cache_handle_type cache_handle = d_paint_params->d_cache_handle;

	// These parameters are only used for the duration of this 'paint()' method.
	d_paint_params = boost::none;

	return cache_handle;
}


GPlatesGui::GlobeRenderedGeometryCollectionPainter::cache_handle_type
GPlatesGui::GlobeRenderedGeometryCollectionPainter::paint_sub_surface(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		const double &viewport_zoom_factor,
		bool improve_performance_reduce_quality_hint)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_globe_state_scope(gl);

	// Initialise our paint parameters so our visit methods can access them.
	d_paint_params = PaintParams(
			gl,
			view_projection,
			viewport_zoom_factor,
			GlobeRenderedGeometryLayerPainter::PAINT_SUB_SURFACE,
			boost::none/*globe_horizon_plane*/,
			boost::none/*vector_geometries_override_colour*/,
			improve_performance_reduce_quality_hint);

	// Draw the layers.
	d_rendered_geometry_collection.accept_visitor(*this);

	// Get the cache handle for all the rendered layers.
	const cache_handle_type cache_handle = d_paint_params->d_cache_handle;

	// These parameters are only used for the duration of this 'paint()' method.
	d_paint_params = boost::none;

	return cache_handle;
}


bool
GPlatesGui::GlobeRenderedGeometryCollectionPainter::visit_rendered_geometry_layer(
		const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer)
{
	// If layer is not active then we don't want to visit it.
	if (!rendered_geometry_layer.is_active())
	{
		return false;
	}

	if (rendered_geometry_layer.is_empty())
	{
		return false;
	}

	// Draw the current rendered geometry layer.
	GlobeRenderedGeometryLayerPainter rendered_geom_layer_painter(
			rendered_geometry_layer,
			d_paint_params->d_inverse_viewport_zoom_factor,
			d_paint_params->d_paint_region,
			d_paint_params->d_globe_horizon_plane,
			d_paint_params->d_vector_geometries_override_colour,
			d_paint_params->d_improve_performance_reduce_quality_hint);
	rendered_geom_layer_painter.set_scale(d_scale);

	// Paint the layer.
	const cache_handle_type layer_cache = rendered_geom_layer_painter.paint(
			*d_paint_params->d_gl,
			d_paint_params->d_view_projection,
			d_layer_painter);

	// Cache the layer's painting.
	d_paint_params->d_cache_handle->push_back(layer_cache);

	// We've already visited the rendered geometry layer so don't visit its rendered geometries.
	return false;
}


void
GPlatesGui::GlobeRenderedGeometryCollectionPainter::set_scale(
		float scale)
{
	d_scale = scale;
}


bool
GPlatesGui::GlobeRenderedGeometryCollectionPainter::visit_main_rendered_layer(
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type)
{
	// We only want access to the main layer type.
	d_paint_params->d_main_rendered_layer_type = main_rendered_layer_type;

	// Use base class implementation.
	return base_type::visit_main_rendered_layer(rendered_geometry_collection, main_rendered_layer_type);
}


boost::optional<GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type>
GPlatesGui::GlobeRenderedGeometryCollectionPainter::get_custom_child_layers_order(
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType parent_layer)
{
	if (parent_layer == GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER)
	{
		if (d_visual_layers_reversed)
		{
			return GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type(
					d_visual_layers.get_layer_order().rbegin(),
					d_visual_layers.get_layer_order().rend());
		}
		else
		{
			return d_visual_layers.get_layer_order();
		}
	}
	else
	{
		return boost::none;
	}
}


void
GPlatesGui::GlobeRenderedGeometryCollectionPainter::set_visual_layers_reversed(
		bool reversed)
{
	d_visual_layers_reversed = reversed;
}


GPlatesGui::GlobeRenderedGeometryCollectionPainter::PaintParams::PaintParams(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		const double &viewport_zoom_factor,
		GlobeRenderedGeometryLayerPainter::PaintRegionType paint_region,
		boost::optional<GPlatesOpenGL::GLIntersect::Plane> globe_horizon_plane,
		boost::optional<Colour> vector_geometries_override_colour,
		bool improve_performance_reduce_quality_hint) :
	d_gl(&gl),
	d_view_projection(view_projection),
	d_inverse_viewport_zoom_factor(1.0 / viewport_zoom_factor),
	d_paint_region(paint_region),
	d_globe_horizon_plane(globe_horizon_plane),
	d_vector_geometries_override_colour(vector_geometries_override_colour),
	d_improve_performance_reduce_quality_hint(improve_performance_reduce_quality_hint),
	d_cache_handle(new std::vector<cache_handle_type>()),
	// Default to RECONSTRUCTION_LAYER (we set it before visiting each layer anyway) ...
	d_main_rendered_layer_type(GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER)
{
}
