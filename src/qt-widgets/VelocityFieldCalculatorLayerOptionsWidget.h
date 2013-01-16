/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_VelocityFieldCalculatorLayerOptionsWidget_H
#define GPLATES_QTWIDGETS_VelocityFieldCalculatorLayerOptionsWidget_H

#include "VelocityFieldCalculatorLayerOptionsWidgetUi.h"

#include "InformationDialog.h"
#include "LayerOptionsWidget.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declaration.
	class ViewportWindow;

	/**
	 * VelocityFieldCalculatorLayerOptionsWidget is used to show additional options for
	 * topology network layers in the visual layers widget.
	 */
	class VelocityFieldCalculatorLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_VelocityFieldCalculatorLayerOptionsWidget
	{
		Q_OBJECT

	public:

		static
		LayerOptionsWidget *
		create(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		virtual
		void
		set_data(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer);

		virtual
		const QString &
		get_title();

	private slots:

		void
		handle_constrained_clicked();

		void
		handle_triangulation_clicked();

		void
		handle_solve_velocity_method_combobox_activated(
				int index);

		void
		handle_arrow_spacing_value_changed(
				double arrow_spacing);

		void
		handle_unlimited_arrow_spacing_clicked();

	private:

		VelocityFieldCalculatorLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;

		GPlatesQtWidgets::InformationDialog *d_help_solve_velocities_method_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_arrow_spacing_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_VelocityFieldCalculatorLayerOptionsWidget_H
