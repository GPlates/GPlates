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

#include <algorithm>
#include <QtGlobal>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

#include "ScalarField3DLayerOptionsWidget.h"

#include "QtWidgetUtils.h"
#include "RemappedColourPaletteWidget.h"
#include "ViewportWindow.h"
#include "VisualLayersDialog.h"

#include "app-logic/ScalarField3DLayerParams.h"

#include "file-io/ReadErrorAccumulation.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/BuiltinColourPaletteType.h"
#include "gui/RasterColourPalette.h"

#include "maths/MathsUtils.h"

#include "presentation/ScalarField3DVisualLayerParams.h"
#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"


// Define this to hide the GUI controls that change the shader test variables.
#define HIDE_SHADER_TEST_VARIABLE_CONTROLS


GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::ScalarField3DLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_open_file_dialog(
			this,
			tr("Open CPT File"),
			tr("Regular CPT file (*.cpt);;All files (*)"),
			view_state),
	d_scalar_colour_palette_widget(new RemappedColourPaletteWidget(view_state, viewport_window, this)),
	d_gradient_colour_palette_widget(new RemappedColourPaletteWidget(view_state, viewport_window, this))
{
	setupUi(this);

	//
	// Render mode.
	//

	isosurface_render_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isosurface_render_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_render_mode_button(bool)));
	cross_sections_render_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			cross_sections_render_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_render_mode_button(bool)));

	//
	// Iso-surface deviation window mode.
	//

	no_deviation_window_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			no_deviation_window_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
	single_deviation_window_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			single_deviation_window_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
	double_deviation_window_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			double_deviation_window_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));

	//
	// Isosurface colour mode.
	//

	isosurface_depth_colour_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isosurface_depth_colour_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_isosurface_colour_mode_button(bool)));
	isosurface_scalar_colour_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isosurface_scalar_colour_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_isosurface_colour_mode_button(bool)));
	isosurface_gradient_colour_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isosurface_gradient_colour_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_isosurface_colour_mode_button(bool)));

	//
	// Cross-section colour mode.
	//

	cross_sections_scalar_colour_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			cross_sections_scalar_colour_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_cross_sections_colour_mode_button(bool)));
	cross_sections_gradient_colour_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			cross_sections_gradient_colour_mode_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_cross_sections_colour_mode_button(bool)));

	//
	// Scalar colour palette.
	//

	QtWidgetUtils::add_widget_to_placeholder(
			d_scalar_colour_palette_widget,
			scalar_palette_placeholder_widget);
	d_scalar_colour_palette_widget->setCursor(QCursor(Qt::ArrowCursor));

	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(select_palette_filename_button_clicked()),
			this, SLOT(handle_select_scalar_palette_filename_button_clicked()));
	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(use_default_palette_button_clicked()),
			this, SLOT(handle_use_default_scalar_palette_button_clicked()));
	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(builtin_colour_palette_selected(const GPlatesGui::BuiltinColourPaletteType &)),
			this, SLOT(handle_builtin_scalar_colour_palette_selected(const GPlatesGui::BuiltinColourPaletteType &)));
	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(builtin_parameters_changed(const GPlatesGui::BuiltinColourPaletteType::Parameters &)),
			this, SLOT(handle_builtin_scalar_parameters_changed(const GPlatesGui::BuiltinColourPaletteType::Parameters &)));

	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(range_check_box_changed(int)),
			this, SLOT(handle_scalar_palette_range_check_box_changed(int)));

	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(min_line_editing_finished(double)),
			this, SLOT(handle_scalar_palette_min_line_editing_finished(double)));
	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(max_line_editing_finished(double)),
			this, SLOT(handle_scalar_palette_max_line_editing_finished(double)));

	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(range_restore_min_max_button_clicked()),
			this, SLOT(handle_scalar_palette_range_restore_min_max_button_clicked()));
	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(range_restore_mean_deviation_button_clicked()),
			this, SLOT(handle_scalar_palette_range_restore_mean_deviation_button_clicked()));
	QObject::connect(
			d_scalar_colour_palette_widget, SIGNAL(range_restore_mean_deviation_spinbox_changed(double)),
			this, SLOT(handle_scalar_palette_range_restore_mean_deviation_spinbox_changed(double)));

	//
	// Gradient colour palette.
	//

	QtWidgetUtils::add_widget_to_placeholder(
			d_gradient_colour_palette_widget,
			gradient_palette_placeholder_widget);
	d_gradient_colour_palette_widget->setCursor(QCursor(Qt::ArrowCursor));

	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(select_palette_filename_button_clicked()),
			this, SLOT(handle_select_gradient_palette_filename_button_clicked()));
	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(use_default_palette_button_clicked()),
			this, SLOT(handle_use_default_gradient_palette_button_clicked()));
	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(builtin_colour_palette_selected(const GPlatesGui::BuiltinColourPaletteType &)),
			this, SLOT(handle_builtin_gradient_colour_palette_selected(const GPlatesGui::BuiltinColourPaletteType &)));
	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(builtin_parameters_changed(const GPlatesGui::BuiltinColourPaletteType::Parameters &)),
			this, SLOT(handle_builtin_gradient_parameters_changed(const GPlatesGui::BuiltinColourPaletteType::Parameters &)));

	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(range_check_box_changed(int)),
			this, SLOT(handle_gradient_palette_range_check_box_changed(int)));

	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(min_line_editing_finished(double)),
			this, SLOT(handle_gradient_palette_min_line_editing_finished(double)));
	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(max_line_editing_finished(double)),
			this, SLOT(handle_gradient_palette_max_line_editing_finished(double)));

	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(range_restore_min_max_button_clicked()),
			this, SLOT(handle_gradient_palette_range_restore_min_max_button_clicked()));
	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(range_restore_mean_deviation_button_clicked()),
			this, SLOT(handle_gradient_palette_range_restore_mean_deviation_button_clicked()));
	QObject::connect(
			d_gradient_colour_palette_widget, SIGNAL(range_restore_mean_deviation_spinbox_changed(double)),
			this, SLOT(handle_gradient_palette_range_restore_mean_deviation_spinbox_changed(double)));

	//
	// Isovalue spinbox/slider.
	//

	isovalue1_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue1_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_isovalue_spinbox_changed(double)));
	isovalue2_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue2_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_isovalue_spinbox_changed(double)));
	isovalue1_slider->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue1_slider, SIGNAL(valueChanged(int)),
			this, SLOT(handle_isovalue_slider_changed(int)));
	isovalue2_slider->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue2_slider, SIGNAL(valueChanged(int)),
			this, SLOT(handle_isovalue_slider_changed(int)));

	isovalue1_lower_deviation_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue1_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_deviation_spinbox_changed(double)));
	isovalue1_upper_deviation_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue1_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_deviation_spinbox_changed(double)));
	isovalue2_lower_deviation_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue2_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_deviation_spinbox_changed(double)));
	isovalue2_upper_deviation_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue2_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_deviation_spinbox_changed(double)));
	isovalue1_symmetric_deviation_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue1_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));
	isovalue2_symmetric_deviation_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue2_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));
	symmetric_deviation_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			symmetric_deviation_button, SIGNAL(stateChanged(int)),
			this, SLOT(handle_symmetric_deviation_check_box_changed()));

	//
	// Deviation window render options.
	//

	opacity_deviation_surfaces_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			opacity_deviation_surfaces_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_opacity_deviation_surfaces_spinbox_changed(double)));
	volume_render_deviation_window_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			volume_render_deviation_window_button, SIGNAL(stateChanged(int)),
			this, SLOT(handle_volume_render_deviation_window_check_box_changed()));
	opacity_deviation_volume_rendering_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			opacity_deviation_volume_rendering_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_opacity_deviation_volume_rendering_spinbox_changed(double)));
	surface_deviation_window_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			surface_deviation_window_button, SIGNAL(stateChanged(int)),
			this, SLOT(handle_surface_deviation_window_check_box_changed()));
	isoline_frequency_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isoline_frequency_spin_box, SIGNAL(valueChanged(int)),
			this, SLOT(handle_isoline_frequency_spinbox_changed(int)));

	//
	// Surface polygons mask.
	//

	enable_surface_polygons_mask_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			enable_surface_polygons_mask_button, SIGNAL(stateChanged(int)),
			this, SLOT(handle_surface_polygons_mask_check_box_changed()));
	show_polygon_walls_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			show_polygon_walls_button, SIGNAL(stateChanged(int)),
			this, SLOT(handle_surface_polygons_mask_check_box_changed()));
	only_show_boundary_walls_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			only_show_boundary_walls_button, SIGNAL(stateChanged(int)),
			this, SLOT(handle_surface_polygons_mask_check_box_changed()));
	treat_polylines_as_polygons_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			treat_polylines_as_polygons_button, SIGNAL(stateChanged(int)),
			this, SLOT(handle_surface_polygons_mask_check_box_changed()));

	//
	// Depth restriction.
	//

	min_depth_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			min_depth_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_depth_restriction_spinbox_changed(double)));
	max_depth_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			max_depth_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_depth_restriction_spinbox_changed(double)));
	restore_actual_depth_range_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			restore_actual_depth_range_button, SIGNAL(clicked()),
			this, SLOT(handle_restore_actual_depth_range_button_clicked()));

	//
	// Quality/performance.
	//

	sampling_rate_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			sampling_rate_spin_box, SIGNAL(valueChanged(int)),
			this, SLOT(handle_quality_performance_spinbox_changed(int)));
	bisection_iterations_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			bisection_iterations_spin_box, SIGNAL(valueChanged(int)),
			this, SLOT(handle_quality_performance_spinbox_changed(int)));
	improve_performance_during_drag_globe_check_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			improve_performance_during_drag_globe_check_box, SIGNAL(stateChanged(int)),
			this, SLOT(handle_improve_performance_during_globe_rotation_check_box_changed()));
	improve_performance_during_drag_globe_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			improve_performance_during_drag_globe_spin_box, SIGNAL(valueChanged(int)),
			this, SLOT(handle_quality_performance_spinbox_changed(int)));

	//
	// Scalar field shader program test variables.
	//

	d_shader_test_variables.resize(NUM_SHADER_TEST_VARIABLES, 0.0f);

	test_variable_0_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_0_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_1_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_1_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_2_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_2_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_3_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_3_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_4_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_4_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_5_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_5_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_6_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_6_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_7_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_7_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_8_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_8_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_9_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_9_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_10_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_10_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_11_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_11_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_12_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_12_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_13_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_13_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_14_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_14_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));
	test_variable_15_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			test_variable_15_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_test_variable_spinbox_changed(double)));

	// NOTE: If you add more test variable spinboxes then update 'NUM_SHADER_TEST_VARIABLES'.

#ifdef HIDE_SHADER_TEST_VARIABLE_CONTROLS
	shader_test_variables_group_box->hide();
