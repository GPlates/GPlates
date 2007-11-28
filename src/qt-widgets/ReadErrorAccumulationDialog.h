/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_READERRORACCUMULATIONDIALOG_H
#define GPLATES_GUI_READERRORACCUMULATIONDIALOG_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <QObject>
#include <QString>

#include "ReadErrorAccumulationDialogUi.h"
#include "file-io/ReadErrorAccumulation.h"
#include "InformationDialog.h"

namespace GPlatesQtWidgets
{
	class ReadErrorAccumulationDialog:
			public QDialog, 
			protected Ui_ReadErrorAccumulationDialog
	{
		Q_OBJECT
		
	public:
		explicit
		ReadErrorAccumulationDialog(
				QWidget *parent_ = NULL);
		
		/**
		 * Removes all errors from the tree and resets the top-level items.
		 */
		void
		clear();

		/**
		 * Updates the dialog from d_read_errors, changing label text and populating the tree.
		 */
		void
		update();

		// Accessors

		const GPlatesFileIO::ReadErrorAccumulation &
		read_errors() const
		{
			return d_read_errors;
		}

		GPlatesFileIO::ReadErrorAccumulation &
		read_errors()
		{
			return d_read_errors;
		}
	
	public slots:
	
		void pop_up_help_dialog()
		{
			d_information_dialog.show();
		}
		
		void expandAll()
		{
			tree_widget_errors_by_type->expandAll();
			tree_widget_errors_by_line->expandAll();
		}
		
		void collapseAll()
		{
			tree_widget_errors_by_type->collapseAll();
			d_tree_type_failures_to_begin_ptr->setExpanded(true);
			d_tree_type_terminating_errors_ptr->setExpanded(true);
			d_tree_type_recoverable_errors_ptr->setExpanded(true);
			d_tree_type_warnings_ptr->setExpanded(true);

			tree_widget_errors_by_line->collapseAll();
			d_tree_line_failures_to_begin_ptr->setExpanded(true);
			d_tree_line_terminating_errors_ptr->setExpanded(true);
			d_tree_line_recoverable_errors_ptr->setExpanded(true);
			d_tree_line_warnings_ptr->setExpanded(true);
		}
		
	private:
		/**
		 * Top-level QTreeWidgetItems which will be managed by the QTreeWidget for "By Error"
		 * We need to store a pointer to them in order to add children.
		 */
		QTreeWidgetItem *d_tree_type_failures_to_begin_ptr;
		QTreeWidgetItem *d_tree_type_terminating_errors_ptr;
		QTreeWidgetItem *d_tree_type_recoverable_errors_ptr;
		QTreeWidgetItem *d_tree_type_warnings_ptr;

		/**
		 * Top-level QTreeWidgetItems which will be managed by the QTreeWidget for "By Line"
		 * We need to store a pointer to them in order to add children.
		 */
		QTreeWidgetItem *d_tree_line_failures_to_begin_ptr;
		QTreeWidgetItem *d_tree_line_terminating_errors_ptr;
		QTreeWidgetItem *d_tree_line_recoverable_errors_ptr;
		QTreeWidgetItem *d_tree_line_warnings_ptr;
		
		/**
		 * InformationDialog used to inform the user about different error types.
		 */
		InformationDialog d_information_dialog;
		static const QString s_information_dialog_text;
		static const QString s_information_dialog_title;

		/**
		 * The ReadErrorAccumulation used to store all errors for all files currently loaded by
		 * GPlates. This is populated by passing it as a reference to parsers.
		 */
		GPlatesFileIO::ReadErrorAccumulation d_read_errors;

		/**
		 * Populates one of the Failure to Begin, Terminating Errors, Recoverable Errors or Warnings
		 * tree items, unhiding it as necessary and ordering errors by type.
		 */
		void
		populate_top_level_tree_by_type(
				QTreeWidgetItem *tree_item_ptr,
				QString tree_item_text,
				const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
				const QIcon &occurrence_icon);

		/**
		 * Populates one of the Failure to Begin, Terminating Errors, Recoverable Errors or Warnings
		 * tree items, unhiding it as necessary and ordering errors by line.
		 */
		void
		populate_top_level_tree_by_line(
				QTreeWidgetItem *tree_item_ptr,
				QString tree_item_text,
				const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
				const QIcon &occurrence_icon);
		
		/**
		 * Builds a tree widget item for the file entry and all errors beneath it, by line number.
		 * Assumes that the error collection passed to it is composed of errors for that file only.
		 * Error Occurrences are added in the order they are found in the ReadErrorAccumulation;
		 * this should be in line order.
		 */
		void
		build_file_tree_by_type(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
				const QIcon &occurrence_icon);
				
		/**
		 * Builds a tree widget item for the file entry and all errors beneath it, by error type.
		 * Assumes that the error collection passed to it is composed of errors for that file only.
		 * Error Occurrences are added in the order they are found in each ReadErrorAccumulation.
		 */
		void
		build_file_tree_by_line(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
				const QIcon &occurrence_icon);

		/**
		 * Adds a sequence of Line Number nodes to a parent tree widget item, with Description
		 * and Result sub-items.
		 */
		void
		build_occurrence_line_list(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
				const QIcon &occurrence_icon,
				bool show_short_description);

		/**
		 * Creates a Type Summary item for an error occurrence with short description and quantity.
		 */
		QTreeWidgetItem *
		create_occurrence_type_summary_item(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorOccurrence &error,
				const QIcon &occurrence_icon,
				size_t quantity);

		/**
		 * Creates a File Info item for an error occurrence with base file name and type.
		 */
		QTreeWidgetItem *
		create_occurrence_file_info_item(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorOccurrence &error);

		/**
		 * Creates a File Path item for an error occurrence with full path (as found on command line).
		 */
		QTreeWidgetItem *
		create_occurrence_file_path_item(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorOccurrence &error);

		/**
		 * Creates a Line item for an error occurrence of the form "Line %d [%d; %d] %s".
		 */
		QTreeWidgetItem *
		create_occurrence_line_item(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorOccurrence &error,
				const QIcon &occurrence_icon,
				bool show_short_description);

		/**
		 * Creates a Description item for an error occurrence with code and full text.
		 */
		QTreeWidgetItem *
		create_occurrence_description_item(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorOccurrence &error);

		/**
		 * Creates a Result item for an error occurrence with code and full text.
		 */
		QTreeWidgetItem *
		create_occurrence_result_item(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorOccurrence &error);


		const QString
		build_summary_string();

		/**
		 * Converts a Description enum to a translated QString (short form).
		 */
		static
		const QString &
		get_short_description_as_string(
				GPlatesFileIO::ReadErrors::Description code);

		/**
		 * Converts a Description enum to a translated QString (full text).
		 */
		static
		const QString &
		get_full_description_as_string(
				GPlatesFileIO::ReadErrors::Description code);

		/**
		 * Converts a Result enum to a translated QString.
		 */
		static
		const QString &
		get_result_as_string(
				GPlatesFileIO::ReadErrors::Result code);
	};
}



#endif  // GPLATES_GUI_READERRORACCUMULATIONDIALOG_H


