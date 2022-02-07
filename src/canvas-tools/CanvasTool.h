/* $Id$ */

/**
 * @file 
 * Contains CanvasTool base class, and status bar listeners
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include <boost/function.hpp>
#include <boost/optional.hpp>

#include "maths/PointOnSphere.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


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
			public GPlatesUtils::ReferenceCount<CanvasTool>
	{
	public:

		virtual
		~CanvasTool()
		{  }

		/**
		 * Typedef for a function that takes a C string and displays it on the status bar.
		 */
		typedef boost::function< void ( const char * ) > status_bar_callback_type;

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<CanvasTool>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<CanvasTool> non_null_ptr_type;

		virtual
		void
		handle_activation()
		{  }

		virtual
		void
		handle_deactivation()
		{  }

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{  }
		
		virtual
		void
		handle_left_press(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{  }

		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
		{  }

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
		{  }

		virtual
		void
		handle_shift_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{  }

		virtual
		void
		handle_shift_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
		{  }

		virtual
		void
		handle_shift_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
		{  }

		virtual
		void
		handle_ctrl_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{  }

		virtual
		bool
		handle_ctrl_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
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
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
		{
			return true; // perform default behaviour (rotate globe)
		}

		virtual
		void
		handle_shift_ctrl_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{  }

		virtual
		bool
		handle_shift_ctrl_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
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
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
		{
			return true; // perform default behaviour (rotate globe)
		}

		virtual
		void
		handle_move_without_drag(	
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold)
		{  }

	protected:

		/**
		 * Constructs CanvasTool, given @a status_bar_callback that can be used by the
		 * canvas tool to set status bar messages.
		 */
		explicit
		CanvasTool(
				const status_bar_callback_type &status_bar_callback) :
			d_status_bar_callback(status_bar_callback)
		{  }

		/**
		 * Subclasses call this function to set text on the status bar.
		 *
		 * Note that @a message should *not* have been translated, i.e. passed
		 * through QObject::tr().
		 */
		void
		set_status_bar_message(
				const char *message)
		{
			if (d_status_bar_callback)
			{
				d_status_bar_callback(message);
			}
		}

	private:

		//! The callback used to show text on the status bar.
		status_bar_callback_type d_status_bar_callback;
	};

}

#endif  // GPLATES_CANVASTOOLS_CANVASTOOL_H