#endif
}


GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::~ScalarField3DLayerOptionsWidget()
{ }


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new ScalarField3DLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());

		if (visual_layer_params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();

			const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);
			const double scalar_field_min = scalar_field_min_max.first;
			const double scalar_field_max = scalar_field_min_max.second;
			const double single_step_isovalue = (scalar_field_max - scalar_field_min) / 100.0;
			const double single_step_deviation = (scalar_field_max - scalar_field_min) / 400.0;

			// Setting the values in the spin boxes will emit signals if the value changes
			// which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.

			//
			// Set the render mode.
			//

			QObject::disconnect(
					isosurface_render_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_render_mode_button(bool)));
			QObject::disconnect(
					cross_sections_render_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_render_mode_button(bool)));

			switch (visual_layer_params->get_render_mode())
			{
			case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE:
				isosurface_render_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS:
				cross_sections_render_mode_button->setChecked(true);
				break;
			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
			QObject::connect(
					isosurface_render_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_render_mode_button(bool)));
			QObject::connect(
					cross_sections_render_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_render_mode_button(bool)));

			QObject::connect(
					isosurface_render_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_render_mode_button(bool)));
			QObject::connect(
					cross_sections_render_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_render_mode_button(bool)));

			//
			// Set the iso-surface colour mode.
			//

			QObject::disconnect(
					isosurface_depth_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_colour_mode_button(bool)));
			QObject::disconnect(
					isosurface_scalar_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_colour_mode_button(bool)));
			QObject::disconnect(
					isosurface_gradient_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_colour_mode_button(bool)));

			switch (visual_layer_params->get_isosurface_colour_mode())
			{
			case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_DEPTH:
				isosurface_depth_colour_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_SCALAR:
				isosurface_scalar_colour_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_GRADIENT:
				isosurface_gradient_colour_mode_button->setChecked(true);
				break;
			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
			QObject::connect(
					isosurface_depth_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_colour_mode_button(bool)));
			QObject::connect(
					isosurface_scalar_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_colour_mode_button(bool)),
					Qt::UniqueConnection);
			QObject::connect(
					isosurface_gradient_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_colour_mode_button(bool)));

			QObject::connect(
					isosurface_depth_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_colour_mode_button(bool)));
			QObject::connect(
					isosurface_scalar_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_colour_mode_button(bool)));
			QObject::connect(
					isosurface_gradient_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_colour_mode_button(bool)));

			//
			// Set the cross-section colour mode.
			//

			QObject::disconnect(
					cross_sections_scalar_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_cross_sections_colour_mode_button(bool)));
			QObject::disconnect(
					cross_sections_gradient_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_cross_sections_colour_mode_button(bool)));

			switch (visual_layer_params->get_cross_section_colour_mode())
			{
			case GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_SCALAR:
				cross_sections_scalar_colour_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_GRADIENT:
				cross_sections_gradient_colour_mode_button->setChecked(true);
				break;
			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
			QObject::connect(
					cross_sections_scalar_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_cross_sections_colour_mode_button(bool)));
			QObject::connect(
					cross_sections_gradient_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_cross_sections_colour_mode_button(bool)));

			QObject::connect(
					cross_sections_scalar_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_cross_sections_colour_mode_button(bool)));
			QObject::connect(
					cross_sections_gradient_colour_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_cross_sections_colour_mode_button(bool)));

			//
			// Set the scalar colour palette.
			//

			d_scalar_colour_palette_widget->set_parameters(
					visual_layer_params->get_scalar_colour_palette_parameters());

			//
			// Set the gradient colour palette.
			//

			d_gradient_colour_palette_widget->set_parameters(
					visual_layer_params->get_gradient_colour_palette_parameters());

			//
			// Set the isosurface deviation window mode.
			//

			QObject::disconnect(
					no_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
			QObject::disconnect(
					single_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
			QObject::disconnect(
					double_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));

			QObject::disconnect(
					no_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
			QObject::disconnect(
					single_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
			QObject::disconnect(
					double_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
			switch (visual_layer_params->get_isosurface_deviation_window_mode())
			{
			case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_NONE:
				no_deviation_window_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE:
				single_deviation_window_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE:
				double_deviation_window_mode_button->setChecked(true);
				break;
			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
			QObject::connect(
					no_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
			QObject::connect(
					single_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
			QObject::connect(
					double_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));

			QObject::connect(
					no_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
			QObject::connect(
					single_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));
			QObject::connect(
					double_deviation_window_mode_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_isosurface_deviation_window_mode_button(bool)));

			//
			// Set the isovalues.
			//

			const GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters &isovalue_parameters =
					visual_layer_params->get_isovalue_parameters();

			const int slider_isovalue1 = get_slider_isovalue(isovalue_parameters.isovalue1, layer, isovalue1_slider);
			const int slider_isovalue2 = get_slider_isovalue(isovalue_parameters.isovalue2, layer, isovalue2_slider);

			QObject::disconnect(
					isovalue1_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_isovalue_spinbox_changed(double)));
			QObject::disconnect(
					isovalue2_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_isovalue_spinbox_changed(double)));
			QObject::disconnect(
					isovalue1_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isovalue_slider_changed(int)));
			QObject::disconnect(
					isovalue2_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isovalue_slider_changed(int)));
			QObject::disconnect(
					isovalue1_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_deviation_spinbox_changed(double)));
			QObject::disconnect(
					isovalue1_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_deviation_spinbox_changed(double)));
			QObject::disconnect(
					isovalue2_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_deviation_spinbox_changed(double)));
			QObject::disconnect(
					isovalue2_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_deviation_spinbox_changed(double)));
			QObject::disconnect(
					isovalue1_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));
			QObject::disconnect(
					isovalue2_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));
			isovalue1_spinbox->setMinimum(scalar_field_min);
			isovalue2_spinbox->setMinimum(scalar_field_min);
			isovalue1_spinbox->setMaximum(scalar_field_max);
			isovalue2_spinbox->setMaximum(scalar_field_max);
			isovalue1_spinbox->setSingleStep(single_step_isovalue);
			isovalue2_spinbox->setSingleStep(single_step_isovalue);
			isovalue1_spinbox->setValue(isovalue_parameters.isovalue1);
			isovalue2_spinbox->setValue(isovalue_parameters.isovalue2);
			isovalue1_slider->setValue(slider_isovalue1);
			isovalue2_slider->setValue(slider_isovalue2);
			isovalue1_lower_deviation_spin_box->setMinimum(0);
			isovalue1_upper_deviation_spin_box->setMinimum(0);
			isovalue2_lower_deviation_spin_box->setMinimum(0);
			isovalue2_upper_deviation_spin_box->setMinimum(0);
			isovalue1_symmetric_deviation_spin_box->setMinimum(0);
			isovalue2_symmetric_deviation_spin_box->setMinimum(0);
			isovalue1_lower_deviation_spin_box->setMaximum(scalar_field_max - scalar_field_min);
			isovalue1_upper_deviation_spin_box->setMaximum(scalar_field_max - scalar_field_min);
			isovalue2_lower_deviation_spin_box->setMaximum(scalar_field_max - scalar_field_min);
			isovalue2_upper_deviation_spin_box->setMaximum(scalar_field_max - scalar_field_min);
			isovalue1_symmetric_deviation_spin_box->setMaximum(scalar_field_max - scalar_field_min);
			isovalue2_symmetric_deviation_spin_box->setMaximum(scalar_field_max - scalar_field_min);
			isovalue1_lower_deviation_spin_box->setSingleStep(single_step_deviation);
			isovalue1_upper_deviation_spin_box->setSingleStep(single_step_deviation);
			isovalue2_lower_deviation_spin_box->setSingleStep(single_step_deviation);
			isovalue2_upper_deviation_spin_box->setSingleStep(single_step_deviation);
			isovalue1_symmetric_deviation_spin_box->setSingleStep(single_step_deviation);
			isovalue2_symmetric_deviation_spin_box->setSingleStep(single_step_deviation);
			if (isovalue_parameters.symmetric_deviation)
			{
				// For symmetric deviations both lower and upper deviations have the same value.
				isovalue1_symmetric_deviation_spin_box->setValue(isovalue_parameters.lower_deviation1);
				isovalue2_symmetric_deviation_spin_box->setValue(isovalue_parameters.lower_deviation2);
			}
			else
			{
				isovalue1_lower_deviation_spin_box->setValue(isovalue_parameters.lower_deviation1);
				isovalue1_upper_deviation_spin_box->setValue(isovalue_parameters.upper_deviation1);
				isovalue2_lower_deviation_spin_box->setValue(isovalue_parameters.lower_deviation2);
				isovalue2_upper_deviation_spin_box->setValue(isovalue_parameters.upper_deviation2);
			}
			QObject::connect(
					isovalue1_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_isovalue_spinbox_changed(double)));
			QObject::connect(
					isovalue2_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_isovalue_spinbox_changed(double)));
			QObject::connect(
					isovalue1_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isovalue_slider_changed(int)));
			QObject::connect(
					isovalue2_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isovalue_slider_changed(int)));
			QObject::connect(
					isovalue1_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_deviation_spinbox_changed(double)));
			QObject::connect(
					isovalue1_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_deviation_spinbox_changed(double)));
			QObject::connect(
					isovalue2_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_deviation_spinbox_changed(double)));
			QObject::connect(
					isovalue2_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_deviation_spinbox_changed(double)));
			QObject::connect(
					isovalue1_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));
			QObject::connect(
					isovalue2_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));

			QObject::disconnect(
					symmetric_deviation_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_symmetric_deviation_check_box_changed()));
			symmetric_deviation_button->setChecked(isovalue_parameters.symmetric_deviation);
			QObject::connect(
					symmetric_deviation_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_symmetric_deviation_check_box_changed()));

			//
			// Set the deviation window render options.
			//

			const GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions
					deviation_window_render_options = visual_layer_params->get_deviation_window_render_options();
			QObject::disconnect(
					opacity_deviation_surfaces_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_deviation_surfaces_spinbox_changed(double)));
			opacity_deviation_surfaces_spin_box->setValue(
					deviation_window_render_options.opacity_deviation_surfaces);
			QObject::connect(
					opacity_deviation_surfaces_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_deviation_surfaces_spinbox_changed(double)));
			volume_render_deviation_window_button->setChecked(
					deviation_window_render_options.deviation_window_volume_rendering);
			QObject::disconnect(
					opacity_deviation_volume_rendering_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_deviation_volume_rendering_spinbox_changed(double)));
			opacity_deviation_volume_rendering_spin_box->setValue(
					deviation_window_render_options.opacity_deviation_window_volume_rendering);
			QObject::connect(
					opacity_deviation_volume_rendering_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_deviation_volume_rendering_spinbox_changed(double)));
			surface_deviation_window_button->setChecked(
					deviation_window_render_options.surface_deviation_window);
			QObject::disconnect(
					isoline_frequency_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isoline_frequency_spinbox_changed(int)));
			isoline_frequency_spin_box->setValue(
					deviation_window_render_options.surface_deviation_window_isoline_frequency);
			QObject::connect(
					isoline_frequency_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isoline_frequency_spinbox_changed(int)));

			//
			// Set the surface polygons mask.
			//

			const GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask surface_polygons_mask =
					visual_layer_params->get_surface_polygons_mask();
			QObject::disconnect(
					enable_surface_polygons_mask_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_surface_polygons_mask_check_box_changed()));
			QObject::disconnect(
					show_polygon_walls_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_surface_polygons_mask_check_box_changed()));
			QObject::disconnect(
					only_show_boundary_walls_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_surface_polygons_mask_check_box_changed()));
			QObject::disconnect(
					treat_polylines_as_polygons_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_surface_polygons_mask_check_box_changed()));
			enable_surface_polygons_mask_button->setChecked(surface_polygons_mask.enable_surface_polygons_mask);
			show_polygon_walls_button->setChecked(surface_polygons_mask.show_polygon_walls);
			treat_polylines_as_polygons_button->setChecked(surface_polygons_mask.treat_polylines_as_polygons);
			only_show_boundary_walls_button->setChecked(surface_polygons_mask.only_show_boundary_walls);
			QObject::connect(
					enable_surface_polygons_mask_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_surface_polygons_mask_check_box_changed()));
			QObject::connect(
					show_polygon_walls_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_surface_polygons_mask_check_box_changed()));
			QObject::connect(
					only_show_boundary_walls_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_surface_polygons_mask_check_box_changed()));
			QObject::connect(
					treat_polylines_as_polygons_button, SIGNAL(stateChanged(int)),
					this, SLOT(handle_surface_polygons_mask_check_box_changed()));

			//
			// Set the depth restriction.
			//

			const std::pair<double, double> depth_min_max = get_depth_min_max(layer);
			const double depth_min = depth_min_max.first;
			const double depth_max = depth_min_max.second;
			GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction depth_restriction =
					visual_layer_params->get_depth_restriction();
			// Ensure the depth restriction range is within the actual depth range.
			// The depth restriction range starts out as [0,1].
			if (depth_restriction.min_depth_radius_restriction < depth_min)
			{
				depth_restriction.min_depth_radius_restriction = depth_min;
			}
			if (depth_restriction.max_depth_radius_restriction > depth_max)
			{
				depth_restriction.max_depth_radius_restriction = depth_max;
			}
			QObject::disconnect(
					min_depth_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_depth_restriction_spinbox_changed(double)));
			min_depth_spin_box->setMinimum(depth_min);
			min_depth_spin_box->setMaximum(depth_max);
			min_depth_spin_box->setSingleStep((depth_max - depth_min) / 50.0);
			min_depth_spin_box->setValue(depth_restriction.min_depth_radius_restriction);
			QObject::connect(
					min_depth_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_depth_restriction_spinbox_changed(double)));
			QObject::disconnect(
					max_depth_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_depth_restriction_spinbox_changed(double)));
			max_depth_spin_box->setMinimum(depth_min);
			max_depth_spin_box->setMaximum(depth_max);
			max_depth_spin_box->setSingleStep((depth_max - depth_min) / 50.0);
			max_depth_spin_box->setValue(depth_restriction.max_depth_radius_restriction);
			QObject::connect(
					max_depth_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_depth_restriction_spinbox_changed(double)));

			//
			// Set the quality/performance.
			//

			const GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance quality_performance =
					visual_layer_params->get_quality_performance();
			QObject::disconnect(
					sampling_rate_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_quality_performance_spinbox_changed(int)));
			sampling_rate_spin_box->setValue(quality_performance.sampling_rate);
			QObject::connect(
					sampling_rate_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_quality_performance_spinbox_changed(int)));
			QObject::disconnect(
					bisection_iterations_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_quality_performance_spinbox_changed(int)));
			bisection_iterations_spin_box->setValue(quality_performance.bisection_iterations);
			QObject::connect(
					bisection_iterations_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_quality_performance_spinbox_changed(int)));

			// Set the "reduce rate during drag globe" check box.
			QObject::disconnect(
					improve_performance_during_drag_globe_check_box, SIGNAL(stateChanged(int)),
					this, SLOT(handle_improve_performance_during_globe_rotation_check_box_changed()));
			improve_performance_during_drag_globe_check_box->setChecked(
					quality_performance.enable_reduce_rate_during_drag_globe);
			QObject::connect(
					improve_performance_during_drag_globe_check_box, SIGNAL(stateChanged(int)),
					this, SLOT(handle_improve_performance_during_globe_rotation_check_box_changed()));
			// Set the sampling rate reduction factor.
			QObject::disconnect(
					improve_performance_during_drag_globe_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_quality_performance_spinbox_changed(int)));
			improve_performance_during_drag_globe_spin_box->setValue(quality_performance.reduce_rate_during_drag_globe);
			QObject::connect(
					improve_performance_during_drag_globe_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_quality_performance_spinbox_changed(int)));

			//
			// Set the shader test variables.
			//

			d_shader_test_variables = visual_layer_params->get_shader_test_variables();
			// If not yet set (or not size 16) then use default values.
			d_shader_test_variables.resize(NUM_SHADER_TEST_VARIABLES, 0.0f);

			QObject::disconnect(
					test_variable_0_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_0_spinbox->setValue(d_shader_test_variables[0]);
			QObject::connect(
					test_variable_0_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_1_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_1_spinbox->setValue(d_shader_test_variables[1]);
			QObject::connect(
					test_variable_1_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_2_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_2_spinbox->setValue(d_shader_test_variables[2]);
			QObject::connect(
					test_variable_2_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_3_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_3_spinbox->setValue(d_shader_test_variables[3]);
			QObject::connect(
					test_variable_3_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_4_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_4_spinbox->setValue(d_shader_test_variables[4]);
			QObject::connect(
					test_variable_4_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_5_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_5_spinbox->setValue(d_shader_test_variables[5]);
			QObject::connect(
					test_variable_5_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_6_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_6_spinbox->setValue(d_shader_test_variables[6]);
			QObject::connect(
					test_variable_6_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_7_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_7_spinbox->setValue(d_shader_test_variables[7]);
			QObject::connect(
					test_variable_7_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_8_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_8_spinbox->setValue(d_shader_test_variables[8]);
			QObject::connect(
					test_variable_8_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_9_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_9_spinbox->setValue(d_shader_test_variables[9]);
			QObject::connect(
					test_variable_9_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_10_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_10_spinbox->setValue(d_shader_test_variables[10]);
			QObject::connect(
					test_variable_10_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_11_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_11_spinbox->setValue(d_shader_test_variables[11]);
			QObject::connect(
					test_variable_11_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_12_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_12_spinbox->setValue(d_shader_test_variables[12]);
			QObject::connect(
					test_variable_12_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_13_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_13_spinbox->setValue(d_shader_test_variables[13]);
			QObject::connect(
					test_variable_13_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_14_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_14_spinbox->setValue(d_shader_test_variables[14]);
			QObject::connect(
					test_variable_14_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			QObject::disconnect(
					test_variable_15_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));
			test_variable_15_spinbox->setValue(d_shader_test_variables[15]);
			QObject::connect(
					test_variable_15_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_test_variable_spinbox_changed(double)));

			//
			// During runtime the appropriate GUI controls based on the default render mode, colour mode, etc,
			// are enabled/disabled when the various GUI slots get called.
			// But this doesn't happen when the visual layer state is first set up in 'set_data()' because slots
			// don't necessarily get called if the state in the GUI control (eg, checkbox) does not actually change.
			// So to get things started the appropriate widgets are disabled in Qt designer for its render mode state.
			//
			enable_disable_options_for_visual_layer_params(
					visual_layer_params->get_scalar_field_3d_render_parameters());
		}
	}
}


const QString &
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::get_title()
{
	static const QString TITLE = "Scalar field options";
	return TITLE;
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::enable_disable_options_for_visual_layer_params(
		const GPlatesViewOperations::ScalarField3DRenderParameters &scalar_field_render_parameters)
{
	// For the deviation spin boxes we start out hiding either the symmetric deviation spin boxes or
	// the lower/upper deviation spin boxes depending on the current state.
	if (scalar_field_render_parameters.get_isovalue_parameters().symmetric_deviation)
	{
		isovalue1_lower_deviation_widget->hide();
		isovalue1_upper_deviation_widget->hide();
		isovalue2_lower_deviation_widget->hide();
		isovalue2_upper_deviation_widget->hide();
		isovalue1_symmetric_deviation_widget->show();
		isovalue2_symmetric_deviation_widget->show();
	}
	else
	{
		isovalue1_lower_deviation_widget->show();
		isovalue1_upper_deviation_widget->show();
		isovalue2_lower_deviation_widget->show();
		isovalue2_upper_deviation_widget->show();
		isovalue1_symmetric_deviation_widget->hide();
		isovalue2_symmetric_deviation_widget->hide();
	}

	//
	// Enable/disable isosurface deviation window mode.
	//

	if (scalar_field_render_parameters.get_render_mode() ==
		GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS)
	{
		isosurface_deviation_window_mode_groupbox->hide();
	}
	else
	{
		isosurface_deviation_window_mode_groupbox->show();
	}

	//
	// Enable/disable colour mode options.
	//

	if (scalar_field_render_parameters.get_render_mode() ==
		GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS)
	{
		isosurface_colour_mode_group_box->hide();
		cross_sections_colour_mode_group_box->show();
	}
	else
	{
		isosurface_colour_mode_group_box->show();
		cross_sections_colour_mode_group_box->hide();
	}

	//
	// Enable/disable colour palette.
	//

	switch (scalar_field_render_parameters.get_render_mode())
	{
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE:
		switch (scalar_field_render_parameters.get_isosurface_colour_mode())
		{
		case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_DEPTH:
			scalar_colour_mapping_group_box->hide();
			gradient_colour_mapping_group_box->hide();
			break;
		case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_SCALAR:
			scalar_colour_mapping_group_box->show();
			gradient_colour_mapping_group_box->hide();
			break;
		case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_GRADIENT:
			scalar_colour_mapping_group_box->hide();
			gradient_colour_mapping_group_box->show();
			break;
		default:
			break;
		}
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS:
		switch (scalar_field_render_parameters.get_cross_section_colour_mode())
		{
		case GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_SCALAR:
			scalar_colour_mapping_group_box->show();
			gradient_colour_mapping_group_box->hide();
			break;
		case GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_GRADIENT:
			scalar_colour_mapping_group_box->hide();
			gradient_colour_mapping_group_box->show();
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	//
	// Enable/disable isovalue options.
	//

	switch (scalar_field_render_parameters.get_isosurface_deviation_window_mode())
	{
	case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_NONE:
		isovalue1_deviation_group_box->hide();
		isovalue2_widget->hide();
		isovalue2_deviation_group_box->hide();
		symmetric_deviation_button->hide();
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE:
		isovalue1_deviation_group_box->show();
		isovalue2_widget->hide();
		isovalue2_deviation_group_box->hide();
		symmetric_deviation_button->show();
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE:
		isovalue1_deviation_group_box->show();
		isovalue2_widget->show();
		isovalue2_deviation_group_box->show();
		symmetric_deviation_button->show();
	default:
		break;
	}

	if (scalar_field_render_parameters.get_render_mode() ==
		GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS)
	{
		isovalue_group_box->hide();
	}
	else
	{
		isovalue_group_box->show();
	}

	//
	// Enable/disable deviation window render options.
	//

	if (scalar_field_render_parameters.get_render_mode() ==
			GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS)
	{
		deviation_window_render_options_group_box->hide();
	}
	else if (scalar_field_render_parameters.get_render_mode() ==
			GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE &&
		scalar_field_render_parameters.get_isosurface_deviation_window_mode() ==
			GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_NONE)
	{
		deviation_window_render_options_group_box->hide();
	}
	else
	{
		deviation_window_render_options_group_box->show();
	}

	if (scalar_field_render_parameters.get_deviation_window_render_options().deviation_window_volume_rendering)
	{
		opacity_deviation_volume_rendering_widget->show();
	}
	else
	{
		opacity_deviation_volume_rendering_widget->hide();
	}
	if (!scalar_field_render_parameters.get_deviation_window_render_options().surface_deviation_window &&
		(!scalar_field_render_parameters.get_surface_polygons_mask().enable_surface_polygons_mask ||
			!scalar_field_render_parameters.get_surface_polygons_mask().show_polygon_walls))
	{
		isoline_frequency_widget->hide();
	}
	else
	{
		isoline_frequency_widget->show();
	}

	//
	// Enable/disable surface polygons mask options.
	//

	if (scalar_field_render_parameters.get_render_mode() ==
		GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS)
	{
		iso_surface_surface_polygons_mask_widget->hide();
	}
	else
	{
		iso_surface_surface_polygons_mask_widget->show();
	}

	if (scalar_field_render_parameters.get_surface_polygons_mask().show_polygon_walls)
	{
		only_show_boundary_walls_widget->show();
	}
	else
	{
		only_show_boundary_walls_widget->hide();
	}

	if (scalar_field_render_parameters.get_surface_polygons_mask().enable_surface_polygons_mask)
	{
		surface_polygons_mask_widget->show();
	}
	else
	{
		surface_polygons_mask_widget->hide();
	}

	//
	// Enable/disable quality/sampling options.
	//

	if (scalar_field_render_parameters.get_render_mode() ==
		GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS)
	{
		quality_performance_group_box->hide();
	}
	else
	{
		quality_performance_group_box->show();
	}

	if (scalar_field_render_parameters.get_quality_performance().enable_reduce_rate_during_drag_globe)
	{
		sample_rate_reduction_widget->show();
	}
	else
	{
		sample_rate_reduction_widget->hide();
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_render_mode_button(
		bool checked)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			if (isosurface_render_mode_button->isChecked())
			{
				visual_layer_params->set_render_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE);
			}
			if (cross_sections_render_mode_button->isChecked())
			{
				visual_layer_params->set_render_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS);
			}

			// Display the appropriate colour mode buttons.
			isosurface_colour_mode_group_box->setVisible(
					isosurface_render_mode_button->isChecked());
			cross_sections_colour_mode_group_box->setVisible(
					cross_sections_render_mode_button->isChecked());

			// Display the scalar colour mapping if scalar colour mode is selected.
			scalar_colour_mapping_group_box->setVisible(
					(isosurface_render_mode_button->isChecked() &&
						isosurface_scalar_colour_mode_button->isChecked()) ||
					(cross_sections_render_mode_button->isChecked() &&
						cross_sections_scalar_colour_mode_button->isChecked()));
			// Display the gradient colour mapping if gradient colour mode is selected.
			gradient_colour_mapping_group_box->setVisible(
					(isosurface_render_mode_button->isChecked() &&
						isosurface_gradient_colour_mode_button->isChecked()) ||
					(cross_sections_render_mode_button->isChecked() &&
						cross_sections_gradient_colour_mode_button->isChecked()));

			// The deviation window render modes do not apply to cross-sections.
			isosurface_deviation_window_mode_groupbox->setVisible(
					isosurface_render_mode_button->isChecked());

			// Isovalue options don't apply to cross-sections.
			isovalue_group_box->setVisible(
					isosurface_render_mode_button->isChecked());

			// Some surface polygon masks options do not apply to cross-sections.
			iso_surface_surface_polygons_mask_widget->setVisible(
					isosurface_render_mode_button->isChecked());

			// The quality/performance options do not apply to cross-sections.
			quality_performance_group_box->setVisible(
					isosurface_render_mode_button->isChecked());
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_isosurface_deviation_window_mode_button(
		bool checked)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			if (no_deviation_window_mode_button->isChecked())
			{
				visual_layer_params->set_isosurface_deviation_window_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_NONE);
			}
			if (single_deviation_window_mode_button->isChecked())
			{
				visual_layer_params->set_isosurface_deviation_window_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE);
			}
			if (double_deviation_window_mode_button->isChecked())
			{
				visual_layer_params->set_isosurface_deviation_window_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE);
			}

			// The first isovalue deviation options only apply to single/double deviation window rendering.
			isovalue1_deviation_group_box->setVisible(
					isosurface_render_mode_button->isChecked() &&
						(single_deviation_window_mode_button->isChecked() ||
							double_deviation_window_mode_button->isChecked()));

			// Second isovalue only applies to double deviation window rendering.
			isovalue2_widget->setVisible(
					isosurface_render_mode_button->isChecked() &&
						double_deviation_window_mode_button->isChecked());
			// The second isovalue deviation options only apply to double deviation window rendering.
			isovalue2_deviation_group_box->setVisible(
					isosurface_render_mode_button->isChecked() &&
						double_deviation_window_mode_button->isChecked());

			// Deviation only applies to single/double deviation window rendering.
			symmetric_deviation_button->setVisible(
					isosurface_render_mode_button->isChecked() &&
						(single_deviation_window_mode_button->isChecked() ||
							double_deviation_window_mode_button->isChecked()));

			// The deviation window render options only apply to single/double deviation window rendering.
			deviation_window_render_options_group_box->setVisible(
					isosurface_render_mode_button->isChecked() &&
						(single_deviation_window_mode_button->isChecked() ||
							double_deviation_window_mode_button->isChecked()));

			// If the render mode is single or double deviation window then we need to ensure the
			// isovalue deviation window(s) do not overlap or exceed the range [min_scalar,max_scalar].
			// The constraints can get violated, for example, when the isovalue1 has had free range
			// while in 'isosurface' render mode but upon switching to 'single deviation window' mode
			// the 'window' may overlap the minimum or maximum scalar field value.
			if (isosurface_render_mode_button->isChecked() &&
				(single_deviation_window_mode_button->isChecked() ||
					double_deviation_window_mode_button->isChecked()))
			{
				GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
				const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);
				const double scalar_field_min = scalar_field_min_max.first;
				const double scalar_field_max = scalar_field_min_max.second;

				GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters =
						visual_layer_params->get_isovalue_parameters();

				if (single_deviation_window_mode_button->isChecked())
				{
					if (isovalue_parameters.lower_deviation1 > isovalue_parameters.isovalue1 - scalar_field_min)
					{
						isovalue_parameters.lower_deviation1 = isovalue_parameters.isovalue1 - scalar_field_min;
						if (symmetric_deviation_button->isChecked())
						{
							isovalue_parameters.upper_deviation1 = isovalue_parameters.lower_deviation1;
						}
					}
					if (isovalue_parameters.upper_deviation1 > scalar_field_max - isovalue_parameters.isovalue1)
					{
						isovalue_parameters.upper_deviation1 = scalar_field_max - isovalue_parameters.isovalue1;
						if (symmetric_deviation_button->isChecked())
						{
							isovalue_parameters.lower_deviation1 = isovalue_parameters.upper_deviation1;
						}
					}
				}
				else // double deviation window ...
				{
					if (isovalue_parameters.lower_deviation1 > isovalue_parameters.isovalue1 - scalar_field_min)
					{
						isovalue_parameters.lower_deviation1 = isovalue_parameters.isovalue1 - scalar_field_min;
						if (symmetric_deviation_button->isChecked())
						{
							isovalue_parameters.upper_deviation1 = isovalue_parameters.lower_deviation1;
						}
					}
					if (isovalue_parameters.isovalue2 < isovalue_parameters.isovalue1)
					{
						isovalue_parameters.isovalue2 = isovalue_parameters.isovalue1;
						isovalue_parameters.upper_deviation1 = 0;
						isovalue_parameters.lower_deviation2 = 0;
						if (symmetric_deviation_button->isChecked())
						{
							isovalue_parameters.lower_deviation1 = isovalue_parameters.upper_deviation1;
							isovalue_parameters.upper_deviation2 = isovalue_parameters.lower_deviation2;
						}
					}
					if (isovalue_parameters.upper_deviation1 >
						isovalue_parameters.isovalue2 - isovalue_parameters.isovalue1)
					{
						isovalue_parameters.upper_deviation1 =
								isovalue_parameters.isovalue2 - isovalue_parameters.isovalue1;
						if (symmetric_deviation_button->isChecked())
						{
							isovalue_parameters.lower_deviation1 = isovalue_parameters.upper_deviation1;
						}
					}
					if (isovalue_parameters.lower_deviation2 >
						isovalue_parameters.isovalue2 - isovalue_parameters.isovalue1 - isovalue_parameters.upper_deviation1)
					{
						isovalue_parameters.lower_deviation2 =
								isovalue_parameters.isovalue2 - isovalue_parameters.isovalue1 - isovalue_parameters.upper_deviation1;
						if (symmetric_deviation_button->isChecked())
						{
							isovalue_parameters.upper_deviation2 = isovalue_parameters.lower_deviation2;
						}
					}
					if (isovalue_parameters.upper_deviation2 > scalar_field_max - isovalue_parameters.isovalue2)
					{
						isovalue_parameters.upper_deviation2 = scalar_field_max - isovalue_parameters.isovalue2;
						if (symmetric_deviation_button->isChecked())
						{
							isovalue_parameters.lower_deviation2 = isovalue_parameters.upper_deviation2;
						}
					}
				}

				visual_layer_params->set_isovalue_parameters(isovalue_parameters);
			}
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_isosurface_colour_mode_button(
		bool checked)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			if (isosurface_depth_colour_mode_button->isChecked())
			{
				visual_layer_params->set_isosurface_colour_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_DEPTH);
			}
			if (isosurface_scalar_colour_mode_button->isChecked())
			{
				visual_layer_params->set_isosurface_colour_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_SCALAR);
			}
			if (isosurface_gradient_colour_mode_button->isChecked())
			{
				visual_layer_params->set_isosurface_colour_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_GRADIENT);
			}

			// The scalar colour palette only applies when scalar colour mode is enabled.
			scalar_colour_mapping_group_box->setVisible(
					isosurface_scalar_colour_mode_button->isChecked());

			// The gradient colour palette only applies when gradient colour mode is enabled.
			gradient_colour_mapping_group_box->setVisible(
					isosurface_gradient_colour_mode_button->isChecked());
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_cross_sections_colour_mode_button(
		bool checked)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			if (cross_sections_scalar_colour_mode_button->isChecked())
			{
				visual_layer_params->set_cross_section_colour_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_SCALAR);
			}
			if (cross_sections_gradient_colour_mode_button->isChecked())
			{
				visual_layer_params->set_cross_section_colour_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_GRADIENT);
			}

			// The scalar colour palette only applies when scalar colour mode is enabled.
			scalar_colour_mapping_group_box->setVisible(
					cross_sections_scalar_colour_mode_button->isChecked());

			// The gradient colour palette only applies when gradient colour mode is enabled.
			gradient_colour_mapping_group_box->setVisible(
					cross_sections_gradient_colour_mode_button->isChecked());
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_select_scalar_palette_filename_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		QString palette_file_name = d_open_file_dialog.get_open_file_name();
		if (palette_file_name.isEmpty())
		{
			return;
		}

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();

		GPlatesFileIO::ReadErrorAccumulation cpt_read_errors;

		// Update the colour palette in the layer params.
		GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
				params->get_scalar_colour_palette_parameters();
		// We only allow real-valued colour palettes since our data is real-valued...
		scalar_colour_palette_parameters.load_colour_palette(palette_file_name, cpt_read_errors);
		params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);

		// Show any CPT read errors.
		if (cpt_read_errors.size() > 0)
		{
			d_viewport_window->handle_read_errors(cpt_read_errors);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_use_default_scalar_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
					params->get_scalar_colour_palette_parameters();

			scalar_colour_palette_parameters.use_default_colour_palette();

			params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_builtin_scalar_colour_palette_selected(
		const GPlatesGui::BuiltinColourPaletteType &builtin_scalar_colour_palette_type)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Update the colour palette in the layer params.
			GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
					params->get_scalar_colour_palette_parameters();
			scalar_colour_palette_parameters.load_builtin_colour_palette(builtin_scalar_colour_palette_type);
			params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_builtin_scalar_parameters_changed(
		const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_scalar_parameters)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Update the built-in palette parameters in the layer params.
			GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
					params->get_scalar_colour_palette_parameters();
			scalar_colour_palette_parameters.set_builtin_colour_palette_parameters(builtin_scalar_parameters);
			params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_scalar_palette_range_check_box_changed(
		int state)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
					params->get_scalar_colour_palette_parameters();
			// Map or unmap the colour palette range.
			if (state == Qt::Checked)
			{
				scalar_colour_palette_parameters.map_palette_range(
						scalar_colour_palette_parameters.get_mapped_palette_range().first,
						scalar_colour_palette_parameters.get_mapped_palette_range().second);
			}
			else
			{
				scalar_colour_palette_parameters.unmap_palette_range();
			}
			params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_scalar_palette_min_line_editing_finished(
		double min_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
					visual_layer_params->get_scalar_colour_palette_parameters();

			const double max_value = scalar_colour_palette_parameters.get_palette_range().second/*max*/;

			// Ensure min not greater than max.
			if (min_value > max_value)
			{
				min_value = max_value;
			}

			scalar_colour_palette_parameters.map_palette_range(min_value, max_value);

			visual_layer_params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_scalar_palette_max_line_editing_finished(
		double max_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
					visual_layer_params->get_scalar_colour_palette_parameters();

			const double min_value = scalar_colour_palette_parameters.get_palette_range().first/*min*/;

			// Ensure max not less than min.
			if (max_value < min_value)
			{
				max_value = min_value;
			}

			scalar_colour_palette_parameters.map_palette_range(min_value, max_value);

			visual_layer_params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_scalar_palette_range_restore_min_max_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);

			GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
					params->get_scalar_colour_palette_parameters();

			scalar_colour_palette_parameters.map_palette_range(
					scalar_field_min_max.first,
					scalar_field_min_max.second);

			params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_scalar_palette_range_restore_mean_deviation_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_field_mean_std_dev = get_scalar_value_mean_std_dev(layer);

			GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
					params->get_scalar_colour_palette_parameters();

			double range_min = scalar_field_mean_std_dev.first -
					scalar_colour_palette_parameters.get_deviation_from_mean() * scalar_field_mean_std_dev.second;
			double range_max = scalar_field_mean_std_dev.first +
					scalar_colour_palette_parameters.get_deviation_from_mean() * scalar_field_mean_std_dev.second;

			scalar_colour_palette_parameters.map_palette_range(range_min, range_max);

			params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_scalar_palette_range_restore_mean_deviation_spinbox_changed(
		double deviation_from_mean)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
					visual_layer_params->get_scalar_colour_palette_parameters();

			scalar_colour_palette_parameters.set_deviation_from_mean(deviation_from_mean);

			visual_layer_params->set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_select_gradient_palette_filename_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		QString palette_file_name = d_open_file_dialog.get_open_file_name();
		if (palette_file_name.isEmpty())
		{
			return;
		}

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();

		GPlatesFileIO::ReadErrorAccumulation cpt_read_errors;

		// Update the colour palette in the layer params.
		GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
				params->get_gradient_colour_palette_parameters();
		// We only allow real-valued colour palettes since our data is real-valued...
		gradient_colour_palette_parameters.load_colour_palette(palette_file_name, cpt_read_errors);
		params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);

		// Show any CPT read errors.
		if (cpt_read_errors.size() > 0)
		{
			d_viewport_window->handle_read_errors(cpt_read_errors);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_use_default_gradient_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
					params->get_gradient_colour_palette_parameters();

			gradient_colour_palette_parameters.use_default_colour_palette();

			params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_builtin_gradient_colour_palette_selected(
		const GPlatesGui::BuiltinColourPaletteType &builtin_gradient_colour_palette_type)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Update the colour palette in the layer params.
			GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
					params->get_gradient_colour_palette_parameters();
			gradient_colour_palette_parameters.load_builtin_colour_palette(builtin_gradient_colour_palette_type);
			params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_builtin_gradient_parameters_changed(
		const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_gradient_parameters)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Update the built-in palette parameters in the layer params.
			GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
					params->get_gradient_colour_palette_parameters();
			gradient_colour_palette_parameters.set_builtin_colour_palette_parameters(builtin_gradient_parameters);
			params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_gradient_palette_range_check_box_changed(
		int state)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
					params->get_gradient_colour_palette_parameters();
			// Map or unmap the colour palette range.
			if (state == Qt::Checked)
			{
				gradient_colour_palette_parameters.map_palette_range(
						gradient_colour_palette_parameters.get_mapped_palette_range().first,
						gradient_colour_palette_parameters.get_mapped_palette_range().second);
			}
			else
			{
				gradient_colour_palette_parameters.unmap_palette_range();
			}
			params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_gradient_palette_min_line_editing_finished(
		double min_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
					visual_layer_params->get_gradient_colour_palette_parameters();

			const double max_value = gradient_colour_palette_parameters.get_palette_range().second/*max*/;

			// Ensure min not greater than max.
			if (min_value > max_value)
			{
				min_value = max_value;
			}

			gradient_colour_palette_parameters.map_palette_range(min_value, max_value);

			visual_layer_params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_gradient_palette_max_line_editing_finished(
		double max_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
					visual_layer_params->get_gradient_colour_palette_parameters();

			const double min_value = gradient_colour_palette_parameters.get_palette_range().first/*min*/;

			// Ensure max not less than min.
			if (max_value < min_value)
			{
				max_value = min_value;
			}

			gradient_colour_palette_parameters.map_palette_range(min_value, max_value);

			visual_layer_params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_gradient_palette_range_restore_min_max_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> gradient_magnitude_min_max = get_gradient_magnitude_min_max(layer);

			GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
					params->get_gradient_colour_palette_parameters();

			gradient_colour_palette_parameters.map_palette_range(
					-gradient_magnitude_min_max.second/*min*/,
					gradient_magnitude_min_max.second/*max*/);

			params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_gradient_palette_range_restore_mean_deviation_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> gradient_magnitude_mean_std_dev = get_gradient_magnitude_mean_std_dev(layer);

			GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
					params->get_gradient_colour_palette_parameters();

			double range_min = -gradient_magnitude_mean_std_dev.first -
					gradient_colour_palette_parameters.get_deviation_from_mean() * gradient_magnitude_mean_std_dev.second;
			double range_max = gradient_magnitude_mean_std_dev.first +
					gradient_colour_palette_parameters.get_deviation_from_mean() * gradient_magnitude_mean_std_dev.second;

			gradient_colour_palette_parameters.map_palette_range(range_min, range_max);

			params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_gradient_palette_range_restore_mean_deviation_spinbox_changed(
		double deviation_from_mean)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
					visual_layer_params->get_gradient_colour_palette_parameters();

			gradient_colour_palette_parameters.set_deviation_from_mean(deviation_from_mean);

			visual_layer_params->set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_isovalue_spinbox_changed(
		double isovalue)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			// Get the QObject that triggered this slot.
			QObject *signal_sender = sender();
			// Return early in case this slot not activated by a signal - shouldn't happen.
			if (!signal_sender)
			{
				return;
			}

			// Cast to a QDoubleSpinBox since only QDoubleSpinBox objects trigger this slot.
			QDoubleSpinBox *isovalue_spinbox = qobject_cast<QDoubleSpinBox *>(signal_sender);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					isovalue_spinbox,
					GPLATES_ASSERTION_SOURCE);

			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);
			const double scalar_field_min = scalar_field_min_max.first;
			const double scalar_field_max = scalar_field_min_max.second;

			const GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode
					isosurface_deviation_window_mode = visual_layer_params->get_isosurface_deviation_window_mode();

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters =
					visual_layer_params->get_isovalue_parameters();

			QSlider *isovalue_slider = NULL;
			if (isovalue_spinbox == isovalue1_spinbox)
			{
				// Ensure deviation does not violate constraints imposed by the isovalue deviation windows.
				if (isosurface_deviation_window_mode ==
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE ||
					isosurface_deviation_window_mode ==
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE)
				{
					// Same lower limit condition for both single and double deviation windows.
					const double isovalue_lower_limit = scalar_field_min + isovalue_parameters.lower_deviation1;
					if (isovalue < isovalue_lower_limit)
					{
						// Setting the spinbox value will trigger this slot again so return after setting.
						isovalue1_spinbox->setValue(isovalue_lower_limit);
						return;
					}
					// Upper limit condition differs between single and double deviation windows.
					const double isovalue_upper_limit =
							(isosurface_deviation_window_mode ==
								GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE)
							? scalar_field_max - isovalue_parameters.upper_deviation1
							: isovalue_parameters.isovalue2 - isovalue_parameters.lower_deviation2 -
									isovalue_parameters.upper_deviation1;
					if (isovalue > isovalue_upper_limit)
					{
						// Setting the spinbox value will trigger this slot again so return after setting.
						isovalue1_spinbox->setValue(isovalue_upper_limit);
						return;
					}
				}
				isovalue_parameters.isovalue1 = isovalue;
				isovalue_slider = isovalue1_slider;
			}
			else
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isovalue_spinbox == isovalue2_spinbox,
						GPLATES_ASSERTION_SOURCE);
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,
						GPLATES_ASSERTION_SOURCE);
				// Ensure isovalue does not violate constraints imposed by the isovalue deviation windows.
				// Lower limit condition.
				const double isovalue_lower_limit =
						isovalue_parameters.isovalue1 + isovalue_parameters.upper_deviation1 + isovalue_parameters.lower_deviation2;
				if (isovalue < isovalue_lower_limit)
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					isovalue2_spinbox->setValue(isovalue_lower_limit);
					return;
				}
				// Upper limit condition.
				const double isovalue_upper_limit = scalar_field_max - isovalue_parameters.upper_deviation2;
				if (isovalue > isovalue_upper_limit)
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					isovalue2_spinbox->setValue(isovalue_upper_limit);
					return;
				}
				isovalue_parameters.isovalue2 = isovalue;
				isovalue_slider = isovalue2_slider;
			}

			visual_layer_params->set_isovalue_parameters(isovalue_parameters);

			// Keep the isovalue slider in-sync...

			// Prevent its slot from being triggered.
			QObject::disconnect(
					isovalue_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isovalue_slider_changed(int)));

			isovalue_slider->setValue(
					get_slider_isovalue(
							isovalue,
							locked_visual_layer->get_reconstruct_graph_layer(),
							isovalue_slider));

			QObject::connect(
					isovalue_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isovalue_slider_changed(int)));
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_isovalue_slider_changed(
		int value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();

			const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);
			const double scalar_field_min = scalar_field_min_max.first;
			const double scalar_field_max = scalar_field_min_max.second;

			// Get the QObject that triggered this slot.
			QObject *signal_sender = sender();
			// Return early in case this slot not activated by a signal - shouldn't happen.
			if (!signal_sender)
			{
				return;
			}

			// Cast to a QSlider since only QSlider objects trigger this slot.
			QSlider *isovalue_slider = qobject_cast<QSlider *>(signal_sender);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					isovalue_slider,
					GPLATES_ASSERTION_SOURCE);

			// Convert slider value to the range [0,1].
			const double slider_ratio = (double(value) - isovalue_slider->minimum()) /
					(isovalue_slider->maximum() - isovalue_slider->minimum());

			const double isovalue = scalar_field_min + slider_ratio * (scalar_field_max - scalar_field_min);

			const GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode
					isosurface_deviation_window_mode = visual_layer_params->get_isosurface_deviation_window_mode();

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters =
					visual_layer_params->get_isovalue_parameters();

			QDoubleSpinBox *isovalue_spin_box = NULL;
			if (isovalue_slider == isovalue1_slider)
			{
				// Ensure deviation does not violate constraints imposed by the isovalue deviation windows.
				if (isosurface_deviation_window_mode ==
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE ||
					isosurface_deviation_window_mode ==
						GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE)
				{
					// Same lower limit condition for both single and double deviation windows.
					const double isovalue_lower_limit = scalar_field_min + isovalue_parameters.lower_deviation1;
					if (isovalue < isovalue_lower_limit)
					{
						// Setting the slider value will trigger this slot again so return after setting.
						// Increment slider value by one and try again.
						// This had the effect of clamping to the closest integer value satisfying constraint.
						isovalue1_slider->setValue(value + 1);
						return;
					}
					// Upper limit condition differs between single and double deviation windows.
					const double isovalue_upper_limit =
							(isosurface_deviation_window_mode ==
								GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE)
							? scalar_field_max - isovalue_parameters.upper_deviation1
							: isovalue_parameters.isovalue2 - isovalue_parameters.lower_deviation2 - isovalue_parameters.upper_deviation1;
					if (isovalue > isovalue_upper_limit)
					{
						// Setting the slider value will trigger this slot again so return after setting.
						// Decrement slider value by one and try again.
						// This had the effect of clamping to the closest integer value satisfying constraint.
						isovalue1_slider->setValue(value - 1);
						return;
					}
				}
				isovalue_parameters.isovalue1 = isovalue;
				isovalue_spin_box = isovalue1_spinbox;
			}
			else
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isovalue_slider == isovalue2_slider,
						GPLATES_ASSERTION_SOURCE);
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,
						GPLATES_ASSERTION_SOURCE);
				// Ensure isovalue does not violate constraints imposed by the isovalue deviation windows.
				// Lower limit condition.
				const double isovalue_lower_limit =
						isovalue_parameters.isovalue1 + isovalue_parameters.upper_deviation1 + isovalue_parameters.lower_deviation2;
				if (isovalue < isovalue_lower_limit)
				{
					// Setting the slider value will trigger this slot again so return after setting.
					// Increment slider value by one and try again.
					// This had the effect of clamping to the closest integer value satisfying constraint.
					isovalue2_slider->setValue(value + 1);
					return;
				}
				// Upper limit condition.
				const double isovalue_upper_limit = scalar_field_max - isovalue_parameters.upper_deviation2;
				if (isovalue > isovalue_upper_limit)
				{
					// Setting the slider value will trigger this slot again so return after setting.
					// Decrement slider value by one and try again.
					// This had the effect of clamping to the closest integer value satisfying constraint.
					isovalue2_slider->setValue(value - 1);
					return;
				}
				isovalue_parameters.isovalue2 = isovalue;
				isovalue_spin_box = isovalue2_spinbox;
			}

			visual_layer_params->set_isovalue_parameters(isovalue_parameters);

			// Keep the isovalue spin box in-sync.
			// And we use the spin box to set our value.
			// This avoids issues with violating the constraints on the values of the isovalue
			// parameters that might occur during 'double -> integer' truncation since slider
			// uses integer values and spin box uses double values.

			QObject::disconnect(
					isovalue_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_isovalue_spinbox_changed(double)));

			// Since we are not disconnecting the spin box slot it will get called here.
			//
			// NOTE: We want the spin box slot to be called since it ensures constraints
			// on the isovalue parameter values and in turn sets our slider isovalue to the possibly
			// constraint-clamped isovalue.
			isovalue_spin_box->setValue(isovalue);

			QObject::connect(
					isovalue_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_isovalue_spinbox_changed(double)));
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_deviation_spinbox_changed(
		double deviation)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			// Get the QObject that triggered this slot.
			QObject *signal_sender = sender();
			// Return early in case this slot not activated by a signal - shouldn't happen.
			if (!signal_sender)
			{
				return;
			}

			// Cast to a QDoubleSpinBox since only QDoubleSpinBox objects trigger this slot.
			QDoubleSpinBox *deviation_spinbox = qobject_cast<QDoubleSpinBox *>(signal_sender);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					deviation_spinbox,
					GPLATES_ASSERTION_SOURCE);

			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);
			const double scalar_field_min = scalar_field_min_max.first;
			const double scalar_field_max = scalar_field_min_max.second;

			const GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode
					isosurface_deviation_window_mode = visual_layer_params->get_isosurface_deviation_window_mode();

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters =
					visual_layer_params->get_isovalue_parameters();

			if (deviation_spinbox == isovalue1_lower_deviation_spin_box)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE ||
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,
						GPLATES_ASSERTION_SOURCE);
				// Ensure deviation does not violate constraints imposed by the isovalue deviation windows.
				// Same condition for both single and double deviation windows.
				const double deviation_limit = isovalue_parameters.isovalue1 - scalar_field_min;
				if (deviation > deviation_limit)
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					isovalue1_lower_deviation_spin_box->setValue(deviation_limit);
					return;
				}
				isovalue_parameters.lower_deviation1 = deviation;
			}
			else if (deviation_spinbox == isovalue1_upper_deviation_spin_box)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE ||
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,
						GPLATES_ASSERTION_SOURCE);
				// Ensure isovalue does not violate constraints imposed by the isovalue deviation windows.
				const double deviation_limit =
						(isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE)
						? scalar_field_max - isovalue_parameters.isovalue1
						: isovalue_parameters.isovalue2 - isovalue_parameters.isovalue1 - isovalue_parameters.lower_deviation2;
				if (deviation > deviation_limit)
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					isovalue1_upper_deviation_spin_box->setValue(deviation_limit);
					return;
				}
				isovalue_parameters.upper_deviation1 = deviation;
			}
			else if (deviation_spinbox == isovalue2_lower_deviation_spin_box)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,
						GPLATES_ASSERTION_SOURCE);
				const double deviation_limit =
						isovalue_parameters.isovalue2 - isovalue_parameters.isovalue1 - isovalue_parameters.upper_deviation1;
				if (deviation > deviation_limit)
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					isovalue2_lower_deviation_spin_box->setValue(deviation_limit);
					return;
				}
				isovalue_parameters.lower_deviation2 = deviation;
			}
			else if (deviation_spinbox == isovalue2_upper_deviation_spin_box)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,
						GPLATES_ASSERTION_SOURCE);
				const double deviation_limit = scalar_field_max - isovalue_parameters.isovalue2;
				if (deviation > deviation_limit)
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					isovalue2_upper_deviation_spin_box->setValue(deviation_limit);
					return;
				}
				isovalue_parameters.upper_deviation2 = deviation;
			}

			visual_layer_params->set_isovalue_parameters(isovalue_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_symmetric_deviation_spinbox_changed(
		double symmetric_deviation)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			// Get the QObject that triggered this slot.
			QObject *signal_sender = sender();
			// Return early in case this slot not activated by a signal - shouldn't happen.
			if (!signal_sender)
			{
				return;
			}

			// Cast to a QDoubleSpinBox since only QDoubleSpinBox objects trigger this slot.
			QDoubleSpinBox *symmetric_deviation_spinbox = qobject_cast<QDoubleSpinBox *>(signal_sender);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					symmetric_deviation_spinbox,
					GPLATES_ASSERTION_SOURCE);

			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);
			const double scalar_field_min = scalar_field_min_max.first;
			const double scalar_field_max = scalar_field_min_max.second;

			const GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode
					isosurface_deviation_window_mode = visual_layer_params->get_isosurface_deviation_window_mode();

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters =
					visual_layer_params->get_isovalue_parameters();

			if (symmetric_deviation_spinbox == isovalue1_symmetric_deviation_spin_box)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE ||
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,
						GPLATES_ASSERTION_SOURCE);
				// Ensure deviation does not violate constraints imposed by the isovalue deviation windows.
				// Same condition for both single and double deviation windows.
				double symmetric_deviation_limit;
				if (isosurface_deviation_window_mode ==
					GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE)
				{
					symmetric_deviation_limit = std::min<double>(
							isovalue_parameters.isovalue1 - scalar_field_min,
							scalar_field_max - isovalue_parameters.isovalue1);
				}
				else
				{
					symmetric_deviation_limit = std::min<double>(
							isovalue_parameters.isovalue1 - scalar_field_min,
							isovalue_parameters.isovalue2 - isovalue_parameters.isovalue1 - isovalue_parameters.lower_deviation2);
				}
				if (symmetric_deviation > symmetric_deviation_limit)
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					isovalue1_symmetric_deviation_spin_box->setValue(symmetric_deviation_limit);
					return;
				}
				isovalue_parameters.lower_deviation1 = symmetric_deviation;
				isovalue_parameters.upper_deviation1 = symmetric_deviation;
			}
			else if (symmetric_deviation_spinbox == isovalue2_symmetric_deviation_spin_box)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,
						GPLATES_ASSERTION_SOURCE);
				const double symmetric_deviation_limit = std::min<double>(
						scalar_field_max - isovalue_parameters.isovalue2,
						isovalue_parameters.isovalue2 - isovalue_parameters.isovalue1 - isovalue_parameters.upper_deviation1);
				if (symmetric_deviation > symmetric_deviation_limit)
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					isovalue2_symmetric_deviation_spin_box->setValue(symmetric_deviation_limit);
					return;
				}
				isovalue_parameters.lower_deviation2 = symmetric_deviation;
				isovalue_parameters.upper_deviation2 = symmetric_deviation;
			}

			visual_layer_params->set_isovalue_parameters(isovalue_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_symmetric_deviation_check_box_changed()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			const GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode
					isosurface_deviation_window_mode = visual_layer_params->get_isosurface_deviation_window_mode();

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE ||
					isosurface_deviation_window_mode ==
							GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,
					GPLATES_ASSERTION_SOURCE);

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters =
					visual_layer_params->get_isovalue_parameters();

			isovalue_parameters.symmetric_deviation = symmetric_deviation_button->isChecked();

			// If symmetric deviation has just been enabled then we need to ensure the lower and upper
			// deviations are the same and then also enforce constraints to avoid overlapping windows.
			if (symmetric_deviation_button->isChecked())
			{
				// The lower and upper deviations must now match.
				// Take the minimum of the lower and upper since this will always satisfy the
				// non-overlapping constraints.
				const float symmetric_deviation1 = std::min<float>(
						isovalue_parameters.lower_deviation1,
						isovalue_parameters.upper_deviation1);
				isovalue_parameters.lower_deviation1 = symmetric_deviation1;
				isovalue_parameters.upper_deviation1 = symmetric_deviation1;

				const float symmetric_deviation2 = std::min<float>(
						isovalue_parameters.lower_deviation2,
						isovalue_parameters.upper_deviation2);
				isovalue_parameters.lower_deviation2 = symmetric_deviation2;
				isovalue_parameters.upper_deviation2 = symmetric_deviation2;

				// Copy the new symmetric deviations into the symmetric spin boxes.
				QObject::disconnect(
						isovalue1_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));
				QObject::disconnect(
						isovalue2_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));
				isovalue1_symmetric_deviation_spin_box->setValue(symmetric_deviation1);
				isovalue2_symmetric_deviation_spin_box->setValue(symmetric_deviation2);
				QObject::connect(
						isovalue1_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));
				QObject::connect(
						isovalue2_symmetric_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_symmetric_deviation_spinbox_changed(double)));

				// Hide the widgets containing the non-symmetric spin boxes.
				isovalue1_lower_deviation_widget->hide();
				isovalue1_upper_deviation_widget->hide();
				isovalue2_lower_deviation_widget->hide();
				isovalue2_upper_deviation_widget->hide();

				// Show the widgets containing the symmetric spin boxes.
				isovalue1_symmetric_deviation_widget->show();
				isovalue2_symmetric_deviation_widget->show();
			}
			else // not symmetric ...
			{
				// Copy the symmetric deviations into the non-symmetric spin boxes.
				// Since they are symmetric then lower and upper must be the same.
				QObject::disconnect(
						isovalue1_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_deviation_spinbox_changed(double)));
				QObject::disconnect(
						isovalue1_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_deviation_spinbox_changed(double)));
				QObject::disconnect(
						isovalue2_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_deviation_spinbox_changed(double)));
				QObject::disconnect(
						isovalue2_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_deviation_spinbox_changed(double)));
				isovalue1_lower_deviation_spin_box->setValue(isovalue_parameters.lower_deviation1);
				isovalue1_upper_deviation_spin_box->setValue(isovalue_parameters.upper_deviation1);
				isovalue2_lower_deviation_spin_box->setValue(isovalue_parameters.lower_deviation2);
				isovalue2_upper_deviation_spin_box->setValue(isovalue_parameters.upper_deviation2);
				QObject::connect(
						isovalue1_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_deviation_spinbox_changed(double)));
				QObject::connect(
						isovalue1_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_deviation_spinbox_changed(double)));
				QObject::connect(
						isovalue2_lower_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_deviation_spinbox_changed(double)));
				QObject::connect(
						isovalue2_upper_deviation_spin_box, SIGNAL(valueChanged(double)),
						this, SLOT(handle_deviation_spinbox_changed(double)));

				// Hide the widgets containing the symmetric spin boxes.
				isovalue1_symmetric_deviation_widget->hide();
				isovalue2_symmetric_deviation_widget->hide();

				// Show the widgets containing the non-symmetric spin boxes.
				isovalue1_lower_deviation_widget->show();
				isovalue1_upper_deviation_widget->show();
				isovalue2_lower_deviation_widget->show();
				isovalue2_upper_deviation_widget->show();
			}

			visual_layer_params->set_isovalue_parameters(isovalue_parameters);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_opacity_deviation_surfaces_spinbox_changed(
		double opacity)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions
					deviation_window_render_options = visual_layer_params->get_deviation_window_render_options();

			deviation_window_render_options.opacity_deviation_surfaces = opacity;

			visual_layer_params->set_deviation_window_render_options(deviation_window_render_options);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_volume_render_deviation_window_check_box_changed()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions
					deviation_window_render_options = visual_layer_params->get_deviation_window_render_options();

			deviation_window_render_options.deviation_window_volume_rendering =
					volume_render_deviation_window_button->isChecked();

			visual_layer_params->set_deviation_window_render_options(deviation_window_render_options);

			// The volume rendering opacity spinbox only applies if volume rendering is enabled.
			opacity_deviation_volume_rendering_widget->setVisible(
					volume_render_deviation_window_button->isChecked());
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_opacity_deviation_volume_rendering_spinbox_changed(
		double opacity)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions
					deviation_window_render_options = visual_layer_params->get_deviation_window_render_options();

			deviation_window_render_options.opacity_deviation_window_volume_rendering = opacity;

			visual_layer_params->set_deviation_window_render_options(deviation_window_render_options);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_surface_deviation_window_check_box_changed()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions
					deviation_window_render_options = visual_layer_params->get_deviation_window_render_options();

			deviation_window_render_options.surface_deviation_window =
					surface_deviation_window_button->isChecked();

			visual_layer_params->set_deviation_window_render_options(deviation_window_render_options);

			// The isoline frequency spinbox only applies if surface deviation is enabled and
			// either the outer spherical surface or the polygon walls are shown.
			// It's possible that show-polygon-walls is on and surface deviation is disabled but
			// we don't check for that because the isoline-frequency spinbox will be disabled anyway
			// because it's render options group box is disabled and it will be re-enabled once
			// the render options group box is re-enabled ().
			isoline_frequency_widget->setVisible(
					surface_deviation_window_button->isChecked() ||
						(enable_surface_polygons_mask_button->isChecked() &&
							show_polygon_walls_button->isChecked()));
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_isoline_frequency_spinbox_changed(
		int frequency)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions
					deviation_window_render_options = visual_layer_params->get_deviation_window_render_options();

			deviation_window_render_options.surface_deviation_window_isoline_frequency = frequency;

			visual_layer_params->set_deviation_window_render_options(deviation_window_render_options);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_surface_polygons_mask_check_box_changed()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask surface_polygons_mask =
					visual_layer_params->get_surface_polygons_mask();

			// If surface polygons masking is not supported (by runtime graphics hardware), and the
			// checkbox was checked, then popup a warning message and then disable the checkbox.
			if (!visual_layer_params->is_surface_polygons_mask_supported() &&
				!surface_polygons_mask.enable_surface_polygons_mask &&
				enable_surface_polygons_mask_button->isChecked())
			{
				// Uncheck the checkbox and disable it so it cannot be checked again.
				enable_surface_polygons_mask_button->setChecked(false);
				enable_surface_polygons_mask_button->setDisabled(true);

				QMessageBox::warning(this, tr("Cannot enable surface polygons mask"),
						tr("Graphics driver reports unsupported render-to-texture-array."
#ifndef Q_WS_MAC // Cannot actually update graphics driver explicitly on Mac OS X systems...
							"\nUpgrading your graphics hardware driver may or may not help."
#endif
						),
						QMessageBox::Ok);
			}

			surface_polygons_mask.enable_surface_polygons_mask = enable_surface_polygons_mask_button->isChecked();
			surface_polygons_mask.show_polygon_walls = show_polygon_walls_button->isChecked();
			surface_polygons_mask.treat_polylines_as_polygons = treat_polylines_as_polygons_button->isChecked();
			surface_polygons_mask.only_show_boundary_walls = only_show_boundary_walls_button->isChecked();

			visual_layer_params->set_surface_polygons_mask(surface_polygons_mask);

			// The 'show only boundary walls' checkbox only applies if 'show polygon walls' is checked.
			only_show_boundary_walls_button->setVisible(
					show_polygon_walls_button->isChecked());

			// All surface polygons mask controls only apply if 'enable surface polygons mask' is checked.
			surface_polygons_mask_widget->setVisible(
					enable_surface_polygons_mask_button->isChecked());

			// The isoline frequency spinbox only applies if surface deviation is enabled and
			// either the outer spherical surface or the polygon walls are shown.
			isoline_frequency_widget->setVisible(
					surface_deviation_window_button->isChecked() ||
						(enable_surface_polygons_mask_button->isChecked() &&
							show_polygon_walls_button->isChecked()));
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_depth_restriction_spinbox_changed(
		double depth_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			// Get the QObject that triggered this slot.
			QObject *signal_sender = sender();
			// Return early in case this slot not activated by a signal - shouldn't happen.
			if (!signal_sender)
			{
				return;
			}

			// Cast to a QDoubleSpinBox since only QDoubleSpinBox objects trigger this slot.
			QDoubleSpinBox *depth_restriction_spinbox = qobject_cast<QDoubleSpinBox *>(signal_sender);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					depth_restriction_spinbox,
					GPLATES_ASSERTION_SOURCE);

			GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction depth_restriction =
					visual_layer_params->get_depth_restriction();

			if (depth_restriction_spinbox == min_depth_spin_box)
			{
				// Clamp the restricted min depth if it's above the restricted max depth.
				if (depth_value > max_depth_spin_box->value())
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					min_depth_spin_box->setValue(max_depth_spin_box->value());
					return;
				}
				depth_restriction.min_depth_radius_restriction = depth_value;
			}
			else if (depth_restriction_spinbox == max_depth_spin_box)
			{
				// Clamp the restricted max depth if it's below the restricted min depth.
				if (depth_value < min_depth_spin_box->value())
				{
					// Setting the spinbox value will trigger this slot again so return after setting.
					max_depth_spin_box->setValue(min_depth_spin_box->value());
					return;
				}
				depth_restriction.max_depth_radius_restriction = depth_value;
			}

			visual_layer_params->set_depth_restriction(depth_restriction);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_restore_actual_depth_range_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();

			const std::pair<double, double> depth_min_max = get_depth_min_max(layer);
			const double depth_min = depth_min_max.first;
			const double depth_max = depth_min_max.second;

			// Set the spin boxes and let their slots handle updating 'visual_layer_params'.
			min_depth_spin_box->setValue(depth_min);
			max_depth_spin_box->setValue(depth_max);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_quality_performance_spinbox_changed(
		int value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			// Get the QObject that triggered this slot.
			QObject *signal_sender = sender();
			// Return early in case this slot not activated by a signal - shouldn't happen.
			if (!signal_sender)
			{
				return;
			}

			// Cast to a QSpinBox since only QSpinBox objects trigger this slot.
			QSpinBox *quality_performance_spin_box = qobject_cast<QSpinBox *>(signal_sender);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					quality_performance_spin_box,
					GPLATES_ASSERTION_SOURCE);

			GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance quality_performance =
					visual_layer_params->get_quality_performance();

			if (quality_performance_spin_box == sampling_rate_spin_box)
			{
				quality_performance.sampling_rate = value;
			}
			else if (quality_performance_spin_box == bisection_iterations_spin_box)
			{
				quality_performance.bisection_iterations = value;
			}
			else if (quality_performance_spin_box == improve_performance_during_drag_globe_spin_box)
			{
				quality_performance.reduce_rate_during_drag_globe = value;
			}

			visual_layer_params->set_quality_performance(quality_performance);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_improve_performance_during_globe_rotation_check_box_changed()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance quality_performance =
					params->get_quality_performance();
			quality_performance.enable_reduce_rate_during_drag_globe =
					improve_performance_during_drag_globe_check_box->isChecked();
			params->set_quality_performance(quality_performance);

			// The sample rate reduction widget is only visible if reduce rate check box is ticked.
			sample_rate_reduction_widget->setVisible(
					improve_performance_during_drag_globe_check_box->isChecked());
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_test_variable_spinbox_changed(
		double value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			// Get the QObject that triggered this slot.
			QObject *signal_sender = sender();
			// Return early in case this slot not activated by a signal - shouldn't happen.
			if (!signal_sender)
			{
				return;
			}

			// Cast to a QDoubleSpinBox since only QDoubleSpinBox objects trigger this slot.
			QDoubleSpinBox *test_variable_spinbox = qobject_cast<QDoubleSpinBox *>(signal_sender);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					test_variable_spinbox,
					GPLATES_ASSERTION_SOURCE);

			if (test_variable_spinbox == test_variable_0_spinbox)
			{
				d_shader_test_variables[0] = value;
			}
			else if (test_variable_spinbox == test_variable_1_spinbox)
			{
				d_shader_test_variables[1] = value;
			}
			else if (test_variable_spinbox == test_variable_2_spinbox)
			{
				d_shader_test_variables[2] = value;
			}
			else if (test_variable_spinbox == test_variable_3_spinbox)
			{
				d_shader_test_variables[3] = value;
			}
			else if (test_variable_spinbox == test_variable_4_spinbox)
			{
				d_shader_test_variables[4] = value;
			}
			else if (test_variable_spinbox == test_variable_5_spinbox)
			{
				d_shader_test_variables[5] = value;
			}
			else if (test_variable_spinbox == test_variable_6_spinbox)
			{
				d_shader_test_variables[6] = value;
			}
			else if (test_variable_spinbox == test_variable_7_spinbox)
			{
				d_shader_test_variables[7] = value;
			}
			else if (test_variable_spinbox == test_variable_8_spinbox)
			{
				d_shader_test_variables[8] = value;
			}
			else if (test_variable_spinbox == test_variable_9_spinbox)
			{
				d_shader_test_variables[9] = value;
			}
			else if (test_variable_spinbox == test_variable_10_spinbox)
			{
				d_shader_test_variables[10] = value;
			}
			else if (test_variable_spinbox == test_variable_11_spinbox)
			{
				d_shader_test_variables[11] = value;
			}
			else if (test_variable_spinbox == test_variable_12_spinbox)
			{
				d_shader_test_variables[12] = value;
			}
			else if (test_variable_spinbox == test_variable_13_spinbox)
			{
				d_shader_test_variables[13] = value;
			}
			else if (test_variable_spinbox == test_variable_14_spinbox)
			{
				d_shader_test_variables[14] = value;
			}
			else if (test_variable_spinbox == test_variable_15_spinbox)
			{
				d_shader_test_variables[15] = value;
			}

			visual_layer_params->set_shader_test_variables(d_shader_test_variables);
		}
	}
}


