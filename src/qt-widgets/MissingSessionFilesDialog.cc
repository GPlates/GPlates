/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <QColor>
#include <QFileInfo>
#include <QPushButton>

#include "MissingSessionFilesDialog.h"

#include "OpenFileDialog.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::MissingSessionFilesDialog::MissingSessionFilesDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_) :
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_view_state(view_state_),
	d_signal_mapper(new QSignalMapper(this))
{
	setupUi(this);

	// Try to adjust column widths.
	QHeaderView *header = missing_files_table_widget->horizontalHeader();
	header->setResizeMode(ColumnNames::FILENAME, QHeaderView::Stretch);
	header->resizeSection(ColumnNames::UPDATE, 30);

	QObject::connect(
			buttonbox->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
			this, SLOT(load()));
	QObject::connect(
			buttonbox->button(QDialogButtonBox::Abort), SIGNAL(clicked()),
			this, SLOT(abort_load()));

    QObject::connect(
			d_signal_mapper, SIGNAL(mapped(int)),
            this, SLOT(update(int)));
}


void
GPlatesQtWidgets::MissingSessionFilesDialog::populate(
		ActionRequested action_requested,
		QStringList missing_file_paths)
{
	d_missing_file_paths = missing_file_paths;
	d_file_path_remapping.clear(); // Clear file path remapping.

	missing_files_table_widget->clearContents(); // Do not clear the header items as well.
	missing_files_table_widget->setRowCount(0);

	// Add a row for each missing file.
	for (int row = 0; row < missing_file_paths.size(); ++row)
	{
		const QString missing_file_path = missing_file_paths[row];

		missing_files_table_widget->insertRow(row);

		// The filename column.
		QTableWidgetItem *file_name_item = new QTableWidgetItem(missing_file_path);
		Qt::ItemFlags file_name_item_flags = file_name_item->flags();
		file_name_item_flags &= ~Qt::ItemIsEditable;
		file_name_item_flags &= ~Qt::ItemIsSelectable;
		file_name_item->setFlags(file_name_item_flags);;
		file_name_item->setData(Qt::BackgroundRole, QColor("#FF6149")); // Colour indicating file does not exist.
		missing_files_table_widget->setItem(row, ColumnNames::FILENAME, file_name_item);

		// The update column.
		QPushButton *update_item = new QPushButton(tr("Locate"));
		// Use the signal mapper so we know which row the clicked button is in.
		QObject::connect(update_item, SIGNAL(clicked()), d_signal_mapper, SLOT(map()));
		d_signal_mapper->setMapping(update_item, row);
		missing_files_table_widget->setCellWidget(row, ColumnNames::UPDATE, update_item);
	}

	missing_files_table_widget->resizeColumnsToContents();

	QPushButton *load_button = buttonbox->button(QDialogButtonBox::Ok);
	QPushButton *abort_button = buttonbox->button(QDialogButtonBox::Abort);

	const QString missing_file_label_string = tr(
			"Some files in the %1 are missing.\n"
			"You have the option to locate them.\n"
			"Any files not located may fail to load.");
	switch (action_requested)
	{
	case LOAD_PROJECT:
		setWindowTitle(tr("Files Missing in Project"));
		missing_files_label->setText(missing_file_label_string.arg(tr("project")));

		load_button->setText(tr("&Load project"));
		load_button->setIcon(QIcon(":/tango_document_open_16.png"));
		load_button->setIconSize(QSize(22, 22));

		abort_button->setText(tr("D&on't load project"));
		abort_button->setIcon(QIcon(":/tango_process_stop_22.png"));
		abort_button->setIconSize(QSize(22, 22));

		break;

	case LOAD_SESSION:
	default:
		setWindowTitle(tr("Files Missing in Session"));
		missing_files_label->setText(missing_file_label_string.arg(tr("session")));

		load_button->setText(tr("&Load session"));
		load_button->setIcon(QIcon(":/tango_document_open_16.png"));
		load_button->setIconSize(QSize(22, 22));

		abort_button->setText(tr("D&on't load session"));
		abort_button->setIcon(QIcon(":/tango_process_stop_22.png"));
		abort_button->setIconSize(QSize(22, 22));

		break;
	}

	abort_button->setDefault(true);
	abort_button->setFocus();
}


boost::optional< QMap<QString/*missing*/, QString/*existing*/> >
GPlatesQtWidgets::MissingSessionFilesDialog::get_file_path_remapping() const
{
	if (d_file_path_remapping.isEmpty())
	{
		return boost::none;
	}

	return d_file_path_remapping;
}


void
GPlatesQtWidgets::MissingSessionFilesDialog::update(
		int row)
{
	if (row < 0 || row >= d_missing_file_paths.size())
	{
	}

	QTableWidgetItem *file_name_item = missing_files_table_widget->item(row, ColumnNames::FILENAME);
	if (file_name_item == NULL)
	{
		return;
	}
	const QString file_name = file_name_item->text();
	const QString file_name_ext = QFileInfo(file_name).completeSuffix();

	// Ask the user to select a file.
	OpenFileDialog open_file_dialog(
			this,
			tr("Open File"),
			// Use the existing filename extension as the filter (in addition to the All files filter)...
			tr("%1 files (*.%2);;All Files (*)").arg(file_name_ext).arg(file_name_ext),
			d_view_state);

	const QString updated_filename = open_file_dialog.get_open_file_name();
	if (updated_filename.isEmpty())
	{
		return;
	}

	file_name_item->setText(updated_filename);
	file_name_item->setData(Qt::BackgroundRole, QColor("white"));

	// Update our file path remapping.
	const QString missing_file_name = d_missing_file_paths[row];
	d_file_path_remapping[missing_file_name] = updated_filename;
}
