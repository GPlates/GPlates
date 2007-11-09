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

#include "ReadErrorAccumulationDialogUi.h"
#include <QObject>
#include <QString>

#include "file-io/ReadErrorAccumulation.h"

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

		/**
		 * Populates one of the Failure to Begin, Terminating Errors, Recoverable Errors or Warnings
		 * tree items, unhiding it as necessary.
		 */
		void
		populate_top_level_tree_widget(
				QTreeWidgetItem *tree_item_ptr,
				QString tree_item_text,
				const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
				const QIcon &occurrence_icon);
		
		/**
		 * Creates a tree widget item for the file entry and all errors beneath it.
		 * Assumes that the error collection passed to it is composed of errors for that file only.
		 */
		void
		create_file_tree_widget(
				QTreeWidgetItem *tree_item_ptr,
				const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
				const QIcon &occurrence_icon);

		/**
		 * Adds tree widget nodes for each error in the given collection to the parent QTreeWidgetItem.
		 */
		void
		create_error_tree(
				QTreeWidgetItem *parent_item_ptr,
				const GPlatesFileIO::ReadErrorOccurrence &error,
				const QIcon &occurrence_icon);


		const QString
		build_summary_string();

		/**
		 * Converts a Description enum to a translated QString.
		 */
		static
		const QString &
		get_description_as_string(
				GPlatesFileIO::ReadErrors::Description code);

		/**
		 * Converts a Result enum to a translated QString.
		 */
		static
		const QString &
		get_result_as_string(
				GPlatesFileIO::ReadErrors::Result code);

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
		
	private:
		// The top-level QTreeWidgetItems will be managed by the QTreeWidget
		QTreeWidgetItem *d_tree_item_failures_to_begin_ptr;
		QTreeWidgetItem *d_tree_item_terminating_errors_ptr;
		QTreeWidgetItem *d_tree_item_recoverable_errors_ptr;
		QTreeWidgetItem *d_tree_item_warnings_ptr;
		
		GPlatesFileIO::ReadErrorAccumulation d_read_errors;
	};
}



#endif  // GPLATES_GUI_READERRORACCUMULATIONDIALOG_H


