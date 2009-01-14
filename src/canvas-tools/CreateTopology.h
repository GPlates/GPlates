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

#ifndef GPLATES_CANVASTOOLS_CREATETOPOLOGY_H
#define GPLATES_CANVASTOOLS_CREATETOPOLOGY_H

#include "gui/CanvasTool.h"
#include "qt-widgets/CreateTopologyWidget.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to interactively manipulate absolute rotations.
	 */
	class CreateTopology:
			public GPlatesGui::CanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<CreateTopology,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<CreateTopology,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~CreateTopology()
		{  }

		/**
		 * Create a CreateTopology instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				const GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesQtWidgets::CreateTopologyWidget &create_topology_widget)
		{
			return CreateTopology::non_null_ptr_type(
					new CreateTopology(
							rendered_geom_collection,
							globe,
							globe_canvas,
							view_state,
							create_topology_widget),
					GPlatesUtils::NullIntrusivePointerHandler());
		}
		
		
		virtual
		void
		handle_activation();


		virtual
		void
		handle_deactivation();


#if 0
		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);


		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);


		virtual
		void
		handle_shift_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);


		virtual
		void
		handle_shift_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);
#endif

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		CreateTopology(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				const GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesQtWidgets::CreateTopologyWidget &create_topology_widget);


		const GPlatesQtWidgets::ViewportWindow &
		view_state() const
		{
			return *d_view_state_ptr;
		}


	private:

		/**
		 * We need to change which canvas-tool layer is shown when this canvas-tool is
		 * activated.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * This is the Reconstruction Pole widget in the Task Panel.
		 * It accumulates the rotation adjustment for us, as well as other book-keeping.
		 */
		GPlatesQtWidgets::CreateTopologyWidget *d_create_topology_widget_ptr;

#if 0
		/**
		 * Whether or not this pole-manipulation tool is currently in the midst of a
		 * pole-manipulating drag.
		 */
		bool d_is_in_drag;
#endif
		// FIXME: d_is_in_new ???
	};
}

#endif  // GPLATES_CANVASTOOLS_CREATETOPOLOGY_H
