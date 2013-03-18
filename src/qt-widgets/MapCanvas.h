/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway.
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
 
#ifndef GPLATES_QTWIDGETS_MAPCANVAS_H
#define GPLATES_QTWIDGETS_MAPCANVAS_H

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <QGLWidget>
#include <QGraphicsScene>
#include <QImage>
#include <QPaintDevice>
#include <QSize>
#include <QTransform>

#include "gui/ColourScheme.h"
#include "gui/Map.h"
#include "gui/QPainterTextRenderer.h"
#include "gui/TextOverlay.h"

#include "opengl/GLContext.h"
#include "opengl/GLVisualLayers.h"


namespace GPlatesGui
{
	class RenderSettings;
	class ViewportZoom;
}

namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLTileRender;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class MapView;

	/**
	 * Responsible for invoking the functions to paint items onto the map.
	 * Note: this is not analogous to GlobeCanvas. The map analogue of GlobeCanvas
	 * is MapView.
	 */
	class MapCanvas : 
			public QGraphicsScene,
			public boost::noncopyable
	{
		Q_OBJECT

	public:

		MapCanvas(
				GPlatesPresentation::ViewState &view_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				MapView *map_view_ptr,
				const GPlatesOpenGL::GLContext::non_null_ptr_type &gl_context,
				const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
				GPlatesGui::RenderSettings &render_settings,
				GPlatesGui::ViewportZoom &viewport_zoom,
				const GPlatesGui::ColourScheme::non_null_ptr_type &colour_scheme,
				QWidget *parent_ = NULL);

		~MapCanvas();

		GPlatesGui::Map &
		map()
		{
			return d_map;
		}

		const GPlatesGui::Map &
		map() const
		{
			return d_map;
		}

		/**
		 * Renders the scene to a QImage of the dimensions specified by @a image_size.
		 *
		 * The paint device is the QGLWidget set as the viewport on MapView (on QGraphicsView).
		 */
		QImage
		render_to_qimage(
				QGLWidget *paint_device,
				const QTransform &viewport_transform,
				const QSize &image_size);

		/**
		 * Paint the scene, as best as possible, by re-directing OpenGL rendering to the specified paint device.
		 *
		 * @a viewport_transform is the view transform of the QGraphicsView initiating the rendering.
		 */
		void
		render_opengl_feedback_to_paint_device(
				QPaintDevice &paint_device,
				const QTransform &viewport_transform);

		void
		set_disable_update(
				bool b);

	Q_SIGNALS:

		void
		repainted();

	public Q_SLOTS:
		
		void
		update_canvas();

	protected:

		/**
		 * A virtual override of the QGraphicsScene function. 
		 */
		void
		drawBackground(
				QPainter *painter,
				const QRectF &rect);

	private:

		/**
		 * Utility class to make the OpenGL context current in @a MapCanvas constructor.
		 *
		 * This is so we can do OpenGL stuff in the @a MapCanvas constructor when normally
		 * we'd have to wait until 'drawBackground()'.
		 */
		struct MakeGLContextCurrent
		{
			explicit
			MakeGLContextCurrent(
					GPlatesOpenGL::GLContext &gl_context)
			{
				gl_context.make_current();
			}
		};

		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		GPlatesPresentation::ViewState &d_view_state;
		MapView *d_map_view_ptr;

		//! Mirrors an OpenGL context and provides a central place to manage low-level OpenGL objects.
		GPlatesOpenGL::GLContext::non_null_ptr_type d_gl_context;
		//! Makes the OpenGL context current in @a GlobeCanvas constructor so it can call OpenGL.
		MakeGLContextCurrent d_make_context_current;

		/**
		 * Enables frame-to-frame caching of persistent OpenGL resources.
		 *
		 * There is a certain amount of caching without this already.
		 * This just prevents a render frame from invalidating cached resources of the previous frame
		 * in order to avoid regenerating the same cached resources unnecessarily each frame.
		 * We hold onto the previous frame's cached resources *while* generating the current frame and
		 * then release our hold on the previous frame (and continue this pattern each new frame).
		 */
		cache_handle_type d_gl_frame_cache_handle;

		/**
		 * Renders text using the QPainter interface.
		 */
		GPlatesGui::TextRenderer::non_null_ptr_type d_text_renderer;

		//! Paints an optional text overlay onto the map.
		boost::scoped_ptr<GPlatesGui::TextOverlay> d_text_overlay;

		//! Holds the state
		GPlatesGui::Map d_map;

		//! A pointer to the state's RenderedGeometryCollection
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geometry_collection;

		bool d_disable_update;


		//! Do some OpenGL initialisation.
		void 
		initializeGL();

		/**
		 * Render one tile of the scene (as specified by @a tile_render).
		 *
		 * The sub-rect of @a image to render into is determined by @a tile_renderer.
		 */
		cache_handle_type
		render_scene_tile_into_image(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesOpenGL::GLTileRender &tile_render,
				QImage &image);

		//! Render onto the canvas.
		cache_handle_type
		render_scene(
				GPlatesOpenGL::GLRenderer &renderer);

		//! Calculate scaling for lines, points and text based on size of view
		float
		calculate_scale();

	};
}

#endif // GPLATES_QTWIDGETS_MAPCANVAS_H
