/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#include <QDir>
#include "ExportAnimationDialog.h"

#include "utils/FloatingPointComparisons.h"
#include "utils/ExportTemplateFilenameSequence.h"
#include "gui/AnimationController.h"



GPlatesQtWidgets::ExportAnimationDialog::ExportAnimationDialog(
		GPlatesGui::AnimationController &animation_controller,
		GPlatesPresentation::ViewState &view_state_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_export_animation_context_ptr(
			new GPlatesGui::ExportAnimationContext(
					*this,
					animation_controller,
					view_state_,
					viewport_window_),
			GPlatesUtils::NullIntrusivePointerHandler()),
	d_animation_controller_ptr(&animation_controller),
	d_view_state_ptr(&viewport_window_),
	d_configure_parameters_dialog_ptr(new GPlatesQtWidgets::ConfigureExportParametersDialog(
			d_export_animation_context_ptr, this))
{
	setupUi(this);

	// Handle my buttons and spinboxes:
	QObject::connect(button_Use_View_Time_start_time, SIGNAL(clicked()),
			this, SLOT(set_start_time_value_to_view_time()));
	QObject::connect(button_Use_View_Time_end_time, SIGNAL(clicked()),
			this, SLOT(set_end_time_value_to_view_time()));

	QObject::connect(widget_start_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_start_time_spinbox_changed(double)));
	QObject::connect(widget_end_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_end_time_spinbox_changed(double)));
	QObject::connect(widget_time_increment, SIGNAL(valueChanged(double)),
			this, SLOT(react_time_increment_spinbox_changed(double)));

	QObject::connect(button_Reverse_the_Animation, SIGNAL(clicked()),
			d_animation_controller_ptr, SLOT(swap_start_and_end_times()));

	QObject::connect(checkbox_finish_exactly_on_end_time, SIGNAL(clicked(bool)),
			d_animation_controller_ptr, SLOT(set_should_finish_exactly_on_end_time(bool)));

	QObject::connect(button_export, SIGNAL(clicked()),
			this, SLOT(react_export_button_clicked()));
	QObject::connect(button_abort, SIGNAL(clicked()),
			this, SLOT(react_abort_button_clicked()));

	QObject::connect(button_configure_export_parameters, SIGNAL(clicked()),
			this, SLOT(react_configure_export_parameters_clicked()));

	// Initialise widgets to state matching AnimationController.
	widget_start_time->setValue(d_animation_controller_ptr->start_time());
	widget_end_time->setValue(d_animation_controller_ptr->end_time());
	widget_time_increment->setValue(d_animation_controller_ptr->time_increment());
	
	// Initialise other widgets to match the current export settings.
	recalculate_progress_bar();
	recalculate_parameters_description();

	// We might actually need the 'exactly on end time' checkbox.
	handle_options_changed();

	// Reset controls to their "Eagerly awaiting user input" state.
	reset();

	// Set up signal/slot connections to respond to AnimationController events.
	QObject::connect(d_animation_controller_ptr, SIGNAL(start_time_changed(double)),
			this, SLOT(handle_start_time_changed(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(end_time_changed(double)),
			this, SLOT(handle_end_time_changed(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(time_increment_changed(double)),
			this, SLOT(handle_time_increment_changed(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(finish_exactly_on_end_time_changed(bool)),
			this, SLOT(handle_options_changed()));
}


const double &
GPlatesQtWidgets::ExportAnimationDialog::view_time() const
{
	return d_animation_controller_ptr->view_time();
}




void
GPlatesQtWidgets::ExportAnimationDialog::reset()
{
	set_export_abort_button_state(false);
	update_status_message(tr("Ready for export."));
	recalculate_progress_bar();
}


void
GPlatesQtWidgets::ExportAnimationDialog::update_progress_bar(
		std::size_t length,
		std::size_t frame)
{
	progress_bar->setRange(0, static_cast<int>(length));
	progress_bar->setValue(static_cast<int>(frame));
	// Demand an immediate repaint to ensure progress bar widget actually
	// gets updated - it lags 1 frame behind otherwise.
	progress_bar->repaint();
	// Process events so the UI remains partly usable while we do all this.
	QCoreApplication::processEvents();
}


void
GPlatesQtWidgets::ExportAnimationDialog::update_status_message(
		QString message)
{
	label_export_status->setText(message);
	// Demand an immediate repaint to ensure status label widget actually
	// gets updated - it doesn't always seem to get updated otherwise. Qt bug?
	label_export_status->repaint();
	// Process events so the UI remains partly usable while we do all this.
	QCoreApplication::processEvents();
}


void
GPlatesQtWidgets::ExportAnimationDialog::set_start_time_value_to_view_time()
{
	d_animation_controller_ptr->set_start_time(view_time());
}

void
GPlatesQtWidgets::ExportAnimationDialog::set_end_time_value_to_view_time()
{
	d_animation_controller_ptr->set_end_time(view_time());
}


void
GPlatesQtWidgets::ExportAnimationDialog::setVisible(
		bool visible)
{
	if ( ! visible) {
		// We are closing. Abort export.
		// FIXME: Should --ideally-- ask export context class to do it,
		//  then react to that 'aborted' event from the context
		if (d_export_animation_context_ptr->is_running()) {
			react_abort_button_clicked();
		}
	}
	QDialog::setVisible(visible);
}



void
GPlatesQtWidgets::ExportAnimationDialog::react_start_time_spinbox_changed(
		double new_val)
{
	d_animation_controller_ptr->set_start_time(new_val);
}


void
GPlatesQtWidgets::ExportAnimationDialog::react_end_time_spinbox_changed(
		double new_val)
{
	d_animation_controller_ptr->set_end_time(new_val);
}


void
GPlatesQtWidgets::ExportAnimationDialog::react_time_increment_spinbox_changed(
		double new_val)
{
	d_animation_controller_ptr->set_time_increment(new_val);
}


void
GPlatesQtWidgets::ExportAnimationDialog::handle_start_time_changed(
		double new_val)
{
	widget_start_time->setValue(new_val);
	recalculate_progress_bar();
}


void
GPlatesQtWidgets::ExportAnimationDialog::handle_end_time_changed(
		double new_val)
{
	widget_end_time->setValue(new_val);
	recalculate_progress_bar();
}


void
GPlatesQtWidgets::ExportAnimationDialog::handle_time_increment_changed(
		double new_val)
{
	widget_time_increment->setValue(new_val);
	recalculate_progress_bar();
}


void
GPlatesQtWidgets::ExportAnimationDialog::handle_options_changed()
{
	checkbox_finish_exactly_on_end_time->setChecked(
			d_animation_controller_ptr->should_finish_exactly_on_end_time());
	recalculate_progress_bar();
}


void
GPlatesQtWidgets::ExportAnimationDialog::react_export_button_clicked()
{
	update_status_message(tr("Exporting..."));
	recalculate_progress_bar();
	set_export_abort_button_state(true);
	
	d_export_animation_context_ptr->do_export();

	set_export_abort_button_state(false);
}

void
GPlatesQtWidgets::ExportAnimationDialog::react_abort_button_clicked()
{
	update_status_message(tr("Aborting..."));
	d_export_animation_context_ptr->abort();
	set_export_abort_button_state(false);
}


void
GPlatesQtWidgets::ExportAnimationDialog::react_configure_export_parameters_clicked()
{
	d_configure_parameters_dialog_ptr->exec();
	recalculate_parameters_description();
}


void
GPlatesQtWidgets::ExportAnimationDialog::set_export_abort_button_state(
		bool we_are_exporting)
{
	// Note: No magic single-purpose button, because people like to double click things,
	// and while that's fine for a non-destructive animation, we don't want that for Export.
	button_export->setDisabled(we_are_exporting);
	button_abort->setEnabled(we_are_exporting);
	
	// We also want to gently encourage users not to mess with parameters while we
	// are in the middle of an export.
	groupbox_range->setDisabled(we_are_exporting);
	groupbox_parameters->setDisabled(we_are_exporting);
}


void
GPlatesQtWidgets::ExportAnimationDialog::recalculate_parameters_description()
{
	label_parameters_target_directory->setText(tr(
			"In the directory \"%1\".")
			.arg(d_export_animation_context_ptr->target_dir().absolutePath()));

	// FIXME: the ExportAnimationContext needs a bit of refactoring inside so
	// that it uses a proper list of parameters for strategies instead of the current
	// kludge of accessors so we can get it ready asap.
	list_parameters_output_files->clear();
	if (d_export_animation_context_ptr->gmt_exporter_enabled()) {
		list_parameters_output_files->addItem(d_export_animation_context_ptr->gmt_exporter_filename_template());
	}
	if (d_export_animation_context_ptr->shp_exporter_enabled()) {
		list_parameters_output_files->addItem(d_export_animation_context_ptr->shp_exporter_filename_template());
	}
	if (d_export_animation_context_ptr->svg_exporter_enabled()) {
		list_parameters_output_files->addItem(d_export_animation_context_ptr->svg_exporter_filename_template());
	}
	if (d_export_animation_context_ptr->velocity_exporter_enabled()) {
		list_parameters_output_files->addItem(d_export_animation_context_ptr->velocity_exporter_filename_template());
	}
	if (d_export_animation_context_ptr->resolved_topology_exporter_enabled()) {
		list_parameters_output_files->addItem(d_export_animation_context_ptr->resolved_topology_exporter_filename_template());
	}
}


void
GPlatesQtWidgets::ExportAnimationDialog::recalculate_progress_bar()
{
	// Ask AnimationController how many frames it thinks we're
	// going to be writing out.
	std::size_t length = d_animation_controller_ptr->duration_in_frames();
	
	// Update labels indicating the true start and end times.
	label_starting_frame_time->setText(tr("%L1 Ma")
			.arg(d_animation_controller_ptr->starting_frame_time(), 0, 'f', 2));
	label_ending_frame_time->setText(tr("%L1 Ma")
			.arg(d_animation_controller_ptr->ending_frame_time(), 0, 'f', 2));
	
	// Update progress bar to show total number of frames that will be written.
	progress_bar->setRange(0, static_cast<int>(length));
	progress_bar->setValue(0);
	// Suggest a repaint soonish to ensure progress bar widget actually
	// gets updated - it doesn't always seem to get updated otherwise. Qt bug?
	progress_bar->update();
}


