/* $Id$ */

/**
 * @file 
 * Contains CanvasTool base class, and status bar listeners
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_CANVASTOOL_H
#define GPLATES_CANVASTOOLS_CANVASTOOL_H

#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "maths/PointOnSphere.h"


namespace GPlatesCanvasTools
{
	/**
	 * Base class for canvas tools that do not need to be implemented differently
	 * for globe and map views.
	 *
	 * Prefer using CanvasTool as the base class over GlobeCanvasTool and
	 * MapCanvasTool if you do not need the extra parameters provided by those.
	 *
	 * Note: the handle_*ctrl* functions all return a boolean value. If the value
	 * returned is true, the default action is performed by the
	 * CanvasTooLAdapterFor{Globe,Map} instance holding the instance of this class.
	 * The default action when the Ctrl key is held down is for the globe to be
	 * rotated or the map to be panned. Return false in these functions if you
	 * wish to suppress this behaviour (rare).
	 *
	 * For other handle_* functions, the default behaviour is to do nothing.
	 *
	 * Clicks off the globe or map: If we are in globe view, the handle_* functions
	 * will be called, with is_on_earth as false and the point_on_sphere as the
	 * nearest point on the horizon. If we are in map view, the handle_* functions
	 * will not be called, as we currently do not have the ability to calculate
	 * the nearest point on the map for clicks off the map.
	 */
	class CanvasTool :
			public boost::noncopyable
	{

	public:

		/**
		 * What view is this instance of CanvasTool being used in?
		 * (Main use: providing context-sensitive status bar messages)
		 */
		enum View
		{
			GLOBE_VIEW,
			MAP_VIEW
		};

		/**
		 * Construct a new instance of CanvasTool. View defaults to globe view.
		 */
		CanvasTool():
			d_view(GLOBE_VIEW)
		{
		}

		/**
		 * Construct a new instance of CanvasTool, specifying whether it is being
		 * used in a globe or a map @a view.
		 */
		explicit
		CanvasTool(
				View view):
			d_view(view)
		{
		}

		virtual
		~CanvasTool()
		{
		}

		virtual
		void
		handle_activation()
		{
		}

		virtual
		void
		handle_deactivation()
		{
		}

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{
		}
		
		virtual
		void
		handle_left_press(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{
		}

		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold)
		{
		}

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold)
		{
		}

		virtual
		void
		handle_shift_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{
		}

		virtual
		void
		handle_shift_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold)
		{
		}

		virtual
		void
		handle_shift_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold)
		{
		}

		virtual
		void
		handle_ctrl_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{
		}

		virtual
		bool
		handle_ctrl_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold)
		{
			return true; // perform default behaviour (rotate globe)
		}

		virtual
		bool
		handle_ctrl_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold)
		{
			return true; // perform default behaviour (rotate globe)
		}

		virtual
		void
		handle_shift_ctrl_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{
		}

		virtual
		bool
		handle_shift_ctrl_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold)
		{
			return true; // perform default behaviour (rotate globe)
		}

		virtual
		bool
		handle_shift_ctrl_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold)
		{
			return true; // perform default behaviour (rotate globe)
		}

		virtual
		void
		handle_move_without_drag(	
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{
		}

		void
		set_view(View view)
		{
			d_view = view;
		}

		View
		get_view()
		{
			return d_view;
		}

		/**
		 * Adds the @a listener for status bar updates.
		 */
		void
		set_status_bar_callback(
				const boost::function< void ( const QString & ) > &callback)
		{
			d_status_bar_callback = callback;
		}

	protected:

		/**
		 * Subclasses call this function to set text on the status bar.
		 * All listeners are notified of the @message.
		 */
		void
		set_status_bar_message(
				const QString &message)
		{
			d_status_bar_callback(message);
		}

	private:

		//! The view that this CanvasTool is being used in (globe or map)
		View d_view;

		//! The callback used to show text on the status bar.
		boost::function< void ( const QString & ) > d_status_bar_callback;
	};

}

#endif  // GPLATES_CANVASTOOLS_CANVASTOOL_H
