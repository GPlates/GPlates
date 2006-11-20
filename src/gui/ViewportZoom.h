/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

namespace GPlatesGui {

	/**
	 * This class encapsulates the behaviour of the zooming-in and -out of
	 * the Viewport.
	 */
	class ViewportZoom {

		static const double m_min_zoom_power;

		static const double m_max_zoom_power;

		static const double m_zoom_power_delta;

		static const double m_initial_zoom_power;

	 public:

		ViewportZoom() :
		 m_zoom_power(m_initial_zoom_power) {

			update_zoom_params();
		}

		~ViewportZoom() {  }

		unsigned
		zoom_percent() const {

			return m_zoom_percent;
		}

		double
		zoom_factor() const {

			return m_zoom_factor;
		}

		void
		zoom_in();

		void
		zoom_out();

		void
		reset_zoom();

	 private:

		void
		update_zoom_params();

		/**
		 * The zoom power affects the zoom thusly:
		 *  zoom_percent = pow(10.0, (zoom_power + 1))
		 *
		 * The zoom power must lie within the range [1.0, 2.0].
		 * Thus (in particular):
		 * - zoom power of 1.0 => zoom percent of 100.0 => no "zoom".
		 * - zoom power of 2.0 => zoom percent of 1000.0 => 10x "zoom".
		 */
		double   m_zoom_power;

		unsigned m_zoom_percent;

		double   m_zoom_factor;

	};

}

#endif  /* GPLATES_GUI_VIEWPORTZOOM_H */
