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
 
#ifndef GPLATES_QTWIDGETS_EXPORTFILENAMETEMPLATEWIDGET_H
#define GPLATES_QTWIDGETS_EXPORTFILENAMETEMPLATEWIDGET_H

#include <QString>
#include <QWidget>

#include "ui_ExportFileNameTemplateWidgetUi.h"

#include "gui/ExportAnimationType.h"


namespace GPlatesQtWidgets
{
	class ExportFileNameTemplateWidget : 
			public QWidget,
			protected Ui_ExportFileNameTemplateWidget
	{
		Q_OBJECT

	public:

		explicit
		ExportFileNameTemplateWidget(
				QWidget *parent_ = NULL);


		/**
		 * Clears the filename template field.
		 */
		void
		clear_file_name_template();


		/**
		 * Sets the filename template field.
		 *
		 * The export format is used to separate the file base name from the extension.
		 */
		void
		set_file_name_template(
				const QString &file_name_template,
				GPlatesGui::ExportAnimationType::Format export_format);


		/**
		 * Returns the filename template (base name and extension).
		 */
		QString
		get_file_name_template() const;

	public Q_SLOTS:

		void
		focus_on_line_edit_filename();

	};
}

#endif  // GPLATES_QTWIDGETS_EXPORTFILENAMETEMPLATEWIDGET_H
