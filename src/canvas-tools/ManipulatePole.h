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

#ifndef GPLATES_CANVASTOOLS_MANIPULATEPOLE_H
#define GPLATES_CANVASTOOLS_MANIPULATEPOLE_H

#include "gui/CanvasTool.h"
#include "qt-widgets/ReconstructionPoleWidget.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesGui
{
	class RenderedGeometryLayers;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to interactively manipulate absolute rotations.
	 */
	class ManipulatePole:
			public GPlatesGui::CanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ManipulatePole,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ManipulatePole,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~ManipulatePole()
		{  }

		/**
		 * Create a ManipulatePole instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				GPlatesGui::RenderedGeometryLayers &layers,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesQtWidgets::ReconstructionPoleWidget &pole_widget_)
		{
			ManipulatePole::non_null_ptr_type ptr(
					new ManipulatePole(globe_, globe_canvas_, layers, view_state_,
							pole_widget_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}
		
		
		virtual
		void
		handle_activation();


		virtual
		void
		handle_deactivation();


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

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		ManipulatePole(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				GPlatesGui::RenderedGeometryLayers &layers,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesQtWidgets::ReconstructionPoleWidget &pole_widget_);


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
		GPlatesGui::RenderedGeometryLayers *d_layers_ptr;

		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * This is the Reconstruction Pole widget in the Task Panel.
		 * It accumulates the rotation adjustment for us, as well as other book-keeping.
		 */
		GPlatesQtWidgets::ReconstructionPoleWidget *d_pole_widget_ptr;

		/**
		 * Whether or not this pole-manipulation tool is currently in the midst of a
		 * pole-manipulating drag.
		 */
		bool d_is_in_drag;
			
		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		ManipulatePole(
				const ManipulatePole &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		ManipulatePole &
		operator=(
				const ManipulatePole &);
		
	};
}

#endif  // GPLATES_CANVASTOOLS_MANIPULATEPOLE_H
