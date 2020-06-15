/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010, 2011, 2013 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_EDITEXPORTPARAMETERSDIALOG_H
#define GPLATES_QTWIDGETS_EDITEXPORTPARAMETERSDIALOG_H

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QString>
#include <QTableWidget>
#include <QVBoxLayout>

#include "ui_EditExportParametersDialogUi.h"

#include "gui/ExportAnimationContext.h"
#include "gui/ExportAnimationType.h"


namespace GPlatesQtWidgets
{
	class ExportFileNameTemplateWidget;
	class ExportOptionsWidget;

	class EditExportParametersDialog : 
			public QDialog,
			protected Ui_EditExportParametersDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		EditExportParametersDialog(
				GPlatesGui::ExportAnimationContext::non_null_ptr_type export_animation_context_ptr,
				QWidget *parent_ = NULL);

		virtual
		~EditExportParametersDialog()
		{ }

		/**
		 * Initialise the export configuration that the user is going to edit with this dialog.
		 *
		 * Once the user has finished editing we will call ExportAnimationDialog::edit_item() with
		 * the specified export table row.
		 */
		void
		initialise(
				int export_row_in_animation_dialog,
				GPlatesGui::ExportAnimationType::ExportID export_id,
				const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr &export_configuration);

		void
		set_single_frame(
				bool is_single_frame)
		{
			d_is_single_frame = is_single_frame;
		}

	private Q_SLOTS:

		void
		react_edit_item_accepted();

	private:

		/**
		 * The ExportAnimationContext is the Context role of the Strategy pattern
		 * in Gamma et al p315.
		 *
		 * It keeps all the actual export parameters.
		 */
		GPlatesGui::ExportAnimationContext::non_null_ptr_type d_export_animation_context_ptr;

		bool d_is_single_frame;

		/**
		 * Used to set and retrieve the filename template.
		 */
		ExportFileNameTemplateWidget *d_export_file_name_template_widget;

		/**
		 * The export table row, in ExportAnimationDialog, of the export configuration being edited.
		 */
		boost::optional<int> d_export_row_in_animation_dialog;

		/**
		 * The export ID of the export configuration being edited.
		 */
		boost::optional<GPlatesGui::ExportAnimationType::ExportID> d_export_id;

		/**
		 * The widget, if any, used to select export options for the export configuration being edited.
		 *
		 * Some export types do not have export options - these will be boost::none.
		 */
		boost::optional<ExportOptionsWidget *> d_export_options_widget;

		/**
		 * The layout for the export options widget.
		 */
		QVBoxLayout *d_export_options_widget_layout;


		void
		clear_export_options_widget();

		void
		set_export_options_widget(
				const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr &export_configuration);
	};
}

#endif  // GPLATES_QTWIDGETS_EDITEXPORTPARAMETERSDIALOG_H
