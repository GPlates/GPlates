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
 
#ifndef GPLATES_QTWIDGETS_EXPORTANIMATIONDIALOG_H
#define GPLATES_QTWIDGETS_EXPORTANIMATIONDIALOG_H

#include <QDir>	// for export settings dialog/backend.

#include <QDialog>
#include "ExportAnimationDialogUi.h"

#include "qt-widgets/ConfigureExportParametersDialog.h"
#include "gui/ExportAnimationContext.h"


namespace GPlatesGui
{
	class AnimationController;
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
				GPlatesQtWidgets::ViewportWindow &view_state_,
				QWidget *parent_ = NULL);

		virtual
		~ExportAnimationDialog()
		{  }

		const double &
		view_time() const;

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
		update_status_message(
				QString message);

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
		react_configure_export_parameters_clicked();

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
		 * View State pointer, which needs to be passed to various exporters,
		 * so that they can get access to things like the current anchored plate
		 * ID and the Reconstruction.
		 */
		GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * We have a minature sub-dialog, which is modal, for configuring parameters.
		 * Although this is appears to be a raw pointer, we parent the dialog to this
		 * one, and Qt will handle memory management for it from then onwards.
		 */
		GPlatesQtWidgets::ConfigureExportParametersDialog *d_configure_parameters_dialog_ptr;


		/**
		 * Updates button label & icon.
		 */
		void
		set_export_abort_button_state(
				bool we_are_exporting);

		/**
		 * Recalculates the summary label describing the parameters
		 * that have been chosen for export, to avoid forcing the
		 * user to open up the configuration dialog each time just to
		 * check what is about to be exported.
		 */
		void
		recalculate_parameters_description();

		/**
		 * Recalculates the range of the progress bar to be displayed
		 * BEFORE we export.
		 *
		 * During export, the @a update_progress_bar() method is used
		 * instead.
		 */
		void
		recalculate_progress_bar();

	};
}

#endif
// GPLATES_QTWIDGETS_EXPORTANIMATIONDIALOG


