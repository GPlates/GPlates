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
#include "PersistentOpenGLObjects.h"
#include "RenderSettings.h"

#include "opengl/GLContext.h"
#include "opengl/GLRenderGraphInternalNode.h"
#include "opengl/GLUNurbsRenderer.h"

#include "presentation/VisualLayers.h"

#include "view-operations/RenderedGeometryCollectionVisitor.h"

#include <QGLWidget>

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
	class RenderedGeometryLayer;
}

namespace GPlatesGui
{
	class GlobeRenderedGeometryLayerPainter;
	class GlobeVisibilityTester;
	class RasterColourSchemeMap;

	/**
	 * Draws rendered geometries (in a @a RenderedGeometryCollection) onto a
	 * 3D orthographic view of the globe using OpenGL.
	 */
	class GlobeRenderedGeometryCollectionPainter :
			private GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor<>,
			private boost::noncopyable
	{
	public:

		GlobeRenderedGeometryCollectionPainter(
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
				const GPlatesPresentation::VisualLayers &visual_layers,
				RenderSettings &render_settings,
				RasterColourSchemeMap &raster_colour_scheme_map,
				TextRenderer::ptr_to_const_type text_renderer_ptr,
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme);

		/**
		 * Draw the rendered geometries.
		 *
		 * @param render_graph_node the render graph node that all rendering should be attached to.
		 * @param viewport_zoom_factor is used for rendering view-dependent geometries.
		 */
		void
		paint(
				const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_node,
				const double &viewport_zoom_factor,
				const GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type &nurbs_renderer);

		void
		set_scale(
				float scale)
		{
			d_scale = scale;
		}

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
					const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_collection_node,
					const double &viewport_zoom_factor,
					const GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type &nurbs_renderer) :
				d_render_collection_node(render_collection_node),
				d_inverse_viewport_zoom_factor(1.0 / viewport_zoom_factor),
				d_nurbs_renderer(nurbs_renderer)
			{  }

			GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type d_render_collection_node;
			double d_inverse_viewport_zoom_factor;
			GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type d_nurbs_renderer;
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

		//! Colour schemes for the app-logic layers.
		RasterColourSchemeMap &d_raster_colour_scheme_map;
		
		//! Used for rendering text on an OpenGL canvas
		TextRenderer::ptr_to_const_type d_text_renderer_ptr;

		//! Used for determining whether a particular point on the globe is visible
		GlobeVisibilityTester d_visibility_tester;

		//! For assigning colours to RenderedGeometry
		ColourScheme::non_null_ptr_type d_colour_scheme;

		//! When rendering globes that are meant to be a scale copy of another
		float d_scale;
	};
}

#endif // GPLATES_GUI_GLOBERENDEREDGEOMETRYPAINTER_H