std::pair<double, double>
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::get_scalar_value_min_max(
		GPlatesAppLogic::Layer &layer) const
{
	const GPlatesAppLogic::ScalarField3DLayerParams *layer_params =
		dynamic_cast<const GPlatesAppLogic::ScalarField3DLayerParams *>(
				layer.get_layer_params().get());

	double scalar_field_min = 0; // Default value.
	double scalar_field_max = 1; // Default value
	if (layer_params)
	{
		if (layer_params->get_scalar_min() &&
			layer_params->get_scalar_max())
		{
			scalar_field_min = layer_params->get_scalar_min().get();
			scalar_field_max = layer_params->get_scalar_max().get();
		}
	}

	return std::make_pair(scalar_field_min, scalar_field_max);
}


std::pair<double, double>
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::get_scalar_value_mean_std_dev(
		GPlatesAppLogic::Layer &layer) const
{
	const GPlatesAppLogic::ScalarField3DLayerParams *layer_params =
		dynamic_cast<const GPlatesAppLogic::ScalarField3DLayerParams *>(
				layer.get_layer_params().get());

	double scalar_field_mean = 0; // Default value.
	double scalar_field_std_dev = 0; // Default value
	if (layer_params)
	{
		if (layer_params->get_scalar_mean() &&
			layer_params->get_scalar_standard_deviation())
		{
			scalar_field_mean = layer_params->get_scalar_mean().get();
			scalar_field_std_dev = layer_params->get_scalar_standard_deviation().get();
		}
	}

	return std::make_pair(scalar_field_mean, scalar_field_std_dev);
}


