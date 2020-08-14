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

#include "gui/Map.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLVisualLayers.h"


namespace GPlatesGui
{
	class TextOverlay;
	class VelocityLegendOverlay;
	class ViewportZoom;
}

namespace GPlatesOpenGL
{
	class GL;
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
				GPlatesGui::ViewportZoom &viewport_zoom,
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
		 * Sets the viewport transform used for rendering this canvas using OpenGL.
		 *
		 * It is based on the viewport dimensions in device pixels (not device *independent* pixels).
		 * Note that Qt uses device *independent* coordinates (eg, in the QGraphicsView/QGraphicsScene/QPainter).
		 * Whereas OpenGL uses device pixels (affected by device pixel ratio on high DPI displays like Apple Retina).
		 */
		void
		set_viewport_transform_for_device_pixels(
				const QTransform &viewport_transform);

		/**
		 * Renders the scene to a QImage of the dimensions specified by @a image_size.
		 *
		 * The paint device is the QGLWidget set as the viewport on MapView (on QGraphicsView).
		 *
		 * @a image_size is in pixels (not widget size). If the caller is rendering a high-DPI image
		 * they should multiply their widget size by the appropriate device pixel ratio and then call
		 * QImage::setDevicePixelRatio on the returned image.
		 *
		 * Returns a null QImage if unable to allocate enough memory for the image data.
		 */
		QImage
		render_to_qimage(
				QPaintDevice &map_canvas_paint_device,
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
				QPaintDevice &map_canvas_paint_device,
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

		//! Viewport transform used for rendering this canvas using OpenGL (in device pixels).
		QTransform d_viewport_transform_for_device_pixels;

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
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLTileRender &tile_render,
				QImage &image,
				const GPlatesOpenGL::GLMatrix &projection_matrix_scene,
				const GPlatesOpenGL::GLMatrix &projection_matrix_text_overlay,
				const QPaintDevice &map_canvas_paint_device);

		//! Render onto the canvas.
		cache_handle_type
		render_scene(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLMatrix &projection_matrix_scene,
				const GPlatesOpenGL::GLMatrix &projection_matrix_text_overlay,
				const QPaintDevice &paint_device,
				const QPaintDevice &map_canvas_paint_device);

		//! Calculate scaling for lines, points and text based on size of view
		float
		calculate_scale(
				const QPaintDevice &paint_device,
				const QPaintDevice &map_canvas_paint_device);

	};
}

#endif // GPLATES_QTWIDGETS_MAPCANVAS_H
