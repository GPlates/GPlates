/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include "ApplicationState.h"
#include "file-io/FeatureCollectionFileFormat.h"

#include "ManageFeatureCollectionsDialogUi.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
	class ManageFeatureCollectionsActionWidget;
	class ManageFeatureCollectionsStateWidget;
	

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
		 * This clears the table completely, and adds a new row for each loaded file.
		 */
		void
		update();
		
		/**
		 * Updates the ManageFeatureCollectionStateWidgets in the current table,
		 * based on whether the referenced files are currently active or not.
		 *
		 * This does not clear the table or add rows - it is not supposed to handle
		 * addition or removal of files, only changes in file 'active' state (triggered
		 * by clicking on the StateWidgets), as in this situation it is undesirable
		 * to clear and re-build the table.
		 */
		void
		update_state();
		
		
		/**
		 * Initiates editing of the file configuration. 
		 */
		void
		edit_configuration(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Causes the file referenced by the action widget to be saved with its
		 * current name. No user interaction is necessary unless an exception
		 * occurs.
		 */
		void
		save_file(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Causes the file referenced by the action widget to be saved with a
		 * new name. A file save dialog is popped up to request the new name
		 * from the user. The file info and appropriate table row will be updated.
		 * Exceptions will be handled with a small error dialog.
		 */
		void
		save_file_as(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Causes a copy of the file referenced by the action widget to be saved
		 * using different name. A file save dialog is popped up to request the new
		 * name from the user. The file info and appropriate table row will be updated.
		 * Exceptions will be handled with a small error dialog.
		 */
		void
		save_file_copy(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Unloads and re-loads the file referenced by the action widget.
		 */
		void
		reload_file(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Causes the file referenced by the action widget to be unloaded,
		 * and removed from the table. No user interaction is necessary.
		 */
		void
		unload_file(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Causes the file referenced by the state widget to be added or
		 * removed from the active reconstructable files list, effectively
		 * 'showing' or 'hiding' that feature data.
		 */
		void
		set_reconstructable_state_for_file(
				ManageFeatureCollectionsStateWidget *state_widget_ptr,
				bool desired_state);

		/**
		 * Causes the file referenced by the state widget to be added or
		 * removed from the active reconstruction files list, meaning
		 * GPlates will use or ignore any reconstruction tree present.
		 */
		void
		set_reconstruction_state_for_file(
				ManageFeatureCollectionsStateWidget *state_widget_ptr,
				bool desired_state);
	
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
		void
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

		/**
		 * Get format for writing to feature collection file.
		 */
		GPlatesFileIO::FeatureCollectionWriteFormat::Format
			get_feature_collection_write_format(
			const GPlatesFileIO::FileInfo& file_info);

	private:
		
		/**
		 * A reference to the ViewportWindow in order to load and unload files.
		 * Note: using ViewportWindow to handle things is the wrong way to do it, but
		 * is necessary for now.
		 */
		ViewportWindow *d_viewport_window_ptr;

		/**
		 * Holds the path information from the last file opened using the Open File dialog.
		 * Note that file save dialogs infer their directory based on the path of the file
		 * being saved.
		 */
		QString d_open_file_path;
		
		/**
		 * Controls whether Save File dialogs include a Compressed GPML option.
		 */
		bool d_gzip_available;
	};
}

#endif  // GPLATES_GUI_MANAGEFEATURECOLLECTIONSDIALOG_H