std::pair<double, double>
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::get_gradient_magnitude_min_max(
		GPlatesAppLogic::Layer &layer) const
{
	const GPlatesAppLogic::ScalarField3DLayerParams *layer_params =
		dynamic_cast<const GPlatesAppLogic::ScalarField3DLayerParams *>(
				layer.get_layer_params().get());

	double gradient_magnitude_min = 0; // Default value.
	double gradient_magnitude_max = 1; // Default value
	if (layer_params)
	{
		if (layer_params->get_gradient_magnitude_min() &&
			layer_params->get_gradient_magnitude_max())
		{
			gradient_magnitude_min = layer_params->get_gradient_magnitude_min().get();
			gradient_magnitude_max = layer_params->get_gradient_magnitude_max().get();
		}
	}

	return std::make_pair(gradient_magnitude_min, gradient_magnitude_max);
}


std::pair<double, double>
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::get_gradient_magnitude_mean_std_dev(
		GPlatesAppLogic::Layer &layer) const
{
	const GPlatesAppLogic::ScalarField3DLayerParams *layer_params =
		dynamic_cast<const GPlatesAppLogic::ScalarField3DLayerParams *>(
				layer.get_layer_params().get());

	double gradient_magnitude_mean = 0; // Default value.
	double gradient_magnitude_std_dev = 0; // Default value
	if (layer_params)
	{
		if (layer_params->get_gradient_magnitude_mean() &&
			layer_params->get_gradient_magnitude_standard_deviation())
		{
			gradient_magnitude_mean = layer_params->get_gradient_magnitude_mean().get();
			gradient_magnitude_std_dev = layer_params->get_gradient_magnitude_standard_deviation().get();
		}
	}

	return std::make_pair(gradient_magnitude_mean, gradient_magnitude_std_dev);
}


