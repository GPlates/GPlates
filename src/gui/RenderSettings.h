/* $Id$ */

/**
 * @file 
 * Holds parameters to be used by GlobeRenderedGeometryLayerPainter
 * (allows us to avoid passing in the Globe into that class)
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_RENDERSETTINGS_H
#define GPLATES_GUI_RENDERSETTINGS_H

#include <QObject>

namespace GPlatesGui
{
	class RenderSettings :
		public QObject
	{

		Q_OBJECT

	public:

		RenderSettings() :
			d_show_points(true),
			d_show_multipoints(true),
			d_show_lines(true),
			d_show_polygons(true),
			// NOTE: By default, topological sections are hidden.
			// These are features referenced by topologies (as topological sections) for *all* reconstruction times.
			// As soon as a topology is loaded that references an already loaded feature, that feature then
			// becomes a topological section.
			// Most users don't want to see these 'dangling bits' around topologies
			// (ie, they just want to see the topologies). The small percentage of users who actually
			// build topologies will have to turn this on manually...
			d_show_topological_sections(false),
			d_show_velocity_arrows(true),
			d_show_strings(true)
		{  }

		RenderSettings(
				bool show_points_,
				bool show_multipoints_,
				bool show_lines_,
				bool show_polygons_,
				bool show_topological_sections_,
				bool show_arrows_,
				bool show_strings_) :
			d_show_points(show_points_),
			d_show_multipoints(show_multipoints_),
			d_show_lines(show_lines_),
			d_show_polygons(show_polygons_),
			d_show_topological_sections(show_topological_sections_),
			d_show_velocity_arrows(show_arrows_),
			d_show_strings(show_strings_)
		{  }

		bool show_points() const { return d_show_points; }
		bool show_multipoints() const { return d_show_multipoints; }
		bool show_lines() const { return d_show_lines; }
		bool show_polygons() const { return d_show_polygons; }
		bool show_topological_sections() const { return d_show_topological_sections; }
		bool show_velocity_arrows() const { return d_show_velocity_arrows; }
		bool show_strings() const { return d_show_strings; }

		void set_show_points(bool b) { d_show_points = b; Q_EMIT settings_changed(); }
		void set_show_multipoints(bool b) { d_show_multipoints = b; Q_EMIT settings_changed(); }
		void set_show_lines(bool b) { d_show_lines = b; Q_EMIT settings_changed(); }
		void set_show_polygons(bool b) { d_show_polygons = b; Q_EMIT settings_changed(); }
		void set_show_topological_sections(bool b) { d_show_topological_sections = b; Q_EMIT settings_changed(); }
		void set_show_velocity_arrows(bool b) { d_show_velocity_arrows = b; Q_EMIT settings_changed(); }
		void set_show_strings(bool b) { d_show_strings = b; Q_EMIT settings_changed(); }
	
	Q_SIGNALS:

		void
		settings_changed();

	private:

		bool d_show_points;
		bool d_show_multipoints;
		bool d_show_lines;
		bool d_show_polygons;
		bool d_show_topological_sections;
		bool d_show_velocity_arrows;
		bool d_show_strings;

	};

}

#endif  // GPLATES_GUI_RENDERSETTINGS_H
