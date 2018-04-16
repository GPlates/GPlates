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
#include <boost/optional.hpp>
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

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLOffScreenContext.h"
#include "opengl/GLVisualLayers.h"


namespace GPlatesGui
{
	class RenderSettings;
	class TextOverlay;
	class VelocityLegendOverlay;
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
				QGLWidget *gl_widget,
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
				QGLWidget *map_canvas_paint_device,
				const QTransform &viewport_transform,
				const QSize &image_size);

		/**
		 * Paint the scene, as best as possible, by re-directing OpenGL rendering to the
		 * feedback paint device @a feedback_paint_device.
		 *
		 * @a map_canvas_paint_device is the map canvas's OpenGL paint device used for OpenGL rendering.
		 *
		 * @a viewport_transform is the view transform of the QGraphicsView initiating the rendering.
		 */
		void
		render_opengl_feedback_to_paint_device(
				QGLWidget *map_canvas_paint_device,
				const QTransform &viewport_transform,
				QPaintDevice &feedback_paint_device);

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
		 * Used to render to an off-screen frame buffer when outside paint event.
		 *
		 * It's a boost::optional since we don't create it until @a initializeGL is called.
		 */
		boost::optional<GPlatesOpenGL::GLOffScreenContext::non_null_ptr_type> d_gl_off_screen_context;

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

		//! Paints an optional text overlay onto the map.
		boost::scoped_ptr<GPlatesGui::TextOverlay> d_text_overlay;

		//! Paints an optional velocity legend overlay onto the map.
		boost::scoped_ptr<GPlatesGui::VelocityLegendOverlay> d_velocity_legend_overlay;

		//! Holds the state
		GPlatesGui::Map d_map;

		//! A pointer to the state's RenderedGeometryCollection
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geometry_collection;


		//! Do some OpenGL initialisation.
		void 
		initializeGL(
				QGLWidget *gl_widget);

		/**
		 * Render one tile of the scene (as specified by @a tile_render).
		 *
		 * The sub-rect of @a image to render into is determined by @a tile_renderer.
		 */
		cache_handle_type
		render_scene_tile_into_image(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesOpenGL::GLTileRender &tile_render,
				QImage &image,
				const GPlatesOpenGL::GLMatrix &projection_matrix_scene,
				const GPlatesOpenGL::GLMatrix &projection_matrix_text_overlay);

		//! Render onto the canvas.
		cache_handle_type
		render_scene(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesOpenGL::GLMatrix &projection_matrix_scene,
				const GPlatesOpenGL::GLMatrix &projection_matrix_text_overlay,
				int paint_device_width,
				int paint_device_height);

		//! Calculate scaling for lines, points and text based on size of view
		float
		calculate_scale(
				int paint_device_width,
				int paint_device_height);

	};
}

#endif // GPLATES_QTWIDGETS_MAPCANVAS_H
