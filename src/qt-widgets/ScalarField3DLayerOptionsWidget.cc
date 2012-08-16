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
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPalette>

#include "ScalarField3DLayerOptionsWidget.h"

#include "ColourScaleWidget.h"
#include "FriendlyLineEdit.h"
#include "QtWidgetUtils.h"
#include "ReadErrorAccumulationDialog.h"
#include "ViewportWindow.h"
#include "VisualLayersDialog.h"

#include "app-logic/ScalarField3DLayerTask.h"

#include "file-io/CptReader.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/CptColourPalette.h"
#include "gui/Dialogs.h"

#include "maths/MathsUtils.h"

#include "presentation/ScalarField3DVisualLayerParams.h"
#include "presentation/VisualLayer.h"


GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::ScalarField3DLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_palette_filename_lineedit(
			new FriendlyLineEdit(
				QString(),
				tr("Default Palette"),
				this)),
	d_open_file_dialog(
			this,
			tr("Open CPT File"),
			tr("Regular CPT file (*.cpt);;All files (*)"),
			view_state),
	d_colour_scale_widget(
			new ColourScaleWidget(
				view_state,
				viewport_window,
				this))
{
	setupUi(this);

	//
	// During runtime the appropriate GUI controls based on the default render mode, colour mode, etc,
	// are enabled/disabled when the various GUI slots get called.
	// But this doesn't happen when the visual layer state is first set up in 'set_data()' because slots
	// don't necessarily get called if the state in the GUI control (eg, checkbox) does not actually change.
	// So to get things started the appropriate widgets are disabled in Qt designer for its render mode state.
	//
	disable_options_for_default_visual_layer_params();

	//
	// Render mode.
	//

	isosurface_render_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isosurface_render_mode_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(handle_render_mode_button(bool)));
	single_deviation_window_render_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			single_deviation_window_render_mode_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(handle_render_mode_button(bool)));
	double_deviation_window_render_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			double_deviation_window_render_mode_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(handle_render_mode_button(bool)));
	cross_sections_render_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			cross_sections_render_mode_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(handle_render_mode_button(bool)));

	//
	// Colour mode.
	//

	depth_colour_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			depth_colour_mode_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(handle_colour_mode_button(bool)));
	isovalue_colour_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isovalue_colour_mode_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(handle_colour_mode_button(bool)));
	gradient_colour_mode_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			gradient_colour_mode_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(handle_colour_mode_button(bool)));

	//
	// Colour palette.
	//

	select_palette_filename_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			select_palette_filename_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_select_palette_filename_button_clicked()));
	use_default_palette_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			use_default_palette_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_default_palette_button_clicked()));

	d_palette_filename_lineedit->setReadOnly(true);
	QtWidgetUtils::add_widget_to_placeholder(
			d_palette_filename_lineedit,
			palette_filename_placeholder_widget);

	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_scale_widget,
			colour_scale_placeholder_widget);
	QPalette colour_scale_palette = d_colour_scale_widget->palette();
	colour_scale_palette.setColor(QPalette::Window, Qt::white);
	d_colour_scale_widget->setPalette(colour_scale_palette);

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
	// Render options.
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
			this, SLOT(handle_isoline_frequency_check_box_changed()));
	isoline_frequency_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			isoline_frequency_spin_box, SIGNAL(valueChanged(int)),
			this, SLOT(handle_isoline_frequency_spinbox_changed(int)));

	//
	// Surface polygons mask.
	//

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
			restore_actual_depth_range_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_restore_actual_depth_range_button_clicked()));

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
			// Setting the values in the spin boxes will emit signals if the value changes
			// which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.

			//
			// Set the render mode.
			//

			switch (visual_layer_params->get_render_mode())
			{
			case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE:
				isosurface_render_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW:
				single_deviation_window_render_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW:
				double_deviation_window_render_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS:
				cross_sections_render_mode_button->setChecked(true);
				break;

			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}

			//
			// Set the colour mode.
			//

			switch (visual_layer_params->get_colour_mode())
			{
			case GPlatesViewOperations::ScalarField3DRenderParameters::COLOUR_MODE_DEPTH:
				depth_colour_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::COLOUR_MODE_ISOVALUE:
				isovalue_colour_mode_button->setChecked(true);
				break;
			case GPlatesViewOperations::ScalarField3DRenderParameters::COLOUR_MODE_GRADIENT:
				gradient_colour_mode_button->setChecked(true);
				break;

			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}

			//
			// Set the colour palette.
			//

			// Load the colour palette into the colour scale widget.
			const bool show_colour_scale = d_colour_scale_widget->populate(
					GPlatesGui::RasterColourPalette::create<double>(visual_layer_params->get_colour_palette()));
			colour_scale_placeholder_widget->setVisible(show_colour_scale);

			// Populate the palette filename.
			d_palette_filename_lineedit->setText(visual_layer_params->get_colour_palette_filename());

			//
			// Set the isovalues.
			//

			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);
			const double scalar_field_min = scalar_field_min_max.first;
			const double scalar_field_max = scalar_field_min_max.second;

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters;
			if (visual_layer_params->get_isovalue_parameters())
			{
				isovalue_parameters = visual_layer_params->get_isovalue_parameters().get();
			}
			else
			{
				// No isovalue yet so just take mid-point between (default) min/max...
				isovalue_parameters.isovalue1 = scalar_field_min + 0.5 * (scalar_field_max - scalar_field_min);
				isovalue_parameters.isovalue2 = scalar_field_min + 0.5 * (scalar_field_max - scalar_field_min);
			}

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
			const double single_step_isovalue = (scalar_field_max - scalar_field_min) / 50.0;
			const double single_step_deviation = (scalar_field_max - scalar_field_min) / 200.0;
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
			// Set the render options.
			//

			const GPlatesViewOperations::ScalarField3DRenderParameters::RenderOptions render_options =
					visual_layer_params->get_render_options();
			QObject::disconnect(
					opacity_deviation_surfaces_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_deviation_surfaces_spinbox_changed(double)));
			opacity_deviation_surfaces_spin_box->setValue(render_options.opacity_deviation_surfaces);
			QObject::connect(
					opacity_deviation_surfaces_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_deviation_surfaces_spinbox_changed(double)));
			volume_render_deviation_window_button->setChecked(render_options.deviation_window_volume_rendering);
			QObject::disconnect(
					opacity_deviation_volume_rendering_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_deviation_volume_rendering_spinbox_changed(double)));
			opacity_deviation_volume_rendering_spin_box->setValue(render_options.opacity_deviation_window_volume_rendering);
			QObject::connect(
					opacity_deviation_volume_rendering_spin_box, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_deviation_volume_rendering_spinbox_changed(double)));
			surface_deviation_window_button->setChecked(render_options.surface_deviation_window);
			QObject::disconnect(
					isoline_frequency_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isoline_frequency_spinbox_changed(int)));
			isoline_frequency_spin_box->setValue(render_options.surface_deviation_window_isoline_frequency);
			QObject::connect(
					isoline_frequency_spin_box, SIGNAL(valueChanged(int)),
					this, SLOT(handle_isoline_frequency_spinbox_changed(int)));

			//
			// Set the surface polygons mask.
			//

			const GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask surface_polygons_mask =
					visual_layer_params->get_surface_polygons_mask();
			show_polygon_walls_button->setChecked(surface_polygons_mask.show_polygon_walls);
			treat_polylines_as_polygons_button->setChecked(surface_polygons_mask.treat_polylines_as_polygons);
			only_show_boundary_walls_button->setChecked(surface_polygons_mask.only_show_boundary_walls);

			//
			// Set the depth restriction.
			//

			const std::pair<double, double> depth_min_max = get_depth_min_max(layer);
			const double depth_min = depth_min_max.first;
			const double depth_max = depth_min_max.second;
			GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction depth_restriction;
			if (visual_layer_params->get_depth_restriction())
			{
				depth_restriction = visual_layer_params->get_depth_restriction().get();
			}
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

			//
			// Set the shader test variables.
			//

			d_shader_test_variables = visual_layer_params->get_shader_test_variables();
			// If not yet set then use default values.
			if (d_shader_test_variables.empty())
			{
				d_shader_test_variables.resize(NUM_SHADER_TEST_VARIABLES, 0.0f);
			}

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
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::disable_options_for_default_visual_layer_params()
{
	GPlatesViewOperations::ScalarField3DRenderParameters default_scalar_field_render_parameters;

	// For the deviation spin boxes we start out hiding either the symmetric deviation spin boxes or
	// the lower/upper deviation spin boxes depending on the default state.
	if (default_scalar_field_render_parameters.get_isovalue_parameters().symmetric_deviation)
	{
		isovalue1_lower_deviation_widget->hide();
		isovalue1_upper_deviation_widget->hide();
		isovalue2_lower_deviation_widget->hide();
		isovalue2_upper_deviation_widget->hide();
	}
	else
	{
		isovalue1_symmetric_deviation_widget->hide();
		isovalue2_symmetric_deviation_widget->hide();
	}

	//
	// Disable colour palette.
	//

	if (default_scalar_field_render_parameters.get_colour_mode() ==
		GPlatesViewOperations::ScalarField3DRenderParameters::COLOUR_MODE_DEPTH)
	{
		colour_palette_group_box->setEnabled(false);
	}

	//
	// Disable isovalue options.
	//

	switch (default_scalar_field_render_parameters.get_render_mode())
	{
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE:
		isovalue1_deviation_group_box->setEnabled(false);
		isovalue2_widget->setEnabled(false);
		isovalue2_deviation_group_box->setEnabled(false);
		symmetric_deviation_button->setEnabled(false);
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW:
		isovalue2_deviation_group_box->setEnabled(false);
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS:
		isovalue_group_box->setEnabled(false);
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW:
	default:
		break;
	}

	//
	// Disable render options.
	//

	switch (default_scalar_field_render_parameters.get_render_mode())
	{
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE:
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS:
		render_options_group_box->setEnabled(false);
		break;

	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW:
	case GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW:
	default:
		break;
	}

	if (!default_scalar_field_render_parameters.get_render_options().deviation_window_volume_rendering)
	{
		opacity_deviation_volume_rendering_widget->setEnabled(false);
	}
	if (!default_scalar_field_render_parameters.get_render_options().surface_deviation_window)
	{
		isoline_frequency_widget->setEnabled(false);
	}

	//
	// Disable surface polygons mask options.
	//

	if (default_scalar_field_render_parameters.get_render_mode() ==
		GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS)
	{
		surface_polygons_mask_group_box->setEnabled(false);
	}

	if (!default_scalar_field_render_parameters.get_surface_polygons_mask().show_polygon_walls)
	{
		only_show_boundary_walls_widget->setEnabled(false);
	}

	//
	// Disable quality/sampling options.
	//

	if (default_scalar_field_render_parameters.get_render_mode() ==
		GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS)
	{
		quality_performance_group_box->setEnabled(false);
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
			if (single_deviation_window_render_mode_button->isChecked())
			{
				visual_layer_params->set_render_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW);
			}
			if (double_deviation_window_render_mode_button->isChecked())
			{
				visual_layer_params->set_render_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW);
			}
			if (cross_sections_render_mode_button->isChecked())
			{
				visual_layer_params->set_render_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS);

				// Handle special-case: cross-sections do not use the depth colour mode.
				if (visual_layer_params->get_colour_mode() ==
					GPlatesViewOperations::ScalarField3DRenderParameters::COLOUR_MODE_DEPTH)
				{
					// Switch to colouring by isovalue (instead of depth).
					isovalue_colour_mode_button->setChecked(true);
				}
			}

			// The 'depth' colour mode does not apply to cross-sections.
			depth_colour_mode_button->setEnabled(
					!cross_sections_render_mode_button->isChecked());

			// Isovalue options don't apply to cross-sections.
			isovalue_group_box->setEnabled(
					!cross_sections_render_mode_button->isChecked());

			// The first isovalue deviation options only apply to single/double deviation window rendering.
			isovalue1_deviation_group_box->setEnabled(
					single_deviation_window_render_mode_button->isChecked() ||
						double_deviation_window_render_mode_button->isChecked());

			// Second isovalue only applies to double deviation window rendering.
			isovalue2_widget->setEnabled(
					double_deviation_window_render_mode_button->isChecked());
			// The second isovalue deviation options only apply to double deviation window rendering.
			isovalue2_deviation_group_box->setEnabled(
					double_deviation_window_render_mode_button->isChecked());

			// Deviation only applies to single/double deviation window rendering.
			symmetric_deviation_button->setEnabled(
					single_deviation_window_render_mode_button->isChecked() ||
						double_deviation_window_render_mode_button->isChecked());

			// The render options only apply to single/double deviation window rendering.
			render_options_group_box->setEnabled(
					single_deviation_window_render_mode_button->isChecked() ||
						double_deviation_window_render_mode_button->isChecked());

			// Surface polygon masks options do not apply to cross-sections.
			surface_polygons_mask_group_box->setEnabled(
					!cross_sections_render_mode_button->isChecked());

			// The quality/performance options do not apply to cross-sections.
			quality_performance_group_box->setEnabled(
					!cross_sections_render_mode_button->isChecked());

			// If the render mode is single or double deviation window then we need to ensure the
			// isovalue deviation window(s) do not overlap or exceed the range [min_scalar,max_scalar].
			// The constraints can get violated, for example, when the isovalue1 has had free range
			// while in 'isosurface' render mode but upon switching to 'single deviation window' mode
			// the 'window' may overlap the minimum or maximum scalar field value.
			if (single_deviation_window_render_mode_button->isChecked() ||
				double_deviation_window_render_mode_button->isChecked())
			{
				GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
				const std::pair<double, double> scalar_field_min_max = get_scalar_value_min_max(layer);
				const double scalar_field_min = scalar_field_min_max.first;
				const double scalar_field_max = scalar_field_min_max.second;

				GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters;
				if (visual_layer_params->get_isovalue_parameters())
				{
					isovalue_parameters = visual_layer_params->get_isovalue_parameters().get();
				}

				if (single_deviation_window_render_mode_button->isChecked())
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
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_colour_mode_button(
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
			if (depth_colour_mode_button->isChecked())
			{
				visual_layer_params->set_colour_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::COLOUR_MODE_DEPTH);
			}
			if (isovalue_colour_mode_button->isChecked())
			{
				visual_layer_params->set_colour_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::COLOUR_MODE_ISOVALUE);
			}
			if (gradient_colour_mode_button->isChecked())
			{
				visual_layer_params->set_colour_mode(
						GPlatesViewOperations::ScalarField3DRenderParameters::COLOUR_MODE_GRADIENT);
			}

			// The colour palette does not apply when depth colour mode is enabled.
			colour_palette_group_box->setEnabled(
					!depth_colour_mode_button->isChecked());
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_select_palette_filename_button_clicked()
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

		ReadErrorAccumulationDialog &read_errors_dialog = d_viewport_window->dialogs().read_error_accumulation_dialog();
		GPlatesFileIO::ReadErrorAccumulation &read_errors = read_errors_dialog.read_errors();
		GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();

		GPlatesFileIO::RegularCptReader regular_cpt_reader;
		GPlatesFileIO::ReadErrorAccumulation regular_errors;
		GPlatesGui::RegularCptColourPalette::maybe_null_ptr_type regular_colour_palette_opt =
			regular_cpt_reader.read_file(palette_file_name, regular_errors);

		// We only accept regular CPT files - we need a continuous range of colours mapped to
		// the input range [0,1] - and categorical CPT files do not support this.
		//
		// There is a slight complication in the detection of whether a
		// CPT file is regular or categorical. For the most part, a line
		// in a categorical CPT file looks nothing like a line in a
		// regular CPT file and will not be successfully parsed; the
		// exception to the rule are the "BFN" lines, the format of
		// which is common to both regular and categorical CPT files.
		// For that reason, we also check if the regular_palette has any
		// ColourSlices.
		//
		// Note: this flow of code is very similar to that in class IntegerCptReader.
		if (regular_colour_palette_opt && regular_colour_palette_opt->size())
		{
			// Add all the errors reported to read_errors.
			read_errors.accumulate(regular_errors);

			// Make sure the value range of the CPT file is [0,1].
			// This is because it gets re-mapped to the appropriate range [scalar_min, scalar_max] or
			// [gradient_magnitude_min, gradient_magnitude_max] in the GPU shader program and hence
			// the CPT file is independent of the particular scalar field.
			if (GPlatesMaths::are_almost_exactly_equal(regular_colour_palette_opt->get_lower_bound(), 0) &&
				GPlatesMaths::are_almost_exactly_equal(regular_colour_palette_opt->get_upper_bound(), 1))
			{
				GPlatesGui::ColourPalette<double>::non_null_ptr_type colour_palette =
					GPlatesGui::convert_colour_palette<
						GPlatesMaths::Real,
						double
					>(regular_colour_palette_opt.get(), GPlatesGui::RealToBuiltInConverter<double>());

				params->set_colour_palette(palette_file_name, colour_palette);

				d_palette_filename_lineedit->setText(QDir::toNativeSeparators(palette_file_name));
			}
			else
			{
				QMessageBox::critical(parentWidget(), "Load CPT file",
						"The regular CPT file must have a z-value range from 0.0 to 1.0.");
			}
		}

		read_errors_dialog.update();
		GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
		if (num_initial_errors != num_final_errors)
		{
			read_errors_dialog.show();
		}

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_use_default_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->use_auto_generated_colour_palette();
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

			const GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode render_mode =
					visual_layer_params->get_render_mode();

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters;
			if (visual_layer_params->get_isovalue_parameters())
			{
				isovalue_parameters = visual_layer_params->get_isovalue_parameters().get();
			}

			QSlider *isovalue_slider = NULL;
			if (isovalue_spinbox == isovalue1_spinbox)
			{
				// Ensure deviation does not violate constraints imposed by the isovalue deviation windows.
				if (render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW ||
					render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW)
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
							(render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW)
							? scalar_field_max - isovalue_parameters.upper_deviation1
							: isovalue_parameters.isovalue2 - isovalue_parameters.lower_deviation2 - isovalue_parameters.upper_deviation1;
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
						render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
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

			const GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode render_mode =
					visual_layer_params->get_render_mode();

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters;
			if (visual_layer_params->get_isovalue_parameters())
			{
				isovalue_parameters = visual_layer_params->get_isovalue_parameters().get();
			}

			QDoubleSpinBox *isovalue_spin_box = NULL;
			if (isovalue_slider == isovalue1_slider)
			{
				// Ensure deviation does not violate constraints imposed by the isovalue deviation windows.
				if (render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW ||
					render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW)
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
							(render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW)
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
						render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
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

			const GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode render_mode =
					visual_layer_params->get_render_mode();

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters;
			if (visual_layer_params->get_isovalue_parameters())
			{
				isovalue_parameters = visual_layer_params->get_isovalue_parameters().get();
			}

			if (deviation_spinbox == isovalue1_lower_deviation_spin_box)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW ||
							render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
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
						render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW ||
							render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
						GPLATES_ASSERTION_SOURCE);
				// Ensure isovalue does not violate constraints imposed by the isovalue deviation windows.
				const double deviation_limit =
						(render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW)
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
						render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
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
						render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
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

			const GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode render_mode =
					visual_layer_params->get_render_mode();

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters;
			if (visual_layer_params->get_isovalue_parameters())
			{
				isovalue_parameters = visual_layer_params->get_isovalue_parameters().get();
			}

			if (symmetric_deviation_spinbox == isovalue1_symmetric_deviation_spin_box)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW ||
							render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
						GPLATES_ASSERTION_SOURCE);
				// Ensure deviation does not violate constraints imposed by the isovalue deviation windows.
				// Same condition for both single and double deviation windows.
				double symmetric_deviation_limit;
				if (render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW)
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
						render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
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
			const GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode render_mode =
					visual_layer_params->get_render_mode();

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_SINGLE_DEVIATION_WINDOW ||
						render_mode == GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
					GPLATES_ASSERTION_SOURCE);

			GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters;
			if (visual_layer_params->get_isovalue_parameters())
			{
				isovalue_parameters = visual_layer_params->get_isovalue_parameters().get();
			}

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
			GPlatesViewOperations::ScalarField3DRenderParameters::RenderOptions render_options =
					visual_layer_params->get_render_options();

			render_options.opacity_deviation_surfaces = opacity;

			visual_layer_params->set_render_options(render_options);
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
			GPlatesViewOperations::ScalarField3DRenderParameters::RenderOptions render_options =
					visual_layer_params->get_render_options();

			render_options.deviation_window_volume_rendering = volume_render_deviation_window_button->isChecked();

			visual_layer_params->set_render_options(render_options);

			// The volume rendering opacity spinbox only applies if volume rendering is enabled.
			opacity_deviation_volume_rendering_widget->setEnabled(
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
			GPlatesViewOperations::ScalarField3DRenderParameters::RenderOptions render_options =
					visual_layer_params->get_render_options();

			render_options.opacity_deviation_window_volume_rendering = opacity;

			visual_layer_params->set_render_options(render_options);
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_isoline_frequency_check_box_changed()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesViewOperations::ScalarField3DRenderParameters::RenderOptions render_options =
					visual_layer_params->get_render_options();

			render_options.surface_deviation_window = surface_deviation_window_button->isChecked();

			visual_layer_params->set_render_options(render_options);

			// The isoline frequency spinbox only applies if surface deviation is enabled.
			isoline_frequency_widget->setEnabled(
					surface_deviation_window_button->isChecked());
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
			GPlatesViewOperations::ScalarField3DRenderParameters::RenderOptions render_options =
					visual_layer_params->get_render_options();

			render_options.surface_deviation_window_isoline_frequency = frequency;

			visual_layer_params->set_render_options(render_options);
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

			surface_polygons_mask.show_polygon_walls = show_polygon_walls_button->isChecked();
			surface_polygons_mask.treat_polylines_as_polygons = treat_polylines_as_polygons_button->isChecked();
			surface_polygons_mask.only_show_boundary_walls = only_show_boundary_walls_button->isChecked();

			visual_layer_params->set_surface_polygons_mask(surface_polygons_mask);

			// The 'show only boundary walls' checkbox only applies if 'show polygon walls' is checked.
			only_show_boundary_walls_button->setEnabled(
					show_polygon_walls_button->isChecked());
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

			GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction depth_restriction;
			if (visual_layer_params->get_depth_restriction())
			{
				depth_restriction = visual_layer_params->get_depth_restriction().get();
			}

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

			visual_layer_params->set_quality_performance(quality_performance);
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
	const GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
		dynamic_cast<const GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
				&layer.get_layer_task_params());

	// Use min/max of scalar field to set the acceptable ranges of iso-values.
	// We might need to use [MEAN - 2*STD_DEV, MEAN + 2*STD_DEV] instead as the range if the
	// field has a small number of scalar values that are way outside the main distribution.
	double scalar_field_min = 0; // Default value.
	double scalar_field_max = 1; // Default value
	if (layer_task_params)
	{
		if (layer_task_params->get_scalar_min())
		{
			scalar_field_min = layer_task_params->get_scalar_min().get();
		}
		if (layer_task_params->get_scalar_max())
		{
			scalar_field_max = layer_task_params->get_scalar_max().get();
		}
	}

	return std::make_pair(scalar_field_min, scalar_field_max);
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
	const GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
		dynamic_cast<const GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
				&layer.get_layer_task_params());

	double depth_min = 0; // Default value.
	double depth_max = 1; // Default value
	if (layer_task_params)
	{
		if (layer_task_params->get_minimum_depth_layer_radius())
		{
			depth_min = layer_task_params->get_minimum_depth_layer_radius().get();
		}
		if (layer_task_params->get_maximum_depth_layer_radius())
		{
			depth_max = layer_task_params->get_maximum_depth_layer_radius().get();
		}
	}

	return std::make_pair(depth_min, depth_max);
}

