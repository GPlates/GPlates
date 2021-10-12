/* $Id$ */

/**
 * \file 
 * Enables/disables canvas tools.
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

#ifndef GPLATES_GUI_ENABLECANVASTOOL_H
#define GPLATES_GUI_ENABLECANVASTOOL_H

#include <boost/tuple/tuple.hpp>

#include "canvas-tools/CanvasToolType.h"
#include "gui/FeatureFocus.h"
#include "view-operations/GeometryType.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class GeometryOperationTarget;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;

	/**
	 * Used for enabling/disabling a canvas tool.
	 */
	class EnableCanvasTool :
		public QObject
	{
		Q_OBJECT

	public:
		EnableCanvasTool(
			GPlatesQtWidgets::ViewportWindow &viewport_window,
			const GPlatesGui::FeatureFocus &feature_focus,
			const GPlatesViewOperations::GeometryOperationTarget &geom_operation_target,
			const GPlatesGui::ChooseCanvasTool &choose_canvas_tool);

		/**
		 * Call when the @a ViewportWindow passed into constructor is fully constructed.
		 * The way @a ViewportWindow is currently setup it creates us before it's
		 * fully constructed itself.
		 */
		void
		initialise();

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Changed which reconstruction geometry is currently focused.
		 */
		void
		feature_focus_changed(
				GPlatesGui::FeatureFocus &feature_focus);

		/**
		 * Geometry builder modifications have stopped.
		 */
		void
		geometry_builder_stopped_updating_geometry_excluding_intermediate_moves();

		/**
		 * The @a GeometryOperationTarget switched @a GeometryBuilder.
		 */
		void
		switched_geometry_builder(
				GPlatesViewOperations::GeometryOperationTarget &,
				GPlatesViewOperations::GeometryBuilder *);

		/**
		 * The @a ChooseCanvasTool chose/switched to a canvas tool.
		 */
		void
		chose_canvas_tool(
				GPlatesGui::ChooseCanvasTool &,
				GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type);
	private:
		/**
		 * Used to do the actual enabling/disabling of canvas tool.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window;

		/**
		 * Used to set the initial canvas tools enabled state.
		 */
		const GPlatesGui::FeatureFocus *d_feature_focus;

		/**
		 * Is true if a feature is currently in focus.
		 */
		bool d_feature_geom_is_in_focus;

		/**
		 * The currently active canvas tool.
		 */
		GPlatesCanvasTools::CanvasToolType::Value d_current_canvas_tool_type;

		/**
		 * The geometry operation target knows which @a GeometryBuilder will be
		 * targeted by a geometry operation.
		 */
		const GPlatesViewOperations::GeometryOperationTarget *d_geom_operation_target;

		void
		connect_to_feature_focus(
				const GPlatesGui::FeatureFocus &);

		void
		connect_to_geometry_builder(
				const GPlatesViewOperations::GeometryBuilder &);

		void
		connect_to_geometry_operation_target(
				const GPlatesViewOperations::GeometryOperationTarget &);

		void
		connect_to_choose_canvas_tool(
				const GPlatesGui::ChooseCanvasTool &);

		/**
		 * We've received new information so update our enabling/disabling of canvas tools.
		 */
		void
		update();

		void
		update_move_geometry_tool();

		void
		update_move_vertex_tool();

		void
		update_insert_vertex_tool();

		void
		update_delete_vertex_tool();

		void
		update_manipulate_pole_tool();

		void
		update_build_topology_tool();

		void
		update_edit_topology_tool();

		/**
		 * Gets the number of vertices and geometry type of geometry that will
		 * be targeted if @a next_canvas_tool is chosen as the next canvas tool.
		 * Returns zero vertices and 'NONE' geometry type if @a next_canvas_tool
		 * cannot be selected next or if targeted geometry doesn't exist.
		 */
		boost::tuple<unsigned int, GPlatesViewOperations::GeometryType::Value>
		get_target_geometry_parameters_if_tool_chosen_next(
				GPlatesCanvasTools::CanvasToolType::Value next_canvas_tool) const;
	};
}

#endif // GPLATES_GUI_ENABLECANVASTOOL_H
