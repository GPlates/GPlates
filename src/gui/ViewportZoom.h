/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_VIEWPORTZOOM_H
#define GPLATES_GUI_VIEWPORTZOOM_H

#include <QObject>

namespace GPlatesGui
{
	/**
	 * This class encapsulates the behaviour of the zooming-in and zooming-out of the Viewport.
	 */
	class ViewportZoom :
			public QObject
	{
		Q_OBJECT
		
	public:

		static const int s_min_zoom_level;
		static const int s_max_zoom_level;

		static const double s_min_zoom_percent;
		static const double s_max_zoom_percent;

		ViewportZoom();

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

		/**
		 * The zoom level is related to the zoom percent in the following manner:
		 *
		 *    zoom percent = pow(10.0, (level - min_zoom_level) / (max_zoom_level - min_zoom_level) *
		 *                             (max_zoom_power - min_zoom_power) + min_zoom_power)
		 *
		 * where min_zoom_power and max_zoom_power are the logarithms (base 10) of
		 * min_zoom_percent and max_zoom_percent respectively.
		 *
		 * That is, the zoom level = O(log(zoom percent)).
		 *
		 * Note that a zoom level of min_zoom_level corresponds to a zoom percent
		 * of min_zoom_percent and a zoom level of max_zoom_level corresponds to
		 * a zoom percent of max_zoom_percent.
		 */
		double
		zoom_level() const;
	
	public Q_SLOTS:

		void
		zoom_in(
				double num_levels = 1.0);

		void
		zoom_out(
				double num_levels = 1.0);

		void
		reset_zoom();

		void
		set_zoom_percent(
				double new_zoom_percent);

		void
		set_zoom_level(
				double new_zoom_level);
		
	Q_SIGNALS:
	
		/**
		 * This signal should only be emitted if the zoom is actually different to what it
		 * was.
		 */
		void
		zoom_changed();

		void
		send_zoom_to_stdout(
			double zoom);

	private:

		static
		double
		min_zoom_power();

		static
		double
		max_zoom_power();

		/**
		 * This is the intuitive "zoom percent".
		 *
		 * GPlates allows zoom percents in the range [100.0, 10000.0].
		 */
		double d_zoom_percent;
	};
}

#endif  // GPLATES_GUI_VIEWPORTZOOM_H
