/**
 * \file 
 * $Revision: 14446 $
 * $Date: 2013-08-13 14:37:12 +0200 (Tue, 13 Aug 2013) $
 * 
 * Copyright (C) 2014 Geological Survey of Norway
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

#ifndef GPLATES_GUI_HELLINGERCANVASTOOLWORKFLOW_H
#define GPLATES_GUI_HELLINGERCANVASTOOLWORKFLOW_H

#include <boost/scoped_ptr.hpp>
#include <utility>

#include "CanvasToolWorkflow.h"
#include "GlobeCanvasTool.h"
#include "MapCanvasTool.h"

#include "GlobeCanvasToolAdapter.h"

#include "canvas-tools/CanvasTool.h"

#include "maths/GeometryType.h"

#include "qt-widgets/HellingerDialog.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	/**
	 * The canvas tool workflow for performing pole fits by the method of Hellinger
	 */
	class HellingerCanvasToolWorkflow :
			public CanvasToolWorkflow
	{
		Q_OBJECT

	public:

		HellingerCanvasToolWorkflow(
				CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window);

		virtual
		void
		initialise();

	protected:

		virtual
		void
		activate_workflow();

		virtual
		void
		deactivate_workflow();

		virtual
		boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
		get_selected_globe_and_map_canvas_tools(
					CanvasToolWorkflows::ToolType selected_tool) const;

	private:

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;

		//! For manipulating hellinger geometries in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_select_hellinger_geometries_tool;
		//! For manipulating hellinger geometries in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_select_hellinger_geometries_tool;

		//! For adjusting the pole estimate in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_adjust_pole_estimate_tool;
		//! For adjusting the pole estimate in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_adjust_pole_estimate_tool;

		GPlatesQtWidgets::HellingerDialog *d_hellinger_dialog_ptr;

		void
		create_canvas_tools(
				CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window);
	};
}

#endif // GPLATES_GUI_HELLINGERCANVASTOOLWORKFLOW_H
