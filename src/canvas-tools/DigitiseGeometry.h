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

#ifndef GPLATES_CANVASTOOLS_DIGITISEGEOMETRY_H
#define GPLATES_CANVASTOOLS_DIGITISEGEOMETRY_H

#include "gui/CanvasTool.h"
#include "qt-widgets/DigitisationWidget.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
}


namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to define new geometry.
	 */
	class DigitiseGeometry:
			public GPlatesGui::CanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<DigitiseGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<DigitiseGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~DigitiseGeometry()
		{  }

		/**
		 * Create a DigitiseGeometry instance.
		 *
		 * FIXME: Clean up unused parameters.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesQtWidgets::DigitisationWidget &digitisation_widget_,
				GPlatesQtWidgets::DigitisationWidget::GeometryType geom_type_)
		{
			DigitiseGeometry::non_null_ptr_type ptr(
					new DigitiseGeometry(globe_, globe_canvas_, view_state_, digitisation_widget_,
							geom_type_),
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
		handle_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		DigitiseGeometry(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesQtWidgets::DigitisationWidget &digitisation_widget_,
				GPlatesQtWidgets::DigitisationWidget::GeometryType geom_type_);
		
		
		const GPlatesQtWidgets::ViewportWindow &
		view_state() const
		{
			return *d_view_state_ptr;
		}
		

	private:
		
		/**
		 * This is the view state used to obtain current reconstruction time.
		 */
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;
		
		/**
		 * This is the Digitisation Widget in the Task Panel.
		 * It accumulates points for us and handles the actual feature creation step.
		 */
		GPlatesQtWidgets::DigitisationWidget *d_digitisation_widget_ptr;
		
		/**
		 * This is the type of geometry this particular DigitiseGeometry tool
		 * should default to.
		 */
		GPlatesQtWidgets::DigitisationWidget::GeometryType d_default_geom_type;
	
		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		DigitiseGeometry(
				const DigitiseGeometry &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		DigitiseGeometry &
		operator=(
				const DigitiseGeometry &);
		
	};
}

#endif  // GPLATES_CANVASTOOLS_DIGITISEGEOMETRY_H
