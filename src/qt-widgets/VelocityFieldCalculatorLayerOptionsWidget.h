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

#include "ui_VelocityFieldCalculatorLayerOptionsWidgetUi.h"

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

	private Q_SLOTS:

		void
		handle_solve_velocity_method_combobox_activated(
				int index);

		void
		handle_arrow_spacing_value_changed(
				double arrow_spacing);

		void
		handle_unlimited_arrow_spacing_clicked();

		void
		handle_arrow_body_scale_value_changed(
				double arrow_body_scale_log10);

		void
		handle_arrowhead_scale_value_changed(
				double arrowhead_scale_log10);

		void
		handle_velocity_delta_time_type_button(
				bool checked);

		void
		handle_velocity_delta_time_value_changed(
				double value);

		void
		handle_velocity_smoothing_check_box_changed();

		void
		handle_velocity_smoothing_distance_spinbox_changed(
				double value);

		void
		handle_exclude_smoothing_in_deforming_regions_check_box_changed();

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
		GPlatesQtWidgets::InformationDialog *d_help_arrow_scale_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_velocity_smoothing_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_velocity_time_delta_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_VelocityFieldCalculatorLayerOptionsWidget_H
