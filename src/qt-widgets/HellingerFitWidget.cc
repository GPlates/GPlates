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

#include <QClipboard>

#include "utils/ComponentManager.h"
#include "HellingerDialog.h"
#include "HellingerModel.h"
#include "HellingerFitWidget.h"

const double INITIAL_SEARCH_RADIUS = 0.2;
const double INITIAL_SIGNIFICANCE_LEVEL = 0.95;
const double INITIAL_ROTATION_ANGLE = 5.;

GPlatesQtWidgets::HellingerFitWidget::HellingerFitWidget(
		GPlatesQtWidgets::HellingerDialog *hellinger_dialog,
		GPlatesQtWidgets::HellingerModel *hellinger_model):
	QWidget(hellinger_dialog),
	d_hellinger_dialog_ptr(hellinger_dialog),
	d_hellinger_model_ptr(hellinger_model),
	d_pole_has_been_calculated(false),
	d_amoeba_residual_ok(false),
	d_last_used_two_way_tolerance(INITIAL_AMOEBA_TWO_WAY_RESIDUAL),
	d_last_used_three_way_tolerance(INITIAL_AMOEBA_THREE_WAY_RESIDUAL),
	d_three_way_fitting_is_enabled(
		GPlatesUtils::ComponentManager::instance().is_enabled(
			GPlatesUtils::ComponentManager::Component::hellinger_three_plate()))
{
	setupUi(this);
	set_up_connections();
	initialise_widgets();
	d_red_palette.setColor(QPalette::Active,QPalette::Base,Qt::red);
}

void
GPlatesQtWidgets::HellingerFitWidget::initialise_widgets()
{
	// Set result boxes to read-only (but enabled). We may want
	// to allow the user to adjust the pole result later.
	// Disabling them is another option, but that greys them
	// out and gives the impression that they don't play a part
	// in the tool.
	spinbox_result_lat_12->setReadOnly(true);
	spinbox_result_lon_12->setReadOnly(true);
	spinbox_result_angle_12->setReadOnly(true);
	spinbox_result_lat_13->setReadOnly(true);
	spinbox_result_lon_13->setReadOnly(true);
	spinbox_result_angle_13->setReadOnly(true);

	spinbox_radius->setValue(INITIAL_SEARCH_RADIUS);
	spinbox_conf_limit->setValue(INITIAL_SIGNIFICANCE_LEVEL);

	spinbox_rho_estimate_12->setValue(INITIAL_ROTATION_ANGLE);
	spinbox_rho_estimate_13->setValue(INITIAL_ROTATION_ANGLE);

	// Set stats and details buttons to false initially.
	button_show_details->setEnabled(false);
	button_calculate_uncertainties->setEnabled(false);

	checkbox_amoeba_iterations->setChecked(false);
	spinbox_amoeba_iterations->setEnabled(false);

	checkbox_amoeba_residual->setChecked(true);
	line_edit_amoeba_tolerance->setText(QString::number(d_hellinger_model_ptr->get_amoeba_tolerance()));

	checkbox_grid_search->setChecked(false);
	spinbox_grid_iterations->setValue(0);

	d_amoeba_residual_ok = true;
	update_buttons();

	enable_three_way_widgets(d_three_way_fitting_is_enabled);

}

void
GPlatesQtWidgets::HellingerFitWidget::update_buttons()
{
	bool two_plate_fit = d_hellinger_model_ptr->get_fit_type() == TWO_PLATE_FIT_TYPE;

	bool estimates_ok = false;
	if (two_plate_fit)
	{
		estimates_ok = (spinbox_rho_estimate_12->value() > 0.0);
	}
	else
	{
		estimates_ok = ((spinbox_rho_estimate_12->value() > 0.0) &&
						(spinbox_rho_estimate_13->value() > 0.0));
	}

	bool amoeba_ok = false;
	if (checkbox_amoeba_residual->isChecked())
	{
		amoeba_ok = d_amoeba_residual_ok;
	}
	else
	{
		amoeba_ok = checkbox_amoeba_iterations->isChecked();
	}

	button_calculate_fit->setEnabled(d_hellinger_model_ptr->picks_are_valid() &&
									spinbox_radius->value() > 0.0 &&
									 estimates_ok &&
									 amoeba_ok &&
									 d_hellinger_dialog_ptr->output_file_path_is_valid());

	button_calculate_uncertainties->setEnabled(d_pole_has_been_calculated &&
											   d_hellinger_dialog_ptr->output_file_path_is_valid());
	button_show_details->setEnabled(d_pole_has_been_calculated);
}

