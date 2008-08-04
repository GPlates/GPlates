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


namespace GPlatesQtWidgets
{
	class ViewportWindow;
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
				const GPlatesQtWidgets::ViewportWindow &view_state_)
		{
			ManipulatePole::non_null_ptr_type ptr(
					new ManipulatePole(globe_, globe_canvas_, view_state_),
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
				bool is_on_globe);

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe);

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		ManipulatePole(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_);
		
		
		const GPlatesQtWidgets::ViewportWindow &
		view_state() const
		{
			return *d_view_state_ptr;
		}
		

	private:
		
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;
			
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
