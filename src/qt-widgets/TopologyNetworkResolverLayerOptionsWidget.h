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
 
#ifndef GPLATES_QTWIDGETS_TOPOLOGYNETWORKRESOLVERLAYEROPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_TOPOLOGYNETWORKRESOLVERLAYEROPTIONSWIDGET_H

#include <QDoubleValidator>

#include "ui_TopologyNetworkResolverLayerOptionsWidgetUi.h"

#include "DrawStyleDialog.h"
#include "InformationDialog.h"
#include "LayerOptionsWidget.h"
#include "OpenFileDialog.h"


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
	class ColourScaleWidget;
	class FriendlyLineEdit;
	class ReadErrorAccumulationDialog;

	/**
	 * TopologyNetworkResolverLayerOptionsWidget is used to show additional options for
	 * topology network layers in the visual layers widget.
	 */
	class TopologyNetworkResolverLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_TopologyNetworkResolverLayerOptionsWidget
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
		handle_strain_rate_smoothing_button(
				bool checked);

		void
		handle_strain_rate_clamping_clicked();

		void
		handle_strain_rate_clamping_line_editing_finished();

		void
		handle_rift_exponential_stretching_constant_line_editing_finished();

		void
		handle_rift_strain_rate_resolution_line_editing_finished();

		void
		handle_rift_edge_length_threshold_line_editing_finished();

		void
		handle_fill_rigid_blocks_clicked();

		void
		handle_segment_velocity_clicked();

		void
		handle_colour_mode_button(
				bool checked);

		void
		handle_draw_mode_button(
				bool checked);

		void
		handle_min_abs_dilatation_spinbox_changed(
				double min_abs_dilatation);

		void
		handle_max_abs_dilatation_spinbox_changed(
				double max_abs_dilatation);

		void
		handle_select_dilatation_palette_filename_button_clicked();

		void
		handle_use_default_dilatation_palette_button_clicked();

		void
		handle_min_abs_second_invariant_spinbox_changed(
				double min_abs_second_invariant);

		void
		handle_max_abs_second_invariant_spinbox_changed(
				double max_abs_second_invariant);

		void
		handle_select_second_invariant_palette_filename_button_clicked();

		void
		handle_use_default_second_invariant_palette_button_clicked();

		void
		handle_min_strain_rate_style_spinbox_changed(
				double min_strain_rate_style);

		void
		handle_max_strain_rate_style_spinbox_changed(
				double max_strain_rate_style);

		void
		handle_select_strain_rate_style_palette_filename_button_clicked();

		void
		handle_use_default_strain_rate_style_palette_button_clicked();

		void
		handle_fill_opacity_spinbox_changed(
				double value);

		void
		handle_fill_intensity_spinbox_changed(
				double value);

		void
		open_draw_style_setting_dlg();

	private:

		/**
		 * Fixes up any QValidator::Intermediate input (only when user editing has finished) so that we
		 * always get a valid result (when user editing has finished) and hence always get an
		 * 'editingFinished()' signal to process/finalise the user input thus avoiding confusing the user.
		 */
		class DoubleValidator :
				public QDoubleValidator
		{
		public:
			DoubleValidator(
					const double &minimum,
					const double &maximum,
					const int decimal_places,
					QObject *parent_) :
				QDoubleValidator(minimum, maximum, decimal_places, parent_)
			{  }

			virtual
			void
			fixup(
					QString &input) const
			{
				// Conversion to double assuming the system locale, falling back to C locale.
				bool ok;
				double value = locale().toDouble(input, &ok);
				if (!ok)
				{
					// QString::toDouble() only uses C locale.
					value = input.toDouble(&ok);
				}

				// Clamp the value to the allowed range.
				if (value < bottom())
				{
					value = bottom();
				}
				else if (value > top())
				{
					value = top();
				}

				input = locale().toString(value, 'f', decimals());
			}
		};

		TopologyNetworkResolverLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;
		DrawStyleDialog *d_draw_style_dialog_ptr;

		OpenFileDialog d_open_file_dialog;

		QDoubleValidator *d_clamp_strain_rate_line_edit_double_validator;
		QDoubleValidator *d_rift_strain_rate_resolution_line_edit_double_validator;
		QDoubleValidator *d_rift_exponential_stretching_constant_line_edit_double_validator;
		QDoubleValidator *d_rift_edge_length_threshold_line_edit_double_validator;

		FriendlyLineEdit *d_dilatation_palette_filename_lineedit;
		ColourScaleWidget *d_dilatation_colour_scale_widget;

		FriendlyLineEdit *d_second_invariant_palette_filename_lineedit;
		ColourScaleWidget *d_second_invariant_colour_scale_widget;

		FriendlyLineEdit *d_strain_rate_style_palette_filename_lineedit;
		ColourScaleWidget *d_strain_rate_style_colour_scale_widget;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;

		GPlatesQtWidgets::InformationDialog *d_help_strain_rate_smoothing_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_strain_rate_clamping_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_rift_exponential_stretching_constant_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_rift_strain_rate_resolution_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_rift_edge_length_threshold_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_triangulation_colour_mode_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_triangulation_draw_mode_dialog;


		//! Used to scale min/max strain rate values into their spinboxes.
		static const double STRAIN_RATE_SCALE;
		// Min/max range for scaled strain rate values.
		static const double SCALED_STRAIN_RATE_MIN;
		static const double SCALED_STRAIN_RATE_MAX;
		//! Number of decimal places for scaled strain rate values.
		static const int SCALED_STRAIN_RATE_DECIMAL_PLACES;
		//! Number of decimal places for rift exponential stretching constant values.
		static const int RIFT_EXPONENTIAL_STRETCHING_CONSTANT_DECIMAL_PLACES;
		//! Number of decimal places for rift edge length threshold values.
		static const int RIFT_EDGE_LENGTH_THRESHOLD_DECIMAL_PLACES;
	};
}

#endif  // GPLATES_QTWIDGETS_TOPOLOGYNETWORKRESOLVERLAYEROPTIONSWIDGET_H
