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
#include <boost/noncopyable.hpp>
#include <QGraphicsScene>

#include "gui/Map.h"

namespace GPlatesGui
{
	class ColourScheme;
	class RenderSettings;
	class ViewportZoom;
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
	class MapCanvas: 
		public QGraphicsScene,
		public boost::noncopyable
	{

		Q_OBJECT

	public:

		MapCanvas(
			GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
			GPlatesGui::RenderSettings &render_settings,
			GPlatesGui::ViewportZoom &viewport_zoom,
			boost::shared_ptr<GPlatesGui::ColourScheme> colour_scheme,
			GPlatesPresentation::ViewState &view_state,
			QWidget *parent_ = NULL);

		GPlatesGui::Map &
		map()
		{
			return d_map;
		}

		void
		draw_svg_output();

		void
		set_map_view_ptr(
				MapView *map_view_ptr)
		{
			d_map_view_ptr = map_view_ptr;
		}

	public slots:
		
		void
		update_canvas();

		void
		update_canvas(
			GPlatesViewOperations::RenderedGeometryCollection &collection,
			GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type update_type);

	private:
		
		GPlatesPresentation::ViewState &d_view_state;
		MapView *d_map_view_ptr;

		//! Holds the state
		GPlatesGui::Map d_map;

		/**
		 * A virtual override of the QGraphicsScene function. 
		 */
		void
		drawBackground(
			QPainter *painter,
			const QRectF &rect);

		/**
		 * A virtual override of the QGraphicsScene function. 
		 */ 
		void
		drawItems(
			QPainter *painter,
			int numItems,
			QGraphicsItem *items[],
			const QStyleOptionGraphicsItem options[],
			QWidget *widget = 0);

		//! A pointer to the state's RenderedGeometryCollection
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geometry_collection;

		//! Calculate scaling for lines, points and text based on size of view
		float
		calculate_scale();

	};

}


#endif // GPLATES_QTWIDGETS_MAPCANVAS_H
