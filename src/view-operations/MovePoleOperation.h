/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_VIEW_OPERATIONS_MOVEPOLEOPERATION_H
#define GPLATES_VIEW_OPERATIONS_MOVEPOLEOPERATION_H

#include <boost/optional.hpp>
#include <QObject>
#include <QPointF>

#include "RenderedGeometryCollection.h"

#include "RenderedGeometryProximity.h"
#include "RenderedRadialArrow.h"

#include "maths/PointOnSphere.h"

#include "gui/Colour.h"

#include "utils/ReferenceCount.h"


namespace GPlatesGui
{
	class MapProjection;
	class ViewportZoom;
}

namespace GPlatesQtWidgets
{
	class MovePoleWidget;
}

namespace GPlatesViewOperations
{
	/**
	 * Enables users to drag the pole to a new location/direction.
	 */
	class MovePoleOperation :
			public QObject,
			public GPlatesUtils::ReferenceCount<MovePoleOperation>
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<MovePoleOperation> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const MovePoleOperation> non_null_ptr_to_const_type;


		//! Create a new @a MovePoleOperation instance.
		static
		non_null_ptr_type
		create(
				GPlatesGui::ViewportZoom &viewport_zoom,
				RenderedGeometryCollection &rendered_geometry_collection,
				RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::MovePoleWidget &move_pole_widget)
		{
			return non_null_ptr_type(
					new MovePoleOperation(
							viewport_zoom,
							rendered_geometry_collection,
							main_rendered_layer_type,
							move_pole_widget));
		}

		//! Activate this operation.
		virtual
		void
		activate();

		//! Deactivate this operation.
		virtual
		void
		deactivate();


		//! The mouse has moved (in globe view) but it is not a drag because mouse button is not pressed.
		void
		mouse_move_on_globe(
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				const double &closeness_inclusion_threshold);

		//! The mouse has moved (in map view) but it is not a drag because mouse button is not pressed.
		void
		mouse_move_on_map(
				const QPointF &current_point_on_scene,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				const GPlatesGui::MapProjection &map_projection);

		/**
		 * User has just clicked and dragged on the globe.
		 *
		 * Returns false if mouse cursor is not near pole, or if cannot change pole location
		 * (eg, because pole location constrained to focused feature stage pole location).
		 */
		bool
		start_drag_on_globe(
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				const double &closeness_inclusion_threshold);

		/**
		 * User has just clicked and dragged on the map.
		 *
		 * Returns false if mouse cursor is not near pole, or if cannot change pole location
		 * (eg, because pole location constrained to focused feature stage pole location).
		 */
		bool
		start_drag_on_map(
				const QPointF &initial_point_on_scene,
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				const GPlatesGui::MapProjection &map_projection);

		//! User is currently in the middle of dragging the mouse.
		void
		update_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

		//! User has released mouse button after dragging.
		void
		end_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

	private Q_SLOTS:

		void
		react_pole_changed();

	private:

		//! Colour to use for highlighting the pole arrow.
		static const GPlatesGui::Colour ARROW_HIGHLIGHT_COLOUR;
		//! Colour to use for highlighting the pole symbol.
		static const GPlatesGui::Colour SYMBOL_HIGHLIGHT_COLOUR;

		//! Colour to use when *not* highlighting the pole arrow.
		static const GPlatesGui::Colour ARROW_UNHIGHLIGHT_COLOUR;
		//! Colour to use when *not* highlighting the pole symbol.
		static const GPlatesGui::Colour SYMBOL_UNHIGHLIGHT_COLOUR;

		static const float ARROW_PROJECTED_LENGTH;
		static const float ARROW_HEAD_PROJECTED_SIZE;
		static const float RATIO_ARROW_LINE_WIDTH_TO_ARROW_HEAD_SIZE;
		static const RenderedRadialArrow::SymbolType SYMBOL_TYPE;
		static const float SYMBOL_SIZE;


		GPlatesGui::ViewportZoom &d_viewport_zoom;

		/**
		 * This is where we render our geometries and activate our render layer.
		 */
		RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * The main rendered layer we're currently rendering into.
		 */
		RenderedGeometryCollection::MainLayerType d_main_rendered_layer_type;

		/**
		 * Used to get and set the pole location.
		 */
		GPlatesQtWidgets::MovePoleWidget &d_move_pole_widget;

		/**
		 * Rendered geometry layer used for pole location.
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_pole_layer_ptr;

		/**
		 * Did the user click on the pole and is currently dragging it.
		 */
		bool d_is_dragging_pole;


		MovePoleOperation(
				GPlatesGui::ViewportZoom &viewport_zoom,
				RenderedGeometryCollection &rendered_geometry_collection,
				RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::MovePoleWidget &move_pole_widget);

		void
		create_rendered_geometry_layers();

		/**
		 * Increase the closeness inclusion threshold from point width to arrowhead width.
		 */
		double
		adjust_closeness_inclusion_threshold(
				const double &closeness_inclusion_threshold);

		bool
		test_proximity_to_pole_on_globe(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

		bool
		test_proximity_to_pole_on_map(
				const QPointF &point_on_scene,
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				const GPlatesGui::MapProjection &map_projection);

		void
		move_pole(
				const GPlatesMaths::PointOnSphere &pole);

		void
		render_pole(
				bool highlight);
	};
}

#endif // GPLATES_VIEW_OPERATIONS_MOVEPOLEOPERATION_H