void
GPlatesQtWidgets::HellingerFitWidget::set_up_connections()
{
	QObject::connect(button_calculate_fit,SIGNAL(clicked()),d_hellinger_dialog_ptr,SLOT(handle_calculate_fit()));
	QObject::connect(button_calculate_uncertainties,SIGNAL(clicked()),d_hellinger_dialog_ptr,SLOT(handle_calculate_uncertainties()));
	QObject::connect(button_show_details, SIGNAL(clicked()),d_hellinger_dialog_ptr,SLOT(handle_show_details()));

	QObject::connect(spinbox_radius, SIGNAL(valueChanged(double)), this, SLOT(handle_spinbox_radius_changed()));
	QObject::connect(spinbox_conf_limit, SIGNAL(valueChanged(double)), this, SLOT(handle_spinbox_confidence_changed()));
	QObject::connect(checkbox_grid_search, SIGNAL(clicked()), this, SLOT(handle_checkbox_grid_search_changed()));

	QObject::connect(spinbox_lat_estimate_12, SIGNAL(valueChanged(double)), this, SLOT(handle_pole_estimate_12_lat_lon_changed()));
	QObject::connect(spinbox_lon_estimate_12, SIGNAL(valueChanged(double)), this, SLOT(handle_pole_estimate_12_lat_lon_changed()));
	QObject::connect(spinbox_rho_estimate_12, SIGNAL(valueChanged(double)), this, SLOT(handle_pole_estimate_12_angle_changed()));

	QObject::connect(spinbox_lat_estimate_13, SIGNAL(valueChanged(double)), this, SLOT(handle_pole_estimate_13_lat_lon_changed()));
	QObject::connect(spinbox_lon_estimate_13, SIGNAL(valueChanged(double)), this, SLOT(handle_pole_estimate_13_lat_lon_changed()));
	QObject::connect(spinbox_rho_estimate_13, SIGNAL(valueChanged(double)), this, SLOT(handle_pole_estimate_13_angle_changed()));

	QObject::connect(checkbox_show_result_12, SIGNAL(clicked()), this, SLOT(handle_show_result_checkboxes_clicked()));
	QObject::connect(checkbox_show_result_13, SIGNAL(clicked()), this, SLOT(handle_show_result_checkboxes_clicked()));
	QObject::connect(checkbox_show_result_23, SIGNAL(clicked()), this, SLOT(handle_show_result_checkboxes_clicked()));

	QObject::connect(checkbox_show_estimate_12, SIGNAL(clicked()), this, SLOT(handle_show_estimate_checkboxes_clicked()));
	QObject::connect(checkbox_show_estimate_13, SIGNAL(clicked()), this, SLOT(handle_show_estimate_checkboxes_clicked()));

	QObject::connect(button_clipboard_12,SIGNAL(clicked()),this,SLOT(handle_clipboard_12_clicked()));
	QObject::connect(button_clipboard_13,SIGNAL(clicked()),this,SLOT(handle_clipboard_13_clicked()));
	QObject::connect(button_clipboard_23,SIGNAL(clicked()),this,SLOT(handle_clipboard_23_clicked()));

	QObject::connect(checkbox_amoeba_iterations,SIGNAL(clicked()),this,SLOT(handle_amoeba_iterations_checked()));
	QObject::connect(checkbox_amoeba_residual,SIGNAL(clicked()),this,SLOT(handle_amoeba_residual_checked()));
	QObject::connect(spinbox_amoeba_iterations,SIGNAL(valueChanged(int)),this,SLOT(handle_amoeba_iterations_changed()));
	QObject::connect(line_edit_amoeba_tolerance,SIGNAL(textChanged(QString)),this,SLOT(handle_amoeba_residual_changed()));

}

void
GPlatesQtWidgets::HellingerFitWidget::enable_three_way_widgets(
		bool enable)
{
	spinbox_lat_estimate_13->setEnabled(enable);
	spinbox_lon_estimate_13->setEnabled(enable);
	spinbox_rho_estimate_13->setEnabled(enable);
	checkbox_show_estimate_13->setEnabled(enable);

	spinbox_result_lat_13->setEnabled(enable);
	spinbox_result_lon_13->setEnabled(enable);
	spinbox_result_angle_13->setEnabled(enable);
	checkbox_show_result_13->setEnabled(enable);
	button_clipboard_13->setEnabled(enable);

	spinbox_result_lat_23->setEnabled(enable);
	spinbox_result_lon_23->setEnabled(enable);
	spinbox_result_angle_23->setEnabled(enable);
	checkbox_show_result_23->setEnabled(enable);
	button_clipboard_23->setEnabled(enable);

}

