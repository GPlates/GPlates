/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_GEOMETRYFOCUSHIGHLIGHT_H
#define GPLATES_GUI_GEOMETRYFOCUSHIGHLIGHT_H

#include <set>
#include <vector>
#include <boost/optional.hpp>
#include <QObject>

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/TopologyUtils.h"

#include "gui/Symbol.h"

#include "model/FeatureId.h"


namespace GPlatesGui
{
	class RenderSettings;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
	class RenderedGeometryCollection;
	class RenderedGeometryParameters;
}

namespace GPlatesGui
{
	class FeatureFocus;

	namespace GeometryFocusHighlight
	{
		/**
		 * Draw the focused geometry (if there is one) into the specified rendered geometry layer.
		 *
		 * If no geometry is currently in focus then the rendered geometry layer will be cleared.
		 *
		 * NOTE: The caller is responsible for activating/deactivating the specified rendered geometry layer.
		 */
		void
		draw_focused_geometry(
				FeatureFocus &feature_focus,
				GPlatesViewOperations::RenderedGeometryLayer &render_geom_layer,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters,
				const GPlatesGui::RenderSettings &render_settings,
				const std::set<GPlatesModel::FeatureId> &topological_sections,
				const GPlatesAppLogic::TopologyUtils::resolved_topological_boundaries_networks_to_shared_sub_segments_map_type &resolved_topological_shared_sub_segments_map,
				const symbol_map_type &symbol_map);
	}
}

#endif // GPLATES_GUI_GEOMETRYFOCUSHIGHLIGHT_H
