/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_VIEWPORTZOOM_H
#define GPLATES_GUI_VIEWPORTZOOM_H

namespace GPlatesGui
{
	/**
	 * This class encapsulates the behaviour of the zooming-in and zooming-out of the Viewport.
	 */
	class ViewportZoom
	{
	public:

		static const int s_min_zoom_level;

		static const int s_max_zoom_level;

		static const int s_initial_zoom_level;

		static const double s_min_zoom_percent;

		static const double s_max_zoom_percent;

		ViewportZoom() :
			d_zoom_level(s_initial_zoom_level)
		{
			update_zoom_percent_from_level();
		}

		~ViewportZoom()
		{  }

		double
		zoom_percent() const
		{
			return d_zoom_percent;
		}

		double
		zoom_factor() const
		{
			return (d_zoom_percent / 100.0);
		}

		void
		zoom_in();

		void
		zoom_out();

		void
		reset_zoom();

		void
		set_zoom(
				double new_zoom_percent);

	private:

		void
		update_zoom_percent_from_level();

		/**
		 * Zoom level affects the zoom thusly:
		 *  zoom_power := zoom_level * zoom_delta + 1.0
		 *
		 * The zoom level is an integer value which lies within the range [0, MAX_LEVEL].
		 * (The value of MAX_LEVEL is determined by the max zoom power.)
		 *
		 * Observe that:
		 *  - zoom level of 0 => zoom power of 1.0 => no "zoom".
		 *  - zoom level of MAX_LEVEL => zoom power of 3.0 => 100x "zoom".
		 *
		 * The zoom power affects the zoom thusly:
		 *  zoom_percent := pow(10.0, (zoom_power + 1))
		 *
		 * The zoom power is a floating-point value which lies within the range [1.0, 3.0].
		 * (The value 3.0 is determined by the max zoom percent of 10000.0.)
		 *
		 * Observe that:
		 * - zoom power of 1.0 => zoom percent of 100.0 => no "zoom".
		 * - zoom power of 3.0 => zoom percent of 10000.0 => 100x "zoom".
		 *
		 * The zoom power will be incremented in deltas of 0.05, meaning there will be 40
		 * deltas between the minimum zoom power and the maximum zoom power.
		 */
		int d_zoom_level;

		/**
		 * This is the intuitive "zoom percent".
		 *
		 * GPlates allows zoom percents in the range [100.0, 10000.0].
		 */
		double d_zoom_percent;

		/**
		 * Whether the current zoom percent is in sync with the current zoom level value.
		 *
		 * When the client code zooms in, zooms out or resets the zoom, the zoom level is
		 * incremented, decremented or reset; then the zoom percent is updated from the
		 * zoom level.  This means the zoom percent is in sync with the current zoom level
		 * value.
		 *
		 * On the other hand, when the client code specifies the zoom percent specifically,
		 * the zoom level is bypassed (since most zoom percent values do not correspond to
		 * any zoom level value), which means the zoom percent is _not_ in sync with the
		 * current zoom level value.
		 */
		bool d_zoom_percent_is_synced_with_level;
	};
}

#endif  /* GPLATES_GUI_VIEWPORTZOOM_H */