void
GPlatesQtWidgets::HellingerFitWidget::show_three_way_widgets(
		bool show_)
{
	// Pole-1-3 estimates
	label_estimate_13->setVisible(show_);
	label_lat_estimate_13->setVisible(show_);
	label_lon_estimate_13->setVisible(show_);
	label_rho_estimate_13->setVisible(show_);
	spinbox_lat_estimate_13->setVisible(show_);
	spinbox_lon_estimate_13->setVisible(show_);
	spinbox_rho_estimate_13->setVisible(show_);
	checkbox_show_estimate_13->setVisible(show_);

	// Pole-1-3 results
	label_result_13->setVisible(show_);
	label_result_lat_13->setVisible(show_);
	label_result_lon_13->setVisible(show_);
	label_result_angle_13->setVisible(show_);
	spinbox_result_lat_13->setVisible(show_);
	spinbox_result_lon_13->setVisible(show_);
	spinbox_result_angle_13->setVisible(show_);
	checkbox_show_result_13->setVisible(show_);
	button_clipboard_13->setVisible(show_);

	// Pole 2-3 results
	label_result_23->setVisible(show_);
	label_result_lat_23->setVisible(show_);
	label_result_lon_23->setVisible(show_);
	label_result_angle_23->setVisible(show_);
	spinbox_result_lat_23->setVisible(show_);
	spinbox_result_lon_23->setVisible(show_);
	spinbox_result_angle_23->setVisible(show_);
	checkbox_show_result_23->setVisible(show_);
	button_clipboard_23->setVisible(show_);
}



void
GPlatesQtWidgets::HellingerFitWidget::update_fit_widgets_from_model()
{
	spinbox_lat_estimate_12->setValue(d_hellinger_model_ptr->get_hellinger_com_file_struct().d_estimate_12.d_lat);
	spinbox_lon_estimate_12->setValue(d_hellinger_model_ptr->get_hellinger_com_file_struct().d_estimate_12.d_lon);
	spinbox_rho_estimate_12->setValue(d_hellinger_model_ptr->get_hellinger_com_file_struct().d_estimate_12.d_angle);

	spinbox_lat_estimate_13->setValue(d_hellinger_model_ptr->get_hellinger_com_file_struct().d_estimate_13.d_lat);
	spinbox_lon_estimate_13->setValue(d_hellinger_model_ptr->get_hellinger_com_file_struct().d_estimate_13.d_lon);
	spinbox_rho_estimate_13->setValue(d_hellinger_model_ptr->get_hellinger_com_file_struct().d_estimate_13.d_angle);

	checkbox_grid_search->setChecked(d_hellinger_model_ptr->get_hellinger_com_file_struct().d_perform_grid_search);
	spinbox_grid_iterations->setEnabled(checkbox_grid_search->isChecked());
	spinbox_grid_iterations->setValue(d_hellinger_model_ptr->get_hellinger_com_file_struct().d_number_of_grid_iterations);

	bool three_plate_fit = d_three_way_fitting_is_enabled &&
			(d_hellinger_model_ptr->get_fit_type(true) == THREE_PLATE_FIT_TYPE);

	bool enable_12_result_boxes = d_hellinger_model_ptr->get_fit_12();

	bool enable_13_and_23_result_boxes = three_plate_fit &&
			d_hellinger_model_ptr->get_fit_13() &&
			d_hellinger_model_ptr->get_fit_23();

	// The result boxes should only be enabled if we have a valid result for the appropriate plate combination.

	spinbox_result_lat_12->setEnabled(enable_12_result_boxes);
	spinbox_result_lon_12->setEnabled(enable_12_result_boxes);
	spinbox_result_angle_12->setEnabled(enable_12_result_boxes);
	checkbox_show_result_12->setEnabled(enable_12_result_boxes);
	button_clipboard_12->setEnabled(enable_12_result_boxes);

	spinbox_result_lat_13->setEnabled(enable_13_and_23_result_boxes);
	spinbox_result_lon_13->setEnabled(enable_13_and_23_result_boxes);
	spinbox_result_angle_13->setEnabled(enable_13_and_23_result_boxes);
	checkbox_show_result_13->setEnabled(enable_13_and_23_result_boxes);
	button_clipboard_13->setEnabled(enable_13_and_23_result_boxes);

	spinbox_result_lat_23->setEnabled(enable_13_and_23_result_boxes);
	spinbox_result_lon_23->setEnabled(enable_13_and_23_result_boxes);
	spinbox_result_angle_23->setEnabled(enable_13_and_23_result_boxes);
	checkbox_show_result_23->setEnabled(enable_13_and_23_result_boxes);
	button_clipboard_23->setEnabled(enable_13_and_23_result_boxes);

	boost::optional<HellingerFitStructure> fit12 = d_hellinger_model_ptr->get_fit_12();
	boost::optional<HellingerFitStructure> fit13 = d_hellinger_model_ptr->get_fit_13();
	boost::optional<HellingerFitStructure> fit23 = d_hellinger_model_ptr->get_fit_23();

	if (fit12)
	{
		spinbox_result_lat_12->setValue((*fit12).d_lat);
		spinbox_result_lon_12->setValue((*fit12).d_lon);
		spinbox_result_angle_12->setValue((*fit12).d_angle);
	}
	if (fit13)
	{
		spinbox_result_lat_13->setValue((*fit13).d_lat);
		spinbox_result_lon_13->setValue((*fit13).d_lon);
		spinbox_result_angle_13->setValue((*fit13).d_angle);
	}
	if (fit23)
	{
		spinbox_result_lat_23->setValue((*fit23).d_lat);
		spinbox_result_lon_23->setValue((*fit23).d_lon);
		spinbox_result_angle_23->setValue((*fit23).d_angle);
	}

	checkbox_grid_search->setEnabled(!three_plate_fit);

	line_edit_amoeba_tolerance->setText(QString::number(
											(d_hellinger_model_ptr->get_fit_type() == TWO_PLATE_FIT_TYPE) ?
												d_last_used_two_way_tolerance :
												d_last_used_three_way_tolerance));

	update_buttons();
}

