/* $Id$ */

/**
 * @file 
 * Contains definition of CreateSmallCircle
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#ifndef GPLATES_CANVASTOOLS_CREATESMALLCIRCLE_H
#define GPLATES_CANVASTOOLS_CREATESMALLCIRCLE_H

#include <QObject>

#include "CanvasTool.h"

#include "qt-widgets/SmallCircleWidget.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/RenderedGeometryCollection.h"

namespace GPlatesGui
{
	class Colour;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesCanvasTools
{
	/**
	 * Canvas tool used to measuring distances on the globe and map
	 */
	class CreateSmallCircle :
			public QObject,
			public CanvasTool
	{
		Q_OBJECT

	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<CreateSmallCircle>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<CreateSmallCircle> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesQtWidgets::SmallCircleWidget &small_circle_widget)
		{
			return new CreateSmallCircle(
					status_bar_callback,
					rendered_geom_collection,
					small_circle_widget);
		}

		virtual
		void
		handle_activation();


		virtual
		void
		handle_deactivation();

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

		virtual
		void
		handle_move_without_drag(	
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

		/**
		 * We'll use shift-left-click to continue drawing an additional circle after closing 
		 * the current circle, so that we can build up multiple concentric circles in the same
		 * operation.
		 */
		virtual
		void
		handle_shift_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere, 
				bool is_on_earth, 
				double proximity_inclusion_threshold);

	private slots:

		/**
		 *  Respond to the widget's clear signal.                                                                    
		 */
		void
		handle_clear_geometries();

	private:

		void
		paint();

		CreateSmallCircle(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesQtWidgets::SmallCircleWidget &small_circle_widget);

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection_ptr;

		//! Small circle layer
		GPlatesViewOperations::RenderedGeometryLayer *d_small_circle_layer_ptr;

		boost::optional<GPlatesMaths::PointOnSphere> d_centre;

		boost::optional<GPlatesMaths::PointOnSphere> d_point_on_radius;

		GPlatesQtWidgets::SmallCircleWidget *d_small_circle_widget_ptr;

		GPlatesQtWidgets::SmallCircleWidget::small_circle_collection_type &d_small_circle_collection_ref;

		bool d_circle_is_being_drawn;

	};
}

#endif  // GPLATES_CANVASTOOLS_CREATESMALLCIRCLE_H
