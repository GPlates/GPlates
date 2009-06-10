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

#include <QGraphicsScene>

#include "gui/MapProjection.h"
#include "view-operations/RenderedGeometryCollection.h"

namespace GPlatesQtWidgets
{

	class MapCanvas: 
		public QGraphicsScene
	{
	Q_OBJECT
	public:
		MapCanvas(
			GPlatesViewOperations::RenderedGeometryCollection &collection);

		const GPlatesGui::MapProjection &
		projection(){
			return d_map_projection;
		}

		void
		set_projection_type(
			int projection_type);

		int
		projection_type();

		void
		set_central_meridian(
			double central_meridian);

		double
		central_meridian();

		void
		draw_svg_output();

		/** 
		* Functions to change display state variables
		*/ 
		void enable_point_display()			{ d_show_point		= true; }
		void enable_line_display()			{ d_show_line 		= true; }
		void enable_polygon_display() 		{ d_show_polygon 	= true; }
		void enable_topology_display() 		{ d_show_topology	= true; }
		void enable_multipoint_display()	{ d_show_multipoint	= true; }
		void enable_arrows_display()		{ d_show_arrows		= true; }

		void disable_point_display() 		{ d_show_point 		= false; }
		void disable_line_display() 		{ d_show_line 		= false; }
		void disable_polygon_display() 		{ d_show_polygon 	= false; }
		void disable_topology_display() 	{ d_show_topology 	= false; }
		void disable_multipoint_display()	{ d_show_multipoint	= false; }
		void disable_arrows_display()		{ d_show_arrows		= false; }

		void toggle_point_display()			{ d_show_point 		= !d_show_point; }
		void toggle_line_display() 			{ d_show_line 		= !d_show_line; }
		void toggle_polygon_display() 		{ d_show_polygon 	= !d_show_polygon; }
		void toggle_topology_display() 		{ d_show_topology 	= !d_show_topology; }
		void toggle_multipoint_display()	{ d_show_multipoint	= !d_show_multipoint; }
		void toggle_arrows_display()		{ d_show_arrows		= !d_show_arrows; }

		/**
		* Flags to determine what data to show
		*/
		bool d_show_point;
		bool d_show_line;
		bool d_show_polygon;
		bool d_show_topology;
		bool d_show_multipoint;
		bool d_show_arrows;

	public slots:

		void
		update_canvas(
			GPlatesViewOperations::RenderedGeometryCollection &collection,
			GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type update_type);

	private:

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

	};

}


#endif // GPLATES_QTWIDGETS_MAPCANVAS_H
