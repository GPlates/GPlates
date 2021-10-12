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

#include <boost/shared_ptr.hpp>
#include <QGraphicsScene>

#include "gui/MapProjection.h"
#include "gui/RenderSettings.h"
#include "gui/TextRenderer.h"
#include "gui/ViewportZoom.h"
#include "view-operations/RenderedGeometryCollection.h"

namespace GPlatesQtWidgets
{

	class MapCanvas: 
		public QGraphicsScene
	{
	Q_OBJECT
	public:
		MapCanvas(
			GPlatesViewOperations::RenderedGeometryCollection &collection,
			GPlatesGui::ViewportZoom &viewport_zoom);

		const GPlatesGui::MapProjection &
		projection(){
			return d_map_projection;
		}

		void
		set_projection_type(
			GPlatesGui::ProjectionType projection_type);

		GPlatesGui::ProjectionType
		projection_type();

		void
		set_central_meridian(
			double central_meridian);

		double
		central_meridian();

		void
		draw_svg_output();

		/** 
		* Functions to change and examine display state variables
		*/ 
		void enable_point_display()			{ d_render_settings.show_points = true; }
		void enable_line_display()			{ d_render_settings.show_lines = true; }
		void enable_polygon_display() 		{ d_render_settings.show_polygons = true; }
		void enable_topology_display() 		{ d_render_settings.show_topology = true; }
		void enable_multipoint_display()	{ d_render_settings.show_multipoints = true; }
		void enable_arrows_display()		{ d_render_settings.show_arrows = true; }
		void enable_strings_display()		{ d_render_settings.show_strings = true; }

		void disable_point_display() 		{ d_render_settings.show_points = false; }
		void disable_line_display() 		{ d_render_settings.show_lines = false; }
		void disable_polygon_display() 		{ d_render_settings.show_polygons = false; }
		void disable_topology_display() 	{ d_render_settings.show_topology = false; }
		void disable_multipoint_display()	{ d_render_settings.show_multipoints = false; }
		void disable_arrows_display()		{ d_render_settings.show_arrows = false; }
		void disable_strings_display()		{ d_render_settings.show_strings = false; }

		void toggle_point_display()			{ d_render_settings.show_points = !d_render_settings.show_points; }
		void toggle_line_display() 			{ d_render_settings.show_lines = !d_render_settings.show_lines; }
		void toggle_polygon_display() 		{ d_render_settings.show_polygons = !d_render_settings.show_polygons; }
		void toggle_topology_display() 		{ d_render_settings.show_topology = !d_render_settings.show_topology; }
		void toggle_multipoint_display()	{ d_render_settings.show_multipoints = !d_render_settings.show_topology; }
		void toggle_arrows_display()		{ d_render_settings.show_arrows = !d_render_settings.show_arrows; }
		void toggle_strings_display()		{ d_render_settings.show_strings = !d_render_settings.show_strings; }
		
		bool point_display_is_enabled()			{ return d_render_settings.show_points; }
		bool line_display_is_enabled()			{ return d_render_settings.show_lines; }
		bool polygon_display_is_enabled()		{ return d_render_settings.show_polygons; }
		bool topology_display_is_enabled()		{ return d_render_settings.show_topology; }
		bool multipoint_display_is_enabled()	{ return d_render_settings.show_multipoints; }
		bool arrows_display_is_enabled()		{ return d_render_settings.show_arrows; }
		bool strings_display_is_enabled()		{ return d_render_settings.show_strings; }

		void
		set_text_renderer(
				GPlatesGui::TextRenderer::ptr_to_const_type text_renderer_ptr)
		{
			d_text_renderer_ptr = text_renderer_ptr;
		}

	public slots:

		void
		update_canvas(
			GPlatesViewOperations::RenderedGeometryCollection &collection,
			GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type update_type);

	private:
		
		/**
		* Flags to determine what data to show
		*/
		GPlatesGui::RenderSettings d_render_settings;
			// FIXME: be sure to synchronise default RenderSettings with ViewportWidgetUi.ui

		/**
		 * A virtual override of the QGraphicsScene function. 
		 *
		 * This sets the background colour and draws the lat-lon grid. 
		 */
		void
		drawBackground(
			QPainter *painter,
			const QRectF &rect);

		/**
		 * Draw lat-lon grid lines on the canvas, at 30-degree intervals. 
		 */
		void
		draw_grid_lines();

		/**
		 * A virtual override of the QGraphicsScene function. 
		 * 
		 * This draws the content of the RenderedGeometryLayers.
		 */ 
		void
		drawItems(
			QPainter *painter,
			int numItems,
			QGraphicsItem *items[],
			const QStyleOptionGraphicsItem options[],
			QWidget *widget = 0);

		
		MapCanvas &
		operator=(
			MapCanvas &other);

		MapCanvas(
			const MapCanvas&);


		GPlatesGui::MapProjection d_map_projection;

		/**
		 * A pointer to the ViewportWindow's RenderedGeometryCollection. 
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geometry_collection;

		GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type d_update_type;

		/**
		 * Used for rendering text
		 */
		GPlatesGui::TextRenderer::ptr_to_const_type d_text_renderer_ptr;
		
		/**
		 * For zoom-dependent rendered objects.                                                                     
		 */
		GPlatesGui::ViewportZoom &d_viewport_zoom;		
		
	};

}


#endif // GPLATES_QTWIDGETS_MAPCANVAS_H
