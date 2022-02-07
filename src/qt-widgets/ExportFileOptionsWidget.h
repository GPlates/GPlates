/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_EXPORTFILEOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTFILEOPTIONSWIDGET_H

#include <QWidget>

#include "ExportFileOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportOptionsUtils.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportFileOptionsWidget is used to allow the user to select exporting to a single file
	 * or exporting to multiple files (one output file per input file) or both.
	 *
	 * NOTE: This widget is meant to be placed in an exporter-specific @a ExportOptionsWidget.
	 * It doesn't implement the @a ExportOptionsWidget interface.
	 */
	class ExportFileOptionsWidget :
			public QWidget,
			protected Ui_ExportFileOptionsWidget
	{
		Q_OBJECT

	public:

		/**
		 * Creates a @a ExportFileOptionsWidget using default options.
		 */
		static
		ExportFileOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportOptionsUtils::ExportFileOptions &default_export_file_options)
		{
			return new ExportFileOptionsWidget(parent, default_export_file_options);
		}


		/**
		 * Returns the options that have (possibly) been edited by the user via the GUI.
		 */
		const GPlatesGui::ExportOptionsUtils::ExportFileOptions &
		get_export_file_options() const
		{
			return d_export_file_options;
		}

	private Q_SLOTS:

		void
		react_check_box_state_changed(
				int state)
		{
			d_export_file_options.export_to_a_single_file =
					checkBox_export_to_single_file->isChecked();

			d_export_file_options.export_to_multiple_files =
					checkBox_export_to_multiple_files->isChecked();

			d_export_file_options.separate_output_directory_per_file =
					checkBox_separate_output_directory_per_file->isChecked();

			// Display "Separate output directory per input file/layer" checkbox only if outputting multiple files.
			checkBox_separate_output_directory_per_file->setVisible(checkBox_export_to_multiple_files->isChecked());
		}

	private:

		explicit
		ExportFileOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportOptionsUtils::ExportFileOptions &export_file_options_) :
			QWidget(parent_),
			d_export_file_options(export_file_options_)
		{
			setupUi(this);

			//
			// Set the state of the export options widget according to the default export configuration
			// passed to us.
			//

			checkBox_export_to_single_file->setCheckState(
					d_export_file_options.export_to_a_single_file
					? Qt::Checked
					: Qt::Unchecked);
			checkBox_export_to_multiple_files->setCheckState(
					d_export_file_options.export_to_multiple_files
					? Qt::Checked
					: Qt::Unchecked);
			checkBox_separate_output_directory_per_file->setCheckState(
					d_export_file_options.separate_output_directory_per_file
					? Qt::Checked
					: Qt::Unchecked);
			// Display "Separate output directory per input file/layer" checkbox only if outputting multiple files.
			checkBox_separate_output_directory_per_file->setVisible(d_export_file_options.export_to_multiple_files);

			make_signal_slot_connections();
		}


		void
		make_signal_slot_connections()
		{
			QObject::connect(
					checkBox_export_to_single_file,
					SIGNAL(stateChanged(int)),
					this,
					SLOT(react_check_box_state_changed(int)));
			QObject::connect(
					checkBox_export_to_multiple_files,
					SIGNAL(stateChanged(int)),
					this,
					SLOT(react_check_box_state_changed(int)));
			QObject::connect(
					checkBox_separate_output_directory_per_file,
					SIGNAL(stateChanged(int)),
					this,
					SLOT(react_check_box_state_changed(int)));
		}

		GPlatesGui::ExportOptionsUtils::ExportFileOptions d_export_file_options;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTFILEOPTIONSWIDGET_H
