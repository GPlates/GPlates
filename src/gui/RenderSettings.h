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
			d_show_static_points(true),
			d_show_static_multipoints(true),
			d_show_static_lines(true),
			d_show_static_polygons(true),
			d_show_topological_sections(true),
			d_show_topological_lines(true),
			d_show_topological_polygons(true),
			d_show_topological_networks(true),
			d_show_velocity_arrows(true),
			d_show_rasters(true),
			d_show_3d_scalar_fields(true),
			d_show_scalar_coverages(true),
			d_show_strings(true)
		{  }

		RenderSettings(
				bool show_static_points_,
				bool show_static_multipoints_,
				bool show_static_lines_,
				bool show_static_polygons_,
				bool show_topological_sections_,
				bool show_topological_lines_,
				bool show_topological_polygons_,
				bool show_topological_networks_,
				bool show_velocity_arrows_,
				bool show_rasters_,
				bool show_3d_scalar_fields_,
				bool show_scalar_coverages_,
				bool show_strings_) :
			d_show_static_points(show_static_points_),
			d_show_static_multipoints(show_static_multipoints_),
			d_show_static_lines(show_static_lines_),
			d_show_static_polygons(show_static_polygons_),
			d_show_topological_sections(show_topological_sections_),
			d_show_topological_lines(show_topological_lines_),
			d_show_topological_polygons(show_topological_polygons_),
			d_show_topological_networks(show_topological_networks_),
			d_show_velocity_arrows(show_velocity_arrows_),
			d_show_rasters(show_rasters_),
			d_show_3d_scalar_fields(show_3d_scalar_fields_),
			d_show_scalar_coverages(show_scalar_coverages_),
			d_show_strings(show_strings_)
		{  }

		bool show_static_points() const { return d_show_static_points; }
		bool show_static_multipoints() const { return d_show_static_multipoints; }
		bool show_static_lines() const { return d_show_static_lines; }
		bool show_static_polygons() const { return d_show_static_polygons; }
		bool show_topological_sections() const { return d_show_topological_sections; }
		bool show_topological_lines() const { return d_show_topological_lines; }
		bool show_topological_polygons() const { return d_show_topological_polygons; }
		bool show_topological_networks() const { return d_show_topological_networks; }
		bool show_velocity_arrows() const { return d_show_velocity_arrows; }
		bool show_rasters() const { return d_show_rasters; }
		bool show_3d_scalar_fields() const { return d_show_3d_scalar_fields; }
		bool show_scalar_coverages() const { return d_show_scalar_coverages; }
		bool show_strings() const { return d_show_strings; }

		void set_show_static_points(bool b) { d_show_static_points = b; Q_EMIT settings_changed(); }
		void set_show_static_multipoints(bool b) { d_show_static_multipoints = b; Q_EMIT settings_changed(); }
		void set_show_static_lines(bool b) { d_show_static_lines = b; Q_EMIT settings_changed(); }
		void set_show_static_polygons(bool b) { d_show_static_polygons = b; Q_EMIT settings_changed(); }
		void set_show_topological_sections(bool b) { d_show_topological_sections = b; Q_EMIT settings_changed(); }
		void set_show_topological_lines(bool b) { d_show_topological_lines = b; Q_EMIT settings_changed(); }
		void set_show_topological_polygons(bool b) { d_show_topological_polygons = b; Q_EMIT settings_changed(); }
		void set_show_topological_networks(bool b) { d_show_topological_networks = b; Q_EMIT settings_changed(); }
		void set_show_velocity_arrows(bool b) { d_show_velocity_arrows = b; Q_EMIT settings_changed(); }
		void set_show_rasters(bool b) { d_show_rasters = b; Q_EMIT settings_changed(); }
		void set_show_3d_scalar_fields(bool b) { d_show_3d_scalar_fields = b; Q_EMIT settings_changed(); }
		void set_show_scalar_coverages(bool b) { d_show_scalar_coverages = b; Q_EMIT settings_changed(); }
		void set_show_strings(bool b) { d_show_strings = b; Q_EMIT settings_changed(); }

		void
		set_show_all(
				bool b)
		{
			d_show_static_points = b;
			d_show_static_multipoints = b;
			d_show_static_lines = b;
			d_show_static_polygons = b;
			d_show_topological_sections = b;
			d_show_topological_lines = b;
			d_show_topological_polygons = b;
			d_show_topological_networks = b;
			d_show_velocity_arrows = b;
			d_show_rasters = b;
			d_show_3d_scalar_fields = b;
			d_show_scalar_coverages = b;
			d_show_strings = b;

			Q_EMIT settings_changed();
		}
	
	Q_SIGNALS:

		void
		settings_changed();

	private:

		bool d_show_static_points;
		bool d_show_static_multipoints;
		bool d_show_static_lines;
		bool d_show_static_polygons;
		bool d_show_topological_sections;
		bool d_show_topological_lines;
		bool d_show_topological_polygons;
		bool d_show_topological_networks;
		bool d_show_velocity_arrows;
		bool d_show_rasters;
		bool d_show_3d_scalar_fields;
		bool d_show_scalar_coverages;
		bool d_show_strings;

	};

}

#endif  // GPLATES_GUI_RENDERSETTINGS_H
