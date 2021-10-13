/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009,2010 The University of Sydney, Australia
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
#include <QHeaderView>
#include <QAbstractItemModel>

#include "ExportAnimationDialog.h"

#include "gui/AnimationController.h"
#include "gui/ExportAnimationStrategy.h"


GPlatesQtWidgets::ExportAnimationDialog::ExportAnimationDialog(
		GPlatesGui::AnimationController &animation_controller,
		GPlatesPresentation::ViewState &view_state_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		QWidget *parent_):
	QDialog(
			parent_, 
			Qt::Window),
	d_export_animation_context_ptr(
			new GPlatesGui::ExportAnimationContext(
					*this,
					animation_controller,
					view_state_,
					viewport_window_),
			GPlatesUtils::NullIntrusivePointerHandler()),
	d_animation_controller_ptr(
			&animation_controller),
	d_configure_parameters_dialog_ptr(
			new GPlatesQtWidgets::ConfigureExportParametersDialog(
					d_export_animation_context_ptr, this)),
	d_open_directory_dialog(
			this,
			tr("Select Path"),
			view_state_),
	d_is_single_frame(false)
{
	setupUi(this);
	stackedWidget->setCurrentIndex(0);
	tableWidget_range->setFocus();
	
	lineEdit_range_path->setText(QDir::currentPath());
	d_single_path=QDir::currentPath();
		
	update_target_directory(lineEdit_range_path->text());
	
	update_single_frame_progress_bar(0,tableWidget_single->rowCount());

	for (int i = 0; i != 2; ++i)
	{
		tableWidget_range->horizontalHeader()->setResizeMode(i, QHeaderView::ResizeToContents);
		tableWidget_single->horizontalHeader()->setResizeMode(i, QHeaderView::ResizeToContents);
	}
	tableWidget_range->horizontalHeader()->setStretchLastSection(true);
	tableWidget_single->horizontalHeader()->setStretchLastSection(true);
	tableWidget_range->horizontalHeader()->setHighlightSections(false);
	tableWidget_single->horizontalHeader()->setHighlightSections(false);

	tableWidget_range->verticalHeader()->hide();
	tableWidget_single->verticalHeader()->hide();

	tableWidget_single->setSortingEnabled(true);
	tableWidget_range->setSortingEnabled(true);

#ifndef _DEBUG
	button_single_remove_all->setVisible(false);
	button_single_add_all->setVisible(false);
	button_range_remove_all->setVisible(false);
	button_range_add_all->setVisible(false);
#endif

	// Handle my buttons and spinboxes:
	QObject::connect(button_Use_View_Time_start_time, SIGNAL(clicked()),
			this, SLOT(set_start_time_value_to_view_time()));
	QObject::connect(button_Use_View_Time_end_time, SIGNAL(clicked()),
			this, SLOT(set_end_time_value_to_view_time()));

	QObject::connect(button_single_use_main_win, SIGNAL(clicked()),
			this, SLOT(set_time_to_view_time()));

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

	QObject::connect(button_export_single_frame, SIGNAL(clicked()),
			this, SLOT(react_export_button_clicked()));
	
	QObject::connect(button_abort, SIGNAL(clicked()),
			this, SLOT(react_abort_button_clicked()));

	QObject::connect(button_add, SIGNAL(clicked()),
 			this, SLOT(react_add_export_clicked()));
	
	QObject::connect(button_single_add, SIGNAL(clicked()),
			this, SLOT(react_add_export_clicked()));

	QObject::connect(button_single_remove, SIGNAL(clicked()),
			this, SLOT(react_remove_export_clicked()));

	QObject::connect(button_remove, SIGNAL(clicked()),
			this, SLOT(react_remove_export_clicked()));

	QObject::connect(button_single_remove_all, SIGNAL(clicked()),
			this, SLOT(react_remove_all_clicked()));
	QObject::connect(button_single_add_all, SIGNAL(clicked()),
			this, SLOT(react_add_all_clicked()));
	QObject::connect(button_range_remove_all, SIGNAL(clicked()),
			this, SLOT(react_remove_all_clicked()));
	QObject::connect(button_range_add_all, SIGNAL(clicked()),
			this, SLOT(react_add_all_clicked()));

	QObject::connect(button_choose_path, SIGNAL(clicked()),
			this, SLOT(react_choose_target_directory_clicked()));

	QObject::connect(button_single_choose_path, SIGNAL(clicked()),
			this, SLOT(react_choose_target_directory_clicked()));

	QObject::connect(lineEdit_range_path, SIGNAL(editingFinished()),
			this, SLOT(set_path()));
	
	QObject::connect(lineEdit_single_path, SIGNAL(editingFinished()),
			this, SLOT(set_path()));
	
	QObject::connect(radioButton_single, SIGNAL(toggled(bool)),
			this, SLOT(select_single_snapshot(bool)));

	QObject::connect(radioButton_range, SIGNAL(toggled(bool)),
			this, SLOT(select_range_snapshot(bool)));

	// Initialise widgets to state matching AnimationController.
	widget_start_time->setValue(d_animation_controller_ptr->start_time());
	widget_end_time->setValue(d_animation_controller_ptr->end_time());
	widget_time_increment->setValue(d_animation_controller_ptr->time_increment());
	
	// Initialise other widgets to match the current export settings.
	recalculate_progress_bar();
	
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
	progress_bar_single_frame->setValue(0);
	progress_bar_single_frame->repaint();
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
		QString message,
		bool is_error_msg)
{
	QLabel* export_status = NULL; 
	
	if(!d_is_single_frame)
	{
		export_status = label_export_status;
		
	}
	else
	{
		export_status = label_export_status_single;
	}

	if(is_error_msg)
	{
		QPalette pal = export_status->palette();
		pal.setColor(
				QPalette::WindowText, 
				QColor("red")); 
		export_status->setPalette(pal);
	}
	else
	{
		QPalette pal = export_status->palette();
		pal.setColor(
				QPalette::WindowText, 
				QColor("black")); 
		export_status->setPalette(pal);
	}
	
	export_status->setText(message);
	// Demand an immediate repaint to ensure status label widget actually
	// gets updated - it doesn't always seem to get updated otherwise. Qt bug?
	export_status->repaint();

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
GPlatesQtWidgets::ExportAnimationDialog::set_export_parameters()
{
	QTableWidget * table_widget = NULL;
	QString path;
	if (radioButton_single->isChecked())
	{
		table_widget=tableWidget_single;
		path=lineEdit_single_path->text();
		d_animation_controller_ptr->set_start_time(spinBox_single_time->value());
		d_animation_controller_ptr->set_end_time(spinBox_single_time->value());
		d_animation_controller_ptr->set_time_increment(1);
	}
	else
	{
		table_widget=tableWidget_range;
		path=lineEdit_range_path->text();
	}
	
	for (int i=0; i<table_widget->rowCount();i++)
	{
		const GPlatesGui::ExportAnimationType::Type export_type =
				ConfigureExportParametersDialog::get_export_type(table_widget->item(i,0));
		const GPlatesGui::ExportAnimationType::Format export_format =
				ConfigureExportParametersDialog::get_export_format(table_widget->item(i,1));

		const GPlatesGui::ExportAnimationType::ExportID export_id =
				get_export_id(export_type, export_format);

		const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr &export_configuration =
				ConfigureExportParametersDialog::get_export_configuration(
						table_widget->item(i,2));

		// This shouldn't happen but check just in case.
		if (!export_configuration)
		{
			qWarning() << "Ignoring NULL export_configuration and associated exporter.";
			continue;
		}

		d_export_animation_context_ptr->add_export_animation_strategy(
				export_id, export_configuration);
	}
}

void
GPlatesQtWidgets::ExportAnimationDialog::react_export_button_clicked()
{

	QString path;
	QTableWidget * table_widget = NULL;
	
	if(d_is_single_frame)
	{
		path = lineEdit_single_path->text();
		table_widget = tableWidget_single;
	}
	else
	{
		table_widget = tableWidget_range;
		path = lineEdit_range_path->text();
	}

	if(table_widget->rowCount()==0)
	{
		//no export item, show error message, do nothing
		update_status_message(
				tr("Nothing to export."), true);
		return;
	}
	if(!update_target_directory(path))
	{
		//target directory invalid, do nothing
		return;
	}
	update_status_message(tr("Exporting..."));
	recalculate_progress_bar();
	set_export_abort_button_state(true);
	set_export_parameters();
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
GPlatesQtWidgets::ExportAnimationDialog::react_add_export_clicked()
{
	QTableWidget * table_widget = NULL;
	if(radioButton_single->isChecked())
	{
		table_widget = tableWidget_single;
	}
	else
	{
		table_widget = tableWidget_range;
	}
	update_status_message("Ready to export");
	d_configure_parameters_dialog_ptr->initialise(table_widget);
	d_configure_parameters_dialog_ptr->exec();
}

void
GPlatesQtWidgets::ExportAnimationDialog::react_remove_export_clicked()
{
	if(radioButton_single->isChecked())
	{
		tableWidget_single->removeRow(tableWidget_single->currentRow());
		update_single_frame_progress_bar(0,tableWidget_single->rowCount());
	}
	else
	{
		tableWidget_range->removeRow(tableWidget_range->currentRow());
	}
	
}

void
GPlatesQtWidgets::ExportAnimationDialog::insert_item(
		GPlatesGui::ExportAnimationType::Type export_type, 
		GPlatesGui::ExportAnimationType::Format export_format,
		const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr &export_configuration)
{
	QTableWidget *tableWidget = NULL;
	if (radioButton_single->isChecked())
	{
		tableWidget = tableWidget_single;
	}
	else
	{
		tableWidget = tableWidget_range;
	}

	tableWidget->setSortingEnabled(false);
	tableWidget->insertRow(0);
	 	
 	QTableWidgetItem *type_item =
			new ConfigureExportParametersDialog::ExportTypeWidgetItem<QTableWidgetItem>(export_type);
	tableWidget->setItem(
			0,
			0,
			type_item);
	type_item->setText(get_export_type_name(export_type));
	
	QTableWidgetItem *format_item =
			new ConfigureExportParametersDialog::ExportFormatWidgetItem<QTableWidgetItem>(export_format);
	tableWidget->setItem(
			0,
			1,
			format_item);
	format_item->setText(get_export_format_description(export_format));
	
	QTableWidgetItem *configuration_item =
			new ConfigureExportParametersDialog::ExportConfigurationWidgetItem<QTableWidgetItem>(export_configuration);
	tableWidget->setItem(
			0,
			2,
			configuration_item);
	configuration_item->setText(export_configuration->get_filename_template());


	tableWidget->setSortingEnabled(true);

	if(radioButton_single->isChecked())
	{
		update_single_frame_progress_bar(0,tableWidget->rowCount());
	}
}

void
GPlatesQtWidgets::ExportAnimationDialog::react_choose_target_directory_clicked()
{
	QString current_path;
	if (d_is_single_frame)
	{
		current_path=lineEdit_single_path->text();
	}
	else
	{
		current_path=lineEdit_range_path->text();
	}
	d_open_directory_dialog.select_directory(current_path);

	QString path = d_open_directory_dialog.get_existing_directory();

	if (!path.isEmpty())
	{
		update_target_directory(path);
	}
}

void
GPlatesQtWidgets::ExportAnimationDialog::set_path()
{
#if 0	
	//comment out these code. 
	//we are going to update target directory while export button is clicked.
	
	if(d_is_single_frame)
	{
		update_target_directory(lineEdit_single_path->text());
	}
	else
	{
		update_target_directory(lineEdit_range_path->text());
	}
#endif	
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

	groupBox_time->setDisabled(we_are_exporting);
	groupbox_parameters_single->setDisabled(we_are_exporting);

	radioButton_single->setDisabled(we_are_exporting);
	radioButton_range->setDisabled(we_are_exporting);

	button_export_single_frame->setDisabled(we_are_exporting);

	main_buttonbox->setDisabled(we_are_exporting);
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

void
GPlatesQtWidgets::ExportAnimationDialog::update_single_frame_progress_bar(
		std::size_t val,
		std::size_t range)
{
	if(0==range)
	{
		return;
	}

	progress_bar_single_frame->setRange(0,range);
	progress_bar_single_frame->setValue(val);
	
	progress_bar_single_frame->update();
	progress_bar_single_frame->repaint();
	
	QCoreApplication::processEvents();
}

bool
GPlatesQtWidgets::ExportAnimationDialog::update_target_directory(
		const QString &new_target)
{
	//Check properties of supplied pathname.
	QDir new_target_dir(new_target);
	QFileInfo new_target_fileinfo(new_target);
	
	bool ret = false;
	
	if (new_target_fileinfo.exists() && 
		new_target_fileinfo.isDir() && 
		new_target_fileinfo.isWritable()) 
	{		 		
		d_export_animation_context_ptr->set_target_dir(new_target_dir);
		
		if(d_is_single_frame)
		{
			d_single_path=new_target;
		}
		else
		{
			d_range_path=new_target;
		}
		ret = true;
	}

	//if the directory is invalid, the following code will restore the previous valid value
	//otherwise, set the new value
	if(d_is_single_frame)
	{
		lineEdit_single_path->setText(QDir::toNativeSeparators(d_single_path));
	}
	else
	{
		lineEdit_range_path->setText(QDir::toNativeSeparators(d_range_path));
	}

	if(ret == false)
	{
		update_status_message(
				tr("Invalid target directory. The directory has been reset to previous valid path."),
				true);
	}
	else
	{
		update_status_message(tr("Ready to export"));
	}
	return ret;
	
}

void
GPlatesQtWidgets::ExportAnimationDialog::react_remove_all_clicked()
{
	QTableWidget *tableWidget = NULL;
	if (d_is_single_frame)
	{
		tableWidget = tableWidget_single;
	}
	else
	{
		tableWidget = tableWidget_range;
	}
	
	while (tableWidget->rowCount())
	{
		tableWidget->removeRow(0);
	}

	d_configure_parameters_dialog_ptr->initialise(tableWidget);
}

void
GPlatesQtWidgets::ExportAnimationDialog::react_add_all_clicked()
{
	QTableWidget *tableWidget = NULL;
	if (d_is_single_frame)
	{
		tableWidget = tableWidget_single;
	}
	else
	{
		tableWidget = tableWidget_range;
	}

	// Add any exports that have not been added so far.
	// This will in turn call our 'insert_item' method for each remaining export to be added.
	d_configure_parameters_dialog_ptr->add_all_remaining_exports();
}
