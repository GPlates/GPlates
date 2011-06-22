/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_EXPORTANIMATIONDIALOG_H
#define GPLATES_QTWIDGETS_EXPORTANIMATIONDIALOG_H

#include <QDir>	// for export settings dialog/backend.
#include <QDialog>

#include "ExportAnimationDialogUi.h"

#include "ConfigureExportParametersDialog.h"
#include "OpenDirectoryDialog.h"

#include "gui/ExportAnimationContext.h"
#include "gui/ExportAnimationStrategy.h"


namespace GPlatesGui
{
	class AnimationController;
}

namespace GPlatesPresentation
{
	class ViewState;
}


namespace GPlatesQtWidgets
{
	class ViewportWindow;

	class ExportAnimationDialog: 
			public QDialog,
			protected Ui_ExportAnimationDialog 
	{
		Q_OBJECT
		
	public:
		explicit
		ExportAnimationDialog(
				GPlatesGui::AnimationController &animation_controller,
				GPlatesPresentation::ViewState &view_state_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				QWidget *parent_ = NULL);

		virtual
		~ExportAnimationDialog()
		{  }

		const double &
		view_time() const;

		void
		insert_item(
				GPlatesGui::ExportAnimationType::Type export_type,
				GPlatesGui::ExportAnimationType::Format export_format,
				const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr &export_configuration);
	
	public slots:
		/**
		 * Reset controls to their "Eagerly awaiting user input" state.
		 */
		void
		reset();
	
		void
		update_progress_bar(
				std::size_t length,
				std::size_t frame);

		void
		update_single_frame_progress_bar(
				std::size_t val,
				std::size_t range);

		void
		update_status_message(
				QString message,
				bool is_error_msg = false);

		void
		set_start_time_value_to_view_time();

		void
		set_end_time_value_to_view_time();

		/**
		 * We need to reimplement setVisible() because reimplementing closeEvent() is not
		 * enough - the default buttonbox "Close" button only appears to hide the dialog.
		 */
		void
		setVisible(
				bool visible);
		
	signals:
		void
		current_time_changed(
				double new_value);

	private slots:
		void
		react_start_time_spinbox_changed(
				double new_val);

		void
		react_end_time_spinbox_changed(
				double new_val);

		void
		react_time_increment_spinbox_changed(
				double new_val);

		void
		handle_start_time_changed(
				double new_val);

		void
		handle_end_time_changed(
				double new_val);

		void
		handle_time_increment_changed(
				double new_val);

		void
		set_path();
		
		void
		select_single_snapshot(
				bool checked)
		{
			if(checked)
			{
				stackedWidget->setCurrentIndex(1);
				d_is_single_frame=true;
				d_configure_parameters_dialog_ptr->set_single_frame(d_is_single_frame);
				update_target_directory(d_single_path);
				reset();
			}
		}

		void 
		select_range_snapshot(
				bool checked)
		{
			if(checked)
			{
				stackedWidget->setCurrentIndex(0);
				d_is_single_frame=false;
				d_configure_parameters_dialog_ptr->set_single_frame(d_is_single_frame);
				update_target_directory(d_range_path);
				reset();
			}
		}

		void
		set_snapshot_time_to_view_time()
		{
			widget_snapshot_time->setValue(view_time());		
		}

		/**
		 * (Re)sets checkboxes according to animation controller state.
		 */
		void
		handle_options_changed();

		void
		react_export_button_clicked();
		
		void
		react_abort_button_clicked();

		void
		react_add_export_clicked();

		void
		react_choose_target_directory_clicked();

		void
		react_remove_export_clicked();

		void
		handle_export_selection_changed();

	private:
		/**
		 * The ExportAnimationContext is the Context role of the Strategy pattern
		 * in Gamma et al p315.
		 * It handles all the actual export logic for us.
		 */
		GPlatesGui::ExportAnimationContext::non_null_ptr_type d_export_animation_context_ptr;

		/**
		 * This is the animation controller, which holds the state of any
		 * animation set up in the application.
		 * This allows us to control the same animation from
		 * ExportAnimationDialog, AnimateDialog and AnimateControlWidget.
		 */
		GPlatesGui::AnimationController *d_animation_controller_ptr;

		/**
		 * We have a minature sub-dialog, which is modal, for configuring parameters.
		 * Although this is appears to be a raw pointer, we parent the dialog to this
		 * one, and Qt will handle memory management for it from then onwards.
		 */
		GPlatesQtWidgets::ConfigureExportParametersDialog *d_configure_parameters_dialog_ptr;

		OpenDirectoryDialog d_open_directory_dialog;

		/*
		 * flag used to indicate which stack widget is currently using                                                                     
		 */
		bool d_is_single_frame;

		 /*
		 * the output path for single snapshot
		 */
		QString d_single_path;
		
		/*
		 * the output path for a range of snapshots
		 */
		QString d_range_path;

		/**
		 * Updates button label & icon.
		 */
		void
		set_export_abort_button_state(
				bool we_are_exporting);

		/**
		 * Recalculates the range of the progress bar to be displayed
		 * BEFORE we export.
		 *
		 * During export, the @a update_progress_bar() method is used
		 * instead.
		 */
		void
		recalculate_progress_bar();

		bool
		update_target_directory(
				const QString &new_target);

		void
		set_export_parameters();
	};
}

#endif // GPLATES_QTWIDGETS_EXPORTANIMATIONDIALOG