void
GPlatesQtWidgets::HellingerFitWidget::update_model_from_fit_widgets()
{
	d_hellinger_model_ptr->set_initial_guess_12(
				spinbox_lat_estimate_12->value(),
				spinbox_lon_estimate_12->value(),
				spinbox_rho_estimate_12->value());

	d_hellinger_model_ptr->set_initial_guess_13(
				spinbox_lat_estimate_13->value(),
				spinbox_lon_estimate_13->value(),
				spinbox_rho_estimate_13->value());

	d_hellinger_model_ptr->set_search_radius(spinbox_radius->value());


	HellingerComFileStructure com_file_struct;

	com_file_struct.d_estimate_12.d_lat = spinbox_lat_estimate_12->value();
	com_file_struct.d_estimate_12.d_lon = spinbox_lon_estimate_12->value();
	com_file_struct.d_estimate_12.d_angle = spinbox_rho_estimate_12->value();

	if (d_hellinger_model_ptr->get_fit_type() == THREE_PLATE_FIT_TYPE)
	{
		com_file_struct.d_estimate_13.d_lat = spinbox_lat_estimate_13->value();
		com_file_struct.d_estimate_13.d_lon = spinbox_lon_estimate_13->value();
		com_file_struct.d_estimate_13.d_angle = spinbox_rho_estimate_13->value();
	}
	com_file_struct.d_search_radius_degrees = spinbox_radius->value();
	com_file_struct.d_perform_grid_search = checkbox_grid_search->isChecked();
	com_file_struct.d_number_of_grid_iterations = spinbox_grid_iterations->value();
	com_file_struct.d_significance_level = spinbox_conf_limit->value();
#if 0
	com_file_struct.d_estimate_kappa = checkbox_kappa->isChecked();
	com_file_struct.d_generate_output_files = checkbox_graphics->isChecked();
#endif
	com_file_struct.d_estimate_kappa = true;
	com_file_struct.d_generate_output_files = true;
	com_file_struct.d_use_amoeba_tolerance = checkbox_amoeba_residual->isChecked();
	com_file_struct.d_use_amoeba_iteration_limit = checkbox_amoeba_iterations->isChecked();
	bool tolerance_ok;
	double tolerance = line_edit_amoeba_tolerance->text().toDouble(&tolerance_ok);
	if (tolerance_ok)
	{
		if (d_hellinger_model_ptr->get_fit_type() == THREE_PLATE_FIT_TYPE)
		{
			com_file_struct.d_amoeba_three_way_tolerance = tolerance;
		}
		else
		{
			com_file_struct.d_amoeba_two_way_tolerance = tolerance;
		}
	}
	com_file_struct.d_number_amoeba_iterations = spinbox_amoeba_iterations->value();

	// Remaining fields in the .com file are not currently configurable from the interface.


	d_hellinger_model_ptr->set_com_file_structure(com_file_struct);
	qDebug() << "tolerance in model: " << d_hellinger_model_ptr->get_amoeba_tolerance();

	if (d_hellinger_model_ptr->get_fit_type() == TWO_PLATE_FIT_TYPE)
	{
		d_last_used_two_way_tolerance = tolerance;
	}
	else
	{
		d_last_used_three_way_tolerance = tolerance;
	}

}

