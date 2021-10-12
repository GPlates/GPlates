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
 
#ifndef GPLATES_QTWIDGETS_CONFIGUREEXPORTPARAMETERSDIALOG_H
#define GPLATES_QTWIDGETS_CONFIGUREEXPORTPARAMETERSDIALOG_H

#include <QDialog>
#include <QDir>
#include "ConfigureExportParametersDialogUi.h"


#include "gui/ExportAnimationContext.h"


namespace GPlatesQtWidgets
{
	class ConfigureExportParametersDialog: 
			public QDialog,
			protected Ui_ConfigureExportParametersDialog 
	{
		Q_OBJECT
		
	public:
		explicit
		ConfigureExportParametersDialog(
				GPlatesGui::ExportAnimationContext::non_null_ptr_type export_animation_context_ptr,
				QWidget *parent_ = NULL);

		virtual
		~ConfigureExportParametersDialog()
		{  }
	
	
	private slots:

		void
		react_choose_target_directory_clicked();

		void
		react_lineedit_target_directory_edited(
				const QString &text);

		void
		react_export_gmt_checked(
				bool enable);

		void
		react_export_shp_checked(
				bool enable);

		void
		react_export_svg_checked(
				bool enable);

		void
		react_export_velocity_checked(
				bool enable);

	private:
	
		/**
		 * Fills the target directory line edit (might be unnecessary depending on
		 * how this was triggered, but never mind that), checks validity, and updates
		 * the Context if we have a valid directory.
		 */
		void
		update_target_directory(
				const QString &new_target);


		/**
		 * The ExportAnimationContext is the Context role of the Strategy pattern
		 * in Gamma et al p315.
		 *
		 * It keeps all the actual export parameters.
		 */
		GPlatesGui::ExportAnimationContext::non_null_ptr_type d_export_animation_context_ptr;

	};
}

#endif  // GPLATES_QTWIDGETS_CONFIGUREEXPORTPARAMETERSDIALOG_H
