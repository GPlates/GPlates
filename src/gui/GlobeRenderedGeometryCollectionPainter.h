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

#ifndef GPLATES_GUI_GLOBERENDEREDGEOMETRYPAINTER_H
#define GPLATES_GUI_GLOBERENDEREDGEOMETRYPAINTER_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "ColourScheme.h"
#include "GlobeRenderedGeometryLayerPainter.h"
#include "GlobeVisibilityTester.h"
#include "LayerPainter.h"
#include "RenderSettings.h"

#include "opengl/GLContext.h"
#include "opengl/GLTexture.h"
#include "opengl/GLVisualLayers.h"

#include "presentation/VisualLayers.h"

#include "view-operations/RenderedGeometryCollectionVisitor.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
	class RenderedGeometryLayer;
}

namespace GPlatesGui
{
	/**
	 * Draws rendered geometries (in a @a RenderedGeometryCollection) onto a
	 * 3D orthographic view of the globe using OpenGL.
	 */
	class GlobeRenderedGeometryCollectionPainter :
			private GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor<
				GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type>,
			private boost::noncopyable
	{
	public:
		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		GlobeRenderedGeometryCollectionPainter(
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
				const GPlatesPresentation::VisualLayers &visual_layers,
				RenderSettings &render_settings,
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme);

		/**
		 * Initialise objects requiring @a GLRenderer.
		 */
		void
		initialise(
				GPlatesOpenGL::GLRenderer &renderer);

		/**
		 * Returns true if any rendered layer has sub-surface geometries that can be rendered.
		 *
		 * It's possible that a sub-surface geometry cannot be rendered, for example, if
		 * the runtime system does not support OpenGL 3 (for 3D scalar fields).
		 *
		 * These are painted using @a paint_sub_surface.
		 */
		bool
		has_renderable_sub_surface_geometries(
				GPlatesOpenGL::GLRenderer &renderer) const;

		/**
		 * Draw the rendered geometries on the surface of the globe.
		 *
		 * This includes rendered direction arrows.
		 *
		 * @param viewport_zoom_factor is used for rendering view-dependent geometries.
		 */
		cache_handle_type
		paint_surface(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &viewport_zoom_factor);

		/**
		 * Draw globe sub-surface rendered geometries that exist below the surface of the globe.
		 *
		 * Currently this is 3D scalar fields.
		 *
		 * @param viewport_zoom_factor is used for rendering view-dependent geometries.
		 * @param surface_occlusion_texture is a viewport-size 2D texture containing the RGBA rendering
		 * of the surface geometries/rasters on the *front* of the globe.
		 */
		cache_handle_type
		paint_sub_surface(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &viewport_zoom_factor,
				boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture);

		void
		set_scale(
				float scale);

		virtual
		boost::optional<GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type>
		get_custom_child_layers_order(
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType parent_layer);

		void
		set_visual_layers_reversed(
				bool reversed);

	private:
		virtual
		bool
		visit_rendered_geometry_layer(
				const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer);

		/**
		 * Parameters that are only available when @a paint_surface is called.
		 */
		struct PaintParams
		{
			PaintParams(
					GPlatesOpenGL::GLRenderer &renderer,
					const double &viewport_zoom_factor,
					GlobeRenderedGeometryLayerPainter::PaintRegionType paint_region,
					boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type>
							surface_occlusion_texture = boost::none);

			GPlatesOpenGL::GLRenderer *d_renderer;
			double d_inverse_viewport_zoom_factor;
			GlobeRenderedGeometryLayerPainter::PaintRegionType d_paint_region;
			boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> d_surface_occlusion_texture;

			// Cache of rendered geometry layers.
			boost::shared_ptr<std::vector<cache_handle_type> > d_cache_handle;
		};


		//! Parameters that are only available when @a paint_surface is called.
		boost::optional<PaintParams> d_paint_params;

		const GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;

		const GPlatesPresentation::VisualLayers &d_visual_layers;

		//! Rendering flags to determine what gets shown
		RenderSettings &d_render_settings;

		//! Used to paint the layers.
		LayerPainter d_layer_painter;

		//! Used for determining whether a particular point on the globe is visible
		GlobeVisibilityTester d_visibility_tester;

		//! For assigning colours to RenderedGeometry
		ColourScheme::non_null_ptr_type d_colour_scheme;

		//! When rendering globes that are meant to be a scale copy of another
		float d_scale;

		//! If true, renders child layers in the RECONSTRUCTION_LAYER in reverse order.
		bool d_visual_layers_reversed;
	};
}

#endif // GPLATES_GUI_GLOBERENDEREDGEOMETRYPAINTER_H
