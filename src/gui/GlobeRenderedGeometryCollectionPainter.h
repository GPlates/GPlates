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
#include "GlobeVisibilityTester.h"
#include "LayerPainter.h"
#include "PersistentOpenGLObjects.h"
#include "RenderSettings.h"
#include "TextRenderer.h"

#include "opengl/GLContext.h"

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
				const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
				const GPlatesPresentation::VisualLayers &visual_layers,
				RenderSettings &render_settings,
				const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme);

		/**
		 * Draw the rendered geometries.
		 *
		 * @param viewport_zoom_factor is used for rendering view-dependent geometries.
		 */
		cache_handle_type
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &viewport_zoom_factor);

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
		 * Parameters that are only available when @a paint is called.
		 */
		struct PaintParams
		{
			PaintParams(
					GPlatesOpenGL::GLRenderer &renderer,
					const double &viewport_zoom_factor) :
				d_renderer(&renderer),
				d_inverse_viewport_zoom_factor(1.0 / viewport_zoom_factor),
				d_cache_handle(new std::vector<cache_handle_type>())
			{  }

			GPlatesOpenGL::GLRenderer *d_renderer;
			double d_inverse_viewport_zoom_factor;

			// Cache of rendered geometry layers.
			boost::shared_ptr<std::vector<cache_handle_type> > d_cache_handle;
		};


		//! Parameters that are only available when @a paint is called.
		boost::optional<PaintParams> d_paint_params;

		const GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		PersistentOpenGLObjects::non_null_ptr_type d_persistent_opengl_objects;

		const GPlatesPresentation::VisualLayers &d_visual_layers;

		//! Rendering flags to determine what gets shown
		RenderSettings &d_render_settings;

		//! Used for rendering text on an OpenGL canvas
		TextRenderer::non_null_ptr_to_const_type d_text_renderer_ptr;

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