void
GPlatesQtWidgets::HellingerFitWidget::update_after_switching_tabs()
{
	update_fit_widgets_from_model();
}

void
GPlatesQtWidgets::HellingerFitWidget::update_enabled_state_of_estimate_widgets(
		bool enable)
{
	bool three_plate_fit = d_three_way_fitting_is_enabled &&
			(d_hellinger_model_ptr->get_fit_type(true) == THREE_PLATE_FIT_TYPE);

	spinbox_lat_estimate_12->setEnabled(enable);
	spinbox_lon_estimate_12->setEnabled(enable);
	spinbox_rho_estimate_12->setEnabled(enable);
	checkbox_show_estimate_12->setEnabled(!enable);

	spinbox_lat_estimate_13->setEnabled(three_plate_fit && enable);
	spinbox_lon_estimate_13->setEnabled(three_plate_fit && enable);
	spinbox_rho_estimate_13->setEnabled(three_plate_fit && enable);
	checkbox_show_estimate_13->setEnabled(three_plate_fit && !enable);
}

void
GPlatesQtWidgets::HellingerFitWidget::update_after_pole_result()
{
	d_pole_has_been_calculated = true;
	update_fit_widgets_from_model();
	update_buttons();
}

void
GPlatesQtWidgets::HellingerFitWidget::start_progress_bar()
{
	progress_bar->setEnabled(true);
	progress_bar->setMaximum(0.);
}

void
GPlatesQtWidgets::HellingerFitWidget::stop_progress_bar()
{
	progress_bar->setEnabled(false);
	progress_bar->setMaximum(1.);
}

GPlatesQtWidgets::HellingerPoleEstimate
GPlatesQtWidgets::HellingerFitWidget::estimate_12() const
{
	return HellingerPoleEstimate(
				spinbox_lat_estimate_12->value(),
				spinbox_lon_estimate_12->value(),
				spinbox_rho_estimate_12->value());
}

void
GPlatesQtWidgets::HellingerFitWidget::set_estimate_12(
		const GPlatesQtWidgets::HellingerPoleEstimate &estimate)
{
	spinbox_lat_estimate_12->setValue(estimate.d_lat);
	spinbox_lon_estimate_12->setValue(estimate.d_lon);
	spinbox_rho_estimate_12->setValue(estimate.d_angle);
}

void
GPlatesQtWidgets::HellingerFitWidget::set_estimate_13(
		const GPlatesQtWidgets::HellingerPoleEstimate &estimate)
{
	spinbox_lat_estimate_13->setValue(estimate.d_lat);
	spinbox_lon_estimate_13->setValue(estimate.d_lon);
	spinbox_rho_estimate_13->setValue(estimate.d_angle);
}

GPlatesQtWidgets::HellingerPoleEstimate
GPlatesQtWidgets::HellingerFitWidget::estimate_13() const
{
	return HellingerPoleEstimate(
				spinbox_lat_estimate_13->value(),
				spinbox_lon_estimate_13->value(),
				spinbox_rho_estimate_13->value());
}

