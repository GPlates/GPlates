/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_HELLINGERFITWIDGET_H
#define GPLATES_QTWIDGETS_HELLINGERFITWIDGET_H

#include "HellingerModel.h"
#include "ui_HellingerFitWidgetUi.h"


namespace GPlatesQtWidgets
{

	class HellingerDialog;
	class HellingerModel;

	class HellingerFitWidget:
			public QWidget,
			protected Ui_HellingerFitWidget
	{
		Q_OBJECT

	public:
		HellingerFitWidget(
				HellingerDialog *hellinger_dialog,
				HellingerModel *hellinger_model);


		virtual
		~HellingerFitWidget()
		{  }

		/**
		 * @brief update_from_model - update the initial guess spinboxes and fit-related info
		 * from the Hellinger model.
		 */
		void
		update_fit_widgets_from_model();

		/**
		 * @brief update_model_from_fit_related_data - update the HellingerModel's fit-related input data
		 * from the widget.
		 */
		void
		update_model_from_fit_widgets();

		void
		update_after_switching_tabs();

		void
		update_enabled_state_of_estimate_widgets(
				bool adjust_pole_tool_is_active = false);

		void
		update_after_pole_result();

		void
		start_progress_bar();

		void
		stop_progress_bar();

		bool
		show_result_12_checked() const
		{
			return checkbox_show_result_12->isChecked();
		}

		bool
		show_result_13_checked() const
		{
			return checkbox_show_result_13->isChecked();
		}

		bool
		show_result_23_checked() const
		{
			return checkbox_show_result_23->isChecked();
		}

		bool
		show_estimate_12_checked() const
		{
			return checkbox_show_estimate_12->isChecked();
		}

		bool
		show_estimate_13_checked() const
		{
			return checkbox_show_estimate_13->isChecked();
		}

		HellingerPoleEstimate
		estimate_12() const;

		void
		set_estimate_12(
				const HellingerPoleEstimate &estimate);

		void
		set_estimate_13(
				const HellingerPoleEstimate &estimate);

		HellingerPoleEstimate
		estimate_13() const;

		void
		enable_pole_estimate_widgets(
				bool enable);

		void
		update_buttons();


	Q_SIGNALS:

		void
		pole_estimate_12_changed(double,double);

		void
		pole_estimate_13_changed(double, double);

		void
		pole_estimate_12_angle_changed(double);

		void
		pole_estimate_13_angle_changed(double);

		void
		show_result_checkboxes_clicked();

		void
		show_estimate_checkboxes_clicked();


	private Q_SLOTS:

		void
		handle_checkbox_grid_search_changed();

		void
		handle_spinbox_radius_changed();

		void
		handle_spinbox_confidence_changed();

		void
		handle_pole_estimate_12_angle_changed();

		void
		handle_pole_estimate_13_angle_changed();

		void
		handle_pole_estimate_12_lat_lon_changed();

		void
		handle_pole_estimate_13_lat_lon_changed();

		void
		handle_show_result_checkboxes_clicked();

		void
		handle_show_estimate_checkboxes_clicked();

		void
		handle_clipboard_12_clicked();

		void
		handle_clipboard_13_clicked();

		void
		handle_clipboard_23_clicked();

		void
		handle_amoeba_iterations_checked();

		void
		handle_amoeba_residual_checked();

		void
		handle_amoeba_iterations_changed();

		void
		handle_amoeba_residual_changed();

	private:

		void
		initialise_widgets();

		void
		set_up_connections();

		void
		enable_three_way_widgets(
				bool enable);

		void
		show_three_way_widgets(
				bool show);

		HellingerDialog *d_hellinger_dialog_ptr;
		HellingerModel *d_hellinger_model_ptr;

		bool d_pole_has_been_calculated;

		bool d_amoeba_residual_ok;

		/**
		 * @brief d_default_palette - a default palette used for resetting
		 * widget backgrounds.
		 */
		QPalette d_default_palette;

		/**
		 * @brief d_red_palette - a red-background palette used to warn of
		 * invalid widget data.
		 */
		QPalette d_red_palette;

		double d_last_used_two_way_tolerance;

		double d_last_used_three_way_tolerance;

		bool d_three_way_fitting_is_enabled;
	};

}


#endif // GPLATES_QTWIDGETS_HELLINGERFITWIDGET_H
