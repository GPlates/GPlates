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
 
#ifndef GPLATES_QTWIDGETS_SCENEVIEW_H
#define GPLATES_QTWIDGETS_SCENEVIEW_H

#include "boost/optional.hpp"

namespace GPlatesMaths
{
	class LatLonPoint;
}

namespace GPlatesQtWidgets
{


	class SceneView
	{

	public:

		SceneView()
			{};

		virtual
		~SceneView()
			{};

		virtual
		void
		set_camera_viewpoint(
			const GPlatesMaths::LatLonPoint &llp) = 0;

		virtual
		void
		enable_raster_display()
		{ };

		virtual
		void
		disable_raster_display()
		{ };

		virtual
		void
		handle_zoom_change()
		{ };

		virtual
		boost::optional<GPlatesMaths::LatLonPoint>
		camera_llp() const = 0;

		virtual
		void
		create_svg_output(
			QString filename) = 0;

		virtual
		void
		draw_svg_output() = 0;

		virtual
		void
		update_canvas() = 0;

		virtual
		void
		move_camera_up() = 0;

		virtual
		void
		move_camera_down() = 0;

		virtual
		void
		move_camera_left() = 0;

		virtual
		void
		move_camera_right() = 0;

		virtual
		void
		rotate_camera_clockwise() = 0;

		virtual
		void
		rotate_camera_anticlockwise() = 0;

		virtual
		void
		reset_camera_orientation() = 0;


	private:
		// Make copy and assignment private to prevent copying/assignment
		SceneView(
			const SceneView &other);

		SceneView &
		operator=(
			const SceneView &other);

	};

}


#endif // GPLATES_QTWIDGETS_SCENEVIEW_H
