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

#ifndef GPLATES_VIEW_OPERATIONS_CHANGELIGHTDIRECTIONOPERATION_H
#define GPLATES_VIEW_OPERATIONS_CHANGELIGHTDIRECTIONOPERATION_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "RenderedGeometryCollection.h"

#include "RenderedGeometryProximity.h"
#include "RenderedRadialArrow.h"

#include "maths/PointOnSphere.h"

#include "gui/Colour.h"


namespace GPlatesGui
{
	class SceneLightingParameters;
	class SimpleGlobeOrientation;
	class ViewportZoom;
}

namespace GPlatesViewOperations
{
	/**
	 * Enables users to drag the light direction to a new location/direction.
	 */
	class ChangeLightDirectionOperation :
			private boost::noncopyable
	{
	public:

		ChangeLightDirectionOperation(
				GPlatesGui::SceneLightingParameters &scene_lighting_parameters,
				GPlatesGui::SimpleGlobeOrientation &globe_orientation,
				GPlatesGui::ViewportZoom &viewport_zoom,
				RenderedGeometryCollection &rendered_geometry_collection,
				RenderedGeometryCollection::MainLayerType main_rendered_layer_type);

		//! Activate this operation.
		void
		activate();

		//! Deactivate this operation.
		void
		deactivate();


		//! The mouse has moved but it is not a drag because mouse button is not pressed.
		void
		mouse_move(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

		//! User has just clicked and dragged on the sphere.
		void
		start_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

		//! User is currently in the middle of dragging the mouse.
		void
		update_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

		//! User has released mouse button after dragging.
		void
		end_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

	private:

		//! Colour to use for highlighting the light direction arrow.
		static const GPlatesGui::Colour ARROW_HIGHLIGHT_COLOUR;
		//! Colour to use for highlighting the light direction symbol.
		static const GPlatesGui::Colour SYMBOL_HIGHLIGHT_COLOUR;

		//! Colour to use when *not* highlighting the light direction arrow.
		static const GPlatesGui::Colour ARROW_UNHIGHLIGHT_COLOUR;
		//! Colour to use when *not* highlighting the light direction symbol.
		static const GPlatesGui::Colour SYMBOL_UNHIGHLIGHT_COLOUR;

		static const float ARROW_PROJECTED_LENGTH;
		static const float ARROW_HEAD_PROJECTED_SIZE;
		static const float RATIO_ARROW_LINE_WIDTH_TO_ARROW_HEAD_SIZE;
		static const RenderedRadialArrow::SymbolType SYMBOL_TYPE;


		GPlatesGui::SceneLightingParameters &d_scene_lighting_parameters;

		GPlatesGui::SimpleGlobeOrientation &d_globe_orientation;

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
		 * Rendered geometry layer used for light direction layer.
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_light_direction_layer_ptr;

		/**
		 * Did the user click on the light direction and is currently dragging it.
		 */
		bool d_is_dragging_light_direction;


		void
		create_rendered_geometry_layers();

		GPlatesMaths::UnitVector3D
		get_world_space_light_direction() const;

		/**
		 * Increase the closeness inclusion threshold from point width to arrowhead width.
		 */
		double
		adjust_closeness_inclusion_threshold(
				const double &closeness_inclusion_threshold);

		bool
		test_proximity_to_light_direction(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

		void
		move_light_direction(
				const GPlatesMaths::UnitVector3D &world_space_light_direction);

		void
		render_light_direction(
				bool highlight);
	};
}

#endif // GPLATES_VIEW_OPERATIONS_CHANGELIGHTDIRECTIONOPERATION_H
