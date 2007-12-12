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
 
#ifndef GPLATES_GUI_MANAGEFEATURECOLLECTIONSDIALOG_H
#define GPLATES_GUI_MANAGEFEATURECOLLECTIONSDIALOG_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <QObject>
#include <QString>

#include "ManageFeatureCollectionsActionWidget.h"

#include "ManageFeatureCollectionsDialogUi.h"

namespace GPlatesQtWidgets
{
	class ViewportWindow;
	

	class ManageFeatureCollectionsDialog:
			public QDialog, 
			protected Ui_ManageFeatureCollectionsDialog
	{
		Q_OBJECT
		
	public:
	
		explicit
		ManageFeatureCollectionsDialog(
				ViewportWindow &viewport_window,
				QWidget *parent_ = NULL);
		
		/**
		 * Updates the contents of the table to match the current ApplicationState.
		 */
		void
		update();
		
		/**
		 * Causes the file referenced by the action widget to be unloaded,
		 * and removed from the table.
		 */
		void
		unload_file(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);
	
	public slots:
	
		void
		open_file();
		
		void
		save_all()
		{  }	// FIXME: This should invoke a call on AppState or ViewportWindow etc.
	
	protected:
	
		/**
		 * Deletes items and completely removes all rows from the table.
		 */
		void
		clear_rows();

		/**
		 * Adds a row to the table, creating a ManageFeatureCollectionsActionWidget to
		 * store the FileInfo and the buttons used to interact with the file.
		 */
		ManageFeatureCollectionsActionWidget *
		add_row(
				GPlatesAppState::ApplicationState::file_info_iterator file_it);
		
		/**
		 * Locates the current row of the table used by the given action widget.
		 * Will return table_feature_collections->rowCount() if not found.
		 */
		int
		find_row(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Removes the row indicated by the given action widget.
		 */
		void
		remove_row(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);
	
	private:
		
		/**
		 * A reference to the ViewportWindow in order to load and unload files.
		 * Note: using ViewportWindow to handle things is the wrong way to do it, but
		 * is necessary for now.
		 */
		ViewportWindow *d_viewport_window_ptr;

	};
}

#endif  // GPLATES_GUI_MANAGEFEATURECOLLECTIONSDIALOG_H
