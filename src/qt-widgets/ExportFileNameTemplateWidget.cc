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

#include <QTableWidget>

#include "ExportFileNameTemplateWidget.h"


namespace GPlatesQtWidgets
{
	namespace
	{
		void
		set_fixed_size_for_item_view(
				QAbstractItemView *view)
		{
			int num_rows = view->model()->rowCount();
			if (num_rows > 0)
			{
				view->setFixedHeight(view->sizeHintForRow(0) * (num_rows + 1));
			}
		}
	}
}


GPlatesQtWidgets::ExportFileNameTemplateWidget::ExportFileNameTemplateWidget(
		QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);

	set_fixed_size_for_item_view(treeWidget_template);
	treeWidget_template->setHeaderHidden(true);
	treeWidget_template->header()->setResizeMode(0, QHeaderView::ResizeToContents);

	clear_file_name_template();
}


void
GPlatesQtWidgets::ExportFileNameTemplateWidget::clear_file_name_template()
{
	lineEdit_filename->clear();
	label_file_extension->clear();
}


void
GPlatesQtWidgets::ExportFileNameTemplateWidget::set_file_name_template(
		const QString &file_name_template,
		GPlatesGui::ExportAnimationType::Format export_format)
{
	// Separate the filename template into base name and extension.
	const QString export_format_filename_extension = "." +
			GPlatesGui::ExportAnimationType::get_export_format_filename_extension(export_format);

	// Set the filename template base name and extension.
	if (file_name_template.endsWith(export_format_filename_extension))
	{
		lineEdit_filename->setText(
				file_name_template.left(file_name_template.size() - export_format_filename_extension.size()));
		label_file_extension->setText(
				file_name_template.right(export_format_filename_extension.size()));
	}
	else
	{
		// File format has no filename extension.
		lineEdit_filename->setText(file_name_template);
		label_file_extension->setText("");
	}
}


QString
GPlatesQtWidgets::ExportFileNameTemplateWidget::get_file_name_template() const
{
	// Recombine the filename template from the base name and the extension.
	return lineEdit_filename->text() + label_file_extension->text();;
}


void
GPlatesQtWidgets::ExportFileNameTemplateWidget::focus_on_line_edit_filename()
{
	lineEdit_filename->setFocus();
}