void
GPlatesQtWidgets::HellingerFitWidget::enable_pole_estimate_widgets(
		bool enable)
{
	spinbox_lat_estimate_12->setEnabled(enable);
	spinbox_lon_estimate_12->setEnabled(enable);
	spinbox_rho_estimate_12->setEnabled(enable);
	spinbox_lat_estimate_13->setEnabled(enable);
	spinbox_lon_estimate_13->setEnabled(enable);
	spinbox_rho_estimate_13->setEnabled(enable);
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_checkbox_grid_search_changed()
{
	spinbox_grid_iterations->setEnabled(checkbox_grid_search->isChecked());
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_spinbox_radius_changed()
{

	if (GPlatesMaths::are_almost_exactly_equal(spinbox_radius->value(),0.))
	{
		spinbox_radius->setPalette(d_red_palette);
	}
	else
	{
		spinbox_radius->setPalette(d_default_palette);
	}
	update_buttons();
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_spinbox_confidence_changed()
{
	if ((GPlatesMaths::are_almost_exactly_equal(spinbox_conf_limit->value(),0.)) ||
			(GPlatesMaths::are_almost_exactly_equal(spinbox_conf_limit->value(),1.)))
	{
		spinbox_conf_limit->setPalette(d_red_palette);
	}
	else
	{
		spinbox_conf_limit->setPalette(d_default_palette);
	}
	update_buttons();
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_pole_estimate_12_angle_changed()
{
	if (GPlatesMaths::are_almost_exactly_equal(spinbox_rho_estimate_12->value(),0.))
	{
		spinbox_rho_estimate_12->setPalette(d_red_palette);
	}
	else{
		spinbox_rho_estimate_12->setPalette(d_default_palette);
		Q_EMIT pole_estimate_12_angle_changed(
					spinbox_rho_estimate_12->value());
	}
	update_buttons();
}


void
GPlatesQtWidgets::HellingerFitWidget::handle_pole_estimate_13_angle_changed()
{
	if (GPlatesMaths::are_almost_exactly_equal(spinbox_rho_estimate_13->value(),0.))
	{
		spinbox_rho_estimate_13->setPalette(d_red_palette);
	}
	else{
		spinbox_rho_estimate_13->setPalette(d_default_palette);
		Q_EMIT pole_estimate_13_angle_changed(
					spinbox_rho_estimate_13->value());
	}
	update_buttons();
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_pole_estimate_12_lat_lon_changed()
{

	Q_EMIT pole_estimate_12_changed(
				spinbox_lat_estimate_12->value(),
				spinbox_lon_estimate_12->value());
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_pole_estimate_13_lat_lon_changed()
{
	Q_EMIT pole_estimate_13_changed(
				spinbox_lat_estimate_13->value(),
				spinbox_lon_estimate_13->value());
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_show_result_checkboxes_clicked()
{
	Q_EMIT show_result_checkboxes_clicked();
}

void GPlatesQtWidgets::HellingerFitWidget::handle_show_estimate_checkboxes_clicked()
{
	Q_EMIT show_estimate_checkboxes_clicked();
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_clipboard_12_clicked()
{
	//TODO: we may want to allow selecting the form of separator used
	// (here tab) in user settings.
	QString text;
	text.append(QString::number(spinbox_result_lat_12->value()));
	text.append("\t");
	text.append(QString::number(spinbox_result_lon_12->value()));
	text.append("\t");
	text.append(QString::number(spinbox_result_angle_12->value()));
	QApplication::clipboard()->setText(text);
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_clipboard_13_clicked()
{
	QString text;
	text.append(QString::number(spinbox_result_lat_13->value()));
	text.append("\t");
	text.append(QString::number(spinbox_result_lon_13->value()));
	text.append("\t");
	text.append(QString::number(spinbox_result_angle_13->value()));
	QApplication::clipboard()->setText(text);
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_clipboard_23_clicked()
{
	QString text;
	text.append(QString::number(spinbox_result_lat_23->value()));
	text.append("\t");
	text.append(QString::number(spinbox_result_lon_23->value()));
	text.append("\t");
	text.append(QString::number(spinbox_result_angle_23->value()));
	QApplication::clipboard()->setText(text);
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_amoeba_iterations_checked()
{
	spinbox_amoeba_iterations->setEnabled(checkbox_amoeba_iterations->isChecked());
	update_buttons();
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_amoeba_residual_checked()
{
	line_edit_amoeba_tolerance->setEnabled(checkbox_amoeba_residual->isChecked());
	update_buttons();
}

void
GPlatesQtWidgets::HellingerFitWidget::handle_amoeba_iterations_changed()
{

}

void
GPlatesQtWidgets::HellingerFitWidget::handle_amoeba_residual_changed()
{
	bool ok;
	double tolerance = line_edit_amoeba_tolerance->text().toFloat(&ok);

	if (!ok)
	{
		line_edit_amoeba_tolerance->setPalette(d_red_palette);
	}
	else
	{
		line_edit_amoeba_tolerance->setPalette(d_default_palette);
	}
	d_amoeba_residual_ok = ok;
	update_buttons();
	qDebug() << "tolerance ok: " << ok;
	qDebug() << "tolerance: " << tolerance;
}


