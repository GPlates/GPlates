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

namespace GPlatesGui
{
	struct RenderSettings
	{
		bool show_points;
		bool show_multipoints;
		bool show_lines;
		bool show_polygons;
		bool show_arrows;
		bool show_topology;
		bool show_strings;

		RenderSettings() :
			show_points(true),
			show_multipoints(true),
			show_lines(true),
			show_polygons(true),
			show_arrows(true),
			show_topology(true),
			show_strings(true)
		{  }

		RenderSettings(
				bool show_points_,
				bool show_multipoints_,
				bool show_lines_,
				bool show_polygons_,
				bool show_arrows_,
				bool show_topology_,
				bool show_strings_) :
			show_points(show_points_),
			show_multipoints(show_multipoints_),
			show_lines(show_lines_),
			show_polygons(show_polygons_),
			show_arrows(show_arrows_),
			show_topology(show_topology_),
			show_strings(show_strings_)
		{  }

	};

}

#endif  // GPLATES_GUI_RENDERSETTINGS_H
