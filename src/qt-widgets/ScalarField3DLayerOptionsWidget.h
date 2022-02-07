/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_SCALARFIELD3DLAYEROPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_SCALARFIELD3DLAYEROPTIONSWIDGET_H

#include <utility>
#include <vector>
#include <QSlider>

#include "ScalarField3DLayerOptionsWidgetUi.h"

#include "LayerOptionsWidget.h"
#include "OpenFileDialog.h"

#include "app-logic/Layer.h"

#include "gui/BuiltinColourPaletteType.h"
#include "gui/RasterColourPalette.h"

#include "view-operations/ScalarField3DRenderParameters.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class Layer;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declaration.
	class ReadErrorAccumulationDialog;
	class RemappedColourPaletteWidget;
	class ViewportWindow;

	/**
	 * ScalarField3DLayerOptionsWidget is used to show additional options for
	 * 3D scalar field layers in the visual layers widget.
	 */
	class ScalarField3DLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_ScalarField3DLayerOptionsWidget
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

		~ScalarField3DLayerOptionsWidget();

	private Q_SLOTS:

		void
		handle_render_mode_button(
				bool checked);

		void
		handle_isosurface_deviation_window_mode_button(
				bool checked);

		void
		handle_isosurface_colour_mode_button(
				bool checked);

		void
		handle_cross_sections_colour_mode_button(
				bool checked);

		void
		handle_select_scalar_palette_filename_button_clicked();

		void
		handle_use_default_scalar_palette_button_clicked();

		void
		handle_builtin_scalar_colour_palette_selected(
				const GPlatesGui::BuiltinColourPaletteType &builtin_scalar_colour_palette_type);

		void
		handle_builtin_scalar_parameters_changed(
				const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_scalar_parameters);

		void
		handle_scalar_palette_range_check_box_changed(
				int state);

		void
		handle_scalar_palette_min_line_editing_finished(
				double value);

		void
		handle_scalar_palette_max_line_editing_finished(
				double value);

		void
		handle_scalar_palette_range_restore_min_max_button_clicked();

		void
		handle_scalar_palette_range_restore_mean_deviation_button_clicked();

		void
		handle_scalar_palette_range_restore_mean_deviation_spinbox_changed(
				double value);

		void
		handle_select_gradient_palette_filename_button_clicked();

		void
		handle_use_default_gradient_palette_button_clicked();

		void
		handle_builtin_gradient_colour_palette_selected(
				const GPlatesGui::BuiltinColourPaletteType &builtin_gradient_colour_palette_type);

		void
		handle_builtin_gradient_parameters_changed(
				const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_gradient_parameters);

		void
		handle_gradient_palette_range_check_box_changed(
				int state);

		void
		handle_gradient_palette_min_line_editing_finished(
				double value);

		void
		handle_gradient_palette_max_line_editing_finished(
				double value);

		void
		handle_gradient_palette_range_restore_min_max_button_clicked();

		void
		handle_gradient_palette_range_restore_mean_deviation_button_clicked();

		void
		handle_gradient_palette_range_restore_mean_deviation_spinbox_changed(
				double value);

		void
		handle_isovalue_spinbox_changed(
				double isovalue);

		void
		handle_isovalue_slider_changed(
				int value);

		void
		handle_deviation_spinbox_changed(
				double deviation);

		void
		handle_symmetric_deviation_spinbox_changed(
				double symmetric_deviation);

		void
		handle_symmetric_deviation_check_box_changed();

		void
		handle_opacity_deviation_surfaces_spinbox_changed(
				double opacity);

		void
		handle_volume_render_deviation_window_check_box_changed();

		void
		handle_opacity_deviation_volume_rendering_spinbox_changed(
				double opacity);

		void
		handle_surface_deviation_window_check_box_changed();

		void
		handle_isoline_frequency_spinbox_changed(
				int frequency);

		void
		handle_surface_polygons_mask_check_box_changed();

		void
		handle_depth_restriction_spinbox_changed(
				double value);

		void
		handle_restore_actual_depth_range_button_clicked();

		void
		handle_quality_performance_spinbox_changed(
				int value);

		void
		handle_improve_performance_during_globe_rotation_check_box_changed();

		void
		handle_test_variable_spinbox_changed(
				double value);

	private:

		ScalarField3DLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		void
		enable_disable_options_for_visual_layer_params(
				const GPlatesViewOperations::ScalarField3DRenderParameters &scalar_field_render_parameters);

		std::pair<double, double>
		get_scalar_value_min_max(
				GPlatesAppLogic::Layer &layer) const;

		std::pair<double, double>
		get_scalar_value_mean_std_dev(
				GPlatesAppLogic::Layer &layer) const;

		std::pair<double, double>
		get_gradient_magnitude_min_max(
				GPlatesAppLogic::Layer &layer) const;

		std::pair<double, double>
		get_gradient_magnitude_mean_std_dev(
				GPlatesAppLogic::Layer &layer) const;

		int
		get_slider_isovalue(
				const double &iso_value,
				GPlatesAppLogic::Layer &layer,
				QSlider *isovalue_slider) const;

		std::pair<double, double>
		get_depth_min_max(
				GPlatesAppLogic::Layer &layer) const;


		/**
		 * The number of QDoubleSpinBox's used for shader test variables.
		 */
		static const unsigned int NUM_SHADER_TEST_VARIABLES = 16;


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;

		OpenFileDialog d_open_file_dialog;
		RemappedColourPaletteWidget *d_scalar_colour_palette_widget;
		RemappedColourPaletteWidget *d_gradient_colour_palette_widget;

		std::vector<float> d_shader_test_variables;


		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
	};
}

#endif  // GPLATES_QTWIDGETS_SCALARFIELD3DLAYEROPTIONSWIDGET_H