int
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::get_slider_isovalue(
		const double &iso_value,
		GPlatesAppLogic::Layer &layer,
		QSlider *isovalue_slider) const
{
	const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);
	const double scalar_field_min = scalar_field_min_max.first;
	const double scalar_field_max = scalar_field_min_max.second;

	return int(0.5 /*round to nearest integer*/ +
		// Convert iso-value from range [scalar_field_min, scalar_field_max]
		// to the range of the slider.
		isovalue_slider->minimum() +
			(iso_value - scalar_field_min) / (scalar_field_max - scalar_field_min) *
				(isovalue_slider->maximum() - isovalue_slider->minimum()));
}


std::pair<double, double>
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::get_depth_min_max(
		GPlatesAppLogic::Layer &layer) const
{
	const GPlatesAppLogic::ScalarField3DLayerParams *layer_params =
		dynamic_cast<const GPlatesAppLogic::ScalarField3DLayerParams *>(
				layer.get_layer_params().get());

	double depth_min = 0; // Default value.
	double depth_max = 1; // Default value
	if (layer_params)
	{
		if (layer_params->get_minimum_depth_layer_radius())
		{
			depth_min = layer_params->get_minimum_depth_layer_radius().get();
		}
		if (layer_params->get_maximum_depth_layer_radius())
		{
			depth_max = layer_params->get_maximum_depth_layer_radius().get();
		}
	}

	return std::make_pair(depth_min, depth_max);
}
