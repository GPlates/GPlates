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
	// Iso-value spinbox/slider.
	//

	iso_value_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			iso_value_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_iso_value_spinbox_changed(double)));

	iso_value_slider->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			iso_value_slider, SIGNAL(valueChanged(int)),
			this, SLOT(handle_iso_value_slider_changed(int)));

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
			// Load the colour palette into the colour scale widget.
			const bool show_colour_scale = d_colour_scale_widget->populate(
					GPlatesGui::RasterColourPalette::create<double>(visual_layer_params->get_colour_palette()));
			colour_scale_placeholder_widget->setVisible(show_colour_scale);

			// Populate the palette filename.
			d_palette_filename_lineedit->setText(visual_layer_params->get_colour_palette_filename());

			// Setting the values in the spin boxes will emit signals if the value changes
			// which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.

			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();

			const std::pair<double, double> scalar_field_min_max = get_scalar_field_min_max(layer);
			const double scalar_field_min = scalar_field_min_max.first;
			const double scalar_field_max = scalar_field_min_max.second;

			const double iso_value =
					visual_layer_params->get_iso_value()
					? visual_layer_params->get_iso_value().get()
					// No iso-value yet so just take mid-point between (default) min/max...
					: scalar_field_min + 0.5 * (scalar_field_max - scalar_field_min);

			const int slider_iso_value = get_slider_iso_value(iso_value, layer);

			QObject::disconnect(
					iso_value_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_iso_value_spinbox_changed(double)));
			QObject::disconnect(
					iso_value_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_iso_value_slider_changed(int)));
			iso_value_spinbox->setMinimum(scalar_field_min);
			iso_value_spinbox->setMaximum(scalar_field_max);
			iso_value_spinbox->setSingleStep((scalar_field_max - scalar_field_min) / 50.0);
			iso_value_spinbox->setValue(iso_value);
			iso_value_slider->setValue(slider_iso_value);
			QObject::connect(
					iso_value_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_iso_value_slider_changed(int)));
			QObject::connect(
					iso_value_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_iso_value_spinbox_changed(double)));

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
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_iso_value_spinbox_changed(
		double iso_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ScalarField3DVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ScalarField3DVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			visual_layer_params->set_iso_value(iso_value);

			// Keep the iso-value slider in-sync...

			// Prevent its slot from being triggered.
			QObject::disconnect(
					iso_value_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_iso_value_slider_changed(int)));

			iso_value_slider->setValue(
					get_slider_iso_value(
							iso_value,
							locked_visual_layer->get_reconstruct_graph_layer()));

			QObject::connect(
					iso_value_slider, SIGNAL(valueChanged(int)),
					this, SLOT(handle_iso_value_slider_changed(int)));
		}
	}
}


void
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::handle_iso_value_slider_changed(
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

			const std::pair<double, double> scalar_field_min_max = get_scalar_field_min_max(layer);
			const double scalar_field_min = scalar_field_min_max.first;
			const double scalar_field_max = scalar_field_min_max.second;

			// Convert slider value to the range [0,1].
			const double slider_ratio = (double(value) - iso_value_slider->minimum()) /
					(iso_value_slider->maximum() - iso_value_slider->minimum());

			const double iso_value = scalar_field_min + slider_ratio * (scalar_field_max - scalar_field_min);

			visual_layer_params->set_iso_value(iso_value);

			// Keep the iso-value spinbox in-sync...

			// Prevent its slot from being triggered.
			QObject::disconnect(
					iso_value_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_iso_value_spinbox_changed(double)));

			iso_value_spinbox->setValue(iso_value);

			QObject::connect(
					iso_value_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_iso_value_spinbox_changed(double)));
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

			visual_layer_params->set_shader_test_variables(d_shader_test_variables);
		}
	}
}


std::pair<double, double>
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::get_scalar_field_min_max(
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
GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::get_slider_iso_value(
		const double &iso_value,
		GPlatesAppLogic::Layer &layer) const
{
	const std::pair<double, double> scalar_field_min_max = get_scalar_field_min_max(layer);
	const double scalar_field_min = scalar_field_min_max.first;
	const double scalar_field_max = scalar_field_min_max.second;

	return int(0.5 /*round to nearest integer*/ +
		// Convert iso-value from range [scalar_field_min, scalar_field_max]
		// to the range of the slider.
		iso_value_slider->minimum() +
			(iso_value - scalar_field_min) / (scalar_field_max - scalar_field_min) *
				(iso_value_slider->maximum() - iso_value_slider->minimum()));
}
