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

#include <iostream>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <QDebug>
#include <QString>
#include <QMessageBox>
#include <QCoreApplication>

#include "FileIOFeedback.h"

#include "FeatureFocus.h"
#include "UnsavedChangesTracker.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/SessionManagement.h"

#include "file-io/FileInfo.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FeatureCollectionFileFormatClassify.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/ErrorOpeningPipeToGzipException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/GpmlOnePointSixOutputVisitor.h"
#include "file-io/OgrException.h"
#include "file-io/File.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/InvalidFeatureCollectionException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"

#include "qt-widgets/FileDialogFilter.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/ManageFeatureCollectionsDialog.h"


namespace
{
	using GPlatesQtWidgets::FileDialogFilter;

	void
	add_filename_extensions_to_file_dialog_filter(
			FileDialogFilter &filter,
			GPlatesFileIO::FeatureCollectionFileFormat::Format file_format,
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry)
	{
		// Add the filename extensions for the specified file format.
		const std::vector<QString> &filename_extensions =
				file_format_registry.get_all_filename_extensions(file_format);
		BOOST_FOREACH(const QString& filename_extension, filename_extensions)
		{
			filter.add_extension(filename_extension);
		}
	}

	FileDialogFilter
	create_file_dialog_filter(
			GPlatesFileIO::FeatureCollectionFileFormat::Format file_format,
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry)
	{
		FileDialogFilter filter(
				QObject::tr(
						file_format_registry.get_short_description(file_format).toAscii().constData()));

		add_filename_extensions_to_file_dialog_filter(filter, file_format, file_format_registry);

		return filter;
	}

	FileDialogFilter
	create_all_filter()
	{
		return FileDialogFilter(QObject::tr("All files") /* no extensions = matches all */);
	}

	/**
	 * Builds a list of input filters for opening all types of feature collections.
	 */
	GPlatesQtWidgets::OpenFileDialog::filter_list_type
	get_input_filters(
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry)
	{
		GPlatesQtWidgets::OpenFileDialog::filter_list_type filters;

		// We want a list of file formats that can read feature collections.
		std::vector<GPlatesFileIO::FeatureCollectionFileFormat::Format> read_file_formats;

		// Iterate over the registered file formats to find those that can read feature collections.
		const std::vector<GPlatesFileIO::FeatureCollectionFileFormat::Format> file_formats =
				file_format_registry.get_registered_file_formats();
		BOOST_FOREACH(GPlatesFileIO::FeatureCollectionFileFormat::Format file_format, file_formats)
		{
			// If the file format supports reading of feature collections then add it to the list.
			if (file_format_registry.does_file_format_support_reading(file_format))
			{
				read_file_formats.push_back(file_format);
			}
		}

		GPlatesQtWidgets::FileDialogFilter all_loadable_files_filter(QObject::tr("All loadable files"));

		// Iterate over the file formats that can read.
		BOOST_FOREACH(GPlatesFileIO::FeatureCollectionFileFormat::Format read_file_format, read_file_formats)
		{
			// Add a filter for the current file format.
			filters.push_back(create_file_dialog_filter(read_file_format, file_format_registry));

			// Also add the filename extensions of the current file format to the "All loadable files" filter.
			add_filename_extensions_to_file_dialog_filter(
					all_loadable_files_filter, read_file_format, file_format_registry);
		}

		// Add the 'All loadable files' filter to the front so it appears first.
		filters.insert(filters.begin(), all_loadable_files_filter);

		// Also add an 'all files' filter.
		filters.push_back(create_all_filter());

		return filters;
	}

	/**
	 * Builds the specially-formatted list of suitable output filters given a file
	 * to be saved. The result can be fed into the Save As or Save a Copy dialogs.
	 */
	GPlatesQtWidgets::SaveFileDialog::filter_list_type
	get_output_filters_for_file(
			GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref,
			const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry)
	{
		GPlatesQtWidgets::SaveFileDialog::filter_list_type filters;

		// Classify the feature collection so we can determine which file formats support it.
		const GPlatesFileIO::FeatureCollectionFileFormat::classifications_type feature_collection_classification =
				GPlatesFileIO::FeatureCollectionFileFormat::classify(
						file_ref.get_file().get_feature_collection(),
						reconstruct_method_registry);

		// Iterate over the registered file formats.
		const std::vector<GPlatesFileIO::FeatureCollectionFileFormat::Format> file_formats =
				file_format_registry.get_registered_file_formats();
		BOOST_FOREACH(GPlatesFileIO::FeatureCollectionFileFormat::Format file_format, file_formats)
		{
			// The file format must support writing of feature collections.
			if (file_format_registry.does_file_format_support_writing(file_format))
			{
				// If the feature collection to be written contains features that the current
				// file format can handle then add it to the list.
				if (GPlatesFileIO::FeatureCollectionFileFormat::intersect(
					file_format_registry.get_feature_classification(file_format),
					feature_collection_classification))
				{
					filters.push_back(create_file_dialog_filter(file_format, file_format_registry));
				}
			}
		}

		// Also add an 'all files' filter.
		filters.push_back(create_all_filter());

		return filters;
	}


	/**
	 * Here is the logic for determining if a file is considered 'unnamed', i.e. not
	 * yet having a name associated with it, no presence on disk.
	 * Taking a very simple approach for now; maybe in the future we can have a flag
	 * in the FileInfo replace this, so that users can "name" their new FeatureCollections
	 * without necessarily @em saving them yet.
	 */
	bool
	file_is_unnamed(
			GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
	{
		// Get the QFileInfo, to determine unnamed state.
		const QFileInfo &qfileinfo = file.get_file().get_file_info().get_qfileinfo();
		return qfileinfo.fileName().isEmpty();
	}
}



GPlatesGui::FileIOFeedback::FileIOFeedback(
		GPlatesAppLogic::ApplicationState &app_state_,
		GPlatesPresentation::ViewState &view_state_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		FeatureFocus &feature_focus_,
		QObject *parent_):
	QObject(parent_),
	d_app_state_ptr(&app_state_),
	d_viewport_window_ptr(&viewport_window_),
	d_file_state_ptr(&app_state_.get_feature_collection_file_state()),
	d_feature_collection_file_io_ptr(&app_state_.get_feature_collection_file_io()),
	d_feature_focus(feature_focus_),
	d_save_file_as_dialog(
			d_viewport_window_ptr,
			tr("Save File As"),
			GPlatesQtWidgets::SaveFileDialog::filter_list_type(),
			view_state_),
	d_save_file_copy_dialog(
			d_viewport_window_ptr,
			tr("Save a copy of the file with a different name"),
			GPlatesQtWidgets::SaveFileDialog::filter_list_type(),
			view_state_),
	d_open_files_dialog(
			d_viewport_window_ptr,
			tr("Open Files"),
			get_input_filters(app_state_.get_feature_collection_file_format_registry()),
			view_state_)
{
	setObjectName("FileIOFeedback");
}



void
GPlatesGui::FileIOFeedback::open_files()
{
	open_files(d_open_files_dialog.get_open_file_names());
}


void
GPlatesGui::FileIOFeedback::open_files(
		const QStringList &filenames)
{
	if (filenames.isEmpty())
	{
		return;
	}

	try_catch_file_load_with_feedback(
			boost::bind(
					&GPlatesAppLogic::FeatureCollectionFileIO::load_files,
					d_feature_collection_file_io_ptr,
					filenames));

	// FIXME: Dropped during refactoring. Was probably unnecessary.. right? //highlight_unsaved_changes();
}


void
GPlatesGui::FileIOFeedback::open_urls(
		const QList<QUrl> &urls)
{
	if (urls.isEmpty())
	{
		return;
	}

	try_catch_file_load_with_feedback(
			boost::bind(
					&GPlatesAppLogic::FeatureCollectionFileIO::load_urls,
					d_feature_collection_file_io_ptr,
					urls));
}


void
GPlatesGui::FileIOFeedback::open_previous_session(
		int session_slot_to_load)
{
	// If loading a new session would scrap some existing changes, warn the user about it first.
	// This is much the same situation as quitting GPlates without having saved.
	bool load_ok = unsaved_changes_tracker().replace_session_event_hook();
	if (load_ok) {
		GPlatesAppLogic::SessionManagement &sm = app_state().get_session_management();
		
		// Unload all empty-filename feature collections, triggering the removal of their layer info,
		// so that the Session we record as being the user's previous session is self-consistent.
		sm.unload_all_unnamed_files();
		
		// Load the new session.
		try_catch_file_load_with_feedback(
				boost::bind(
						&GPlatesAppLogic::SessionManagement::load_previous_session,
						&sm,
						session_slot_to_load));

	}
}


void
GPlatesGui::FileIOFeedback::reload_file(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	// If the currently focused feature is in the feature collection that is to
	// be reloaded, save the feature id and the property name of the focused
	// geometry, so that the focus can remain on the same conceptual geometry.
	boost::optional<GPlatesModel::FeatureId> focused_feature_id;
	boost::optional<GPlatesModel::PropertyName> focused_property_name;
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			file.get_file().get_feature_collection();
	GPlatesModel::FeatureHandle::weak_ref focused_feature =
			d_feature_focus.focused_feature();
	if (focused_feature.is_valid() &&
			feature_collection.handle_ptr() == focused_feature->parent_ptr())
	{
		GPlatesModel::FeatureHandle::iterator prop_iter = d_feature_focus.associated_geometry_property();
		if (prop_iter.is_still_valid())
		{
			focused_feature_id = focused_feature->feature_id();
			focused_property_name = (*prop_iter)->property_name();
		}
	}

	try_catch_file_load_with_feedback(
			boost::bind(
					&GPlatesAppLogic::FeatureCollectionFileIO::reload_file,
					d_feature_collection_file_io_ptr,
					file));

	if (focused_property_name)
	{
		bool found = false;

		// Go through the feature collection, and find the feature with the
		// saved feature id. Then find the corresponding property inside that.
		BOOST_FOREACH(GPlatesModel::FeatureHandle::non_null_ptr_type feature, *feature_collection)
		{
			if (feature->feature_id() == focused_feature_id)
			{
				for (GPlatesModel::FeatureHandle::iterator iter = feature->begin();
						iter != feature->end(); ++iter)
				{
					if ((*iter)->property_name() == focused_property_name)
					{
						d_feature_focus.set_focus(feature->reference(), iter);
						found = true;
						break;
					}
				}
				break;
			}
		}

		if (!found)
		{
			d_feature_focus.unset_focus();
		}
	}
}


bool
GPlatesGui::FileIOFeedback::save_file_as_appropriate(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	if (file_is_unnamed(file)) {
		return save_file_as(file);
	} else {
		return save_file_in_place(file);
	}
}


bool
GPlatesGui::FileIOFeedback::save_file_in_place(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	// Save the feature collection with GUI feedback.
	return save_file(file);
}


bool
GPlatesGui::FileIOFeedback::save_file_as(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	// Configure and open the Save As dialog.
	d_save_file_as_dialog.set_filters(
			get_output_filters_for_file(
				file,
				d_app_state_ptr->get_reconstruct_method_registry(),
				d_app_state_ptr->get_feature_collection_file_format_registry()));

	QString file_path = file.get_file().get_file_info().get_qfileinfo().filePath();
	d_save_file_as_dialog.select_file(file_path);
	boost::optional<QString> filename_opt = d_save_file_as_dialog.get_file_name();

	if ( ! filename_opt)
	{
		// User cancelled the Save As dialog. This should count as a failure,
		// since if they cancel the dialog during the closing-gplates-save, it should
		// abort the shutdown of GPlates.
		return false;
	}
	
	QString &filename = *filename_opt;

	// Make a new FileInfo object to tell save_file() what the new name should be.
	// This also copies any other info stored in the FileInfo.
	GPlatesFileIO::FileInfo new_fileinfo = GPlatesFileIO::create_copy_with_new_filename(
			filename, file.get_file().get_file_info());

	// Save the feature collection, with GUI feedback.
	bool ok = save_file(
			new_fileinfo,
			file.get_file().get_feature_collection(),
			// New filename means new file format which means we can't use configuration of original file...
			boost::none);
	
	// If there was an error saving, don't change the fileinfo.
	if ( ! ok) {
		return false;
	}

	// Change the file info in the file - this will emit signals to interested observers.
	file.set_file_info(new_fileinfo);
	
	return true;
}


bool
GPlatesGui::FileIOFeedback::save_file_copy(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	// Configure and pop up the Save a Copy dialog.
	d_save_file_copy_dialog.set_filters(
			get_output_filters_for_file(
				file,
				d_app_state_ptr->get_reconstruct_method_registry(),
				d_app_state_ptr->get_feature_collection_file_format_registry()));

	QString file_path = file.get_file().get_file_info().get_qfileinfo().filePath();
	d_save_file_copy_dialog.select_file(file_path);
	boost::optional<QString> filename_opt = d_save_file_copy_dialog.get_file_name();

	if ( ! filename_opt)
	{
		// User cancelled the Save a Copy dialog. This should count as a failure,
		// since if they cancel the dialog during the closing-gplates-save, it should
		// abort the shutdown of GPlates.
		return false;
	}
	
	QString &filename = *filename_opt;

	// Make a new FileInfo object to tell save_file() what the copy name should be.
	// This also copies any other info stored in the FileInfo.
	GPlatesFileIO::FileInfo new_fileinfo = GPlatesFileIO::create_copy_with_new_filename(
			filename, file.get_file().get_file_info());

	// Save the feature collection, with GUI feedback.
	//
	// NOTE: The 'clear_unsaved_changes' flag is set to false because we are not really
	// saving the changes to the original file (only making a copy) whereas the original file
	// is still associated with the unsaved feature collection.
	save_file(
			new_fileinfo,
			file.get_file().get_feature_collection(),
			// New filename means new file format which means we can't use configuration of original file...
			boost::none,
			false/*clear_unsaved_changes*/);

	return true;
}


bool
GPlatesGui::FileIOFeedback::save_file(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	return save_file(
			file.get_file().get_file_info(),
			file.get_file().get_feature_collection(),
			// Use file's configuration when writing it out...
			file.get_file().get_file_configuration(),
			false/*clear_unsaved_changes*/);
}


bool
GPlatesGui::FileIOFeedback::save_file(
		const GPlatesFileIO::FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> file_configuration,
		bool clear_unsaved_changes)
{
	// Pop-up error dialogs need a parent so that they don't just blindly appear in the centre of
	// the screen.
	QWidget *parent_widget = &(viewport_window());

	try
	{
		// Save the feature collection. This is where we finally dip down into the file-io level.
		d_feature_collection_file_io_ptr->save_file(
				file_info,
				feature_collection,
				file_configuration,
				clear_unsaved_changes);
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &e)
	{
		QString message = tr("An error occurred while saving the file '%1'")
				.arg(e.filename());
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}
	catch (GPlatesFileIO::ErrorOpeningPipeToGzipException &e)
	{
		QString message = tr("GPlates was unable to use the '%1' program to save the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to save"
				" files without compression.")
				.arg(e.command())
				.arg(e.filename());
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}
	catch (GPlatesGlobal::InvalidFeatureCollectionException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		// FIXME: Needs an attempt to report the filename.
		QString message = tr("Error: Attempted to write an invalid feature collection.");
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}
	catch (GPlatesGlobal::UnexpectedEmptyFeatureCollectionException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		// FIXME: Report the filename.
		QString message = tr("Error: Attempted to write an empty feature collection.");
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		// FIXME: Report the filename.
		QString message = tr("Error: Writing files in this format is currently not supported.");
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}
	catch (GPlatesFileIO::OgrException &)
	{
		// FIXME: Report the filename.
		QString message = tr("An OGR error occurred.");
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}

	// Since a file has just been saved (successfully), we should let UnsavedChangesTracker know.
	unsaved_changes_tracker().handle_model_has_changed();

	return true;
}


bool
GPlatesGui::FileIOFeedback::save_all(
		bool include_unnamed_files)
{
	viewport_window().status_message("GPlates is saving files...");

	// For each loaded file; if it has unsaved changes, behave as though 'save in place' was clicked.
	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			d_file_state_ptr->get_loaded_files();

	// Return true only if all files saved without issue.
	bool all_ok = true;

	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &loaded_file,
			loaded_files)
	{
		// Attempt to ensure GUI still gets updates... FIXME, it's not enough.
		QCoreApplication::processEvents();

		// Get the FeatureCollectionHandle, to determine unsaved state.
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
				loaded_file.get_file().get_feature_collection();

		// Does this file need saving?
		if (feature_collection_ref.is_valid() && feature_collection_ref->contains_unsaved_changes()) {
			// For now, to avoid pointless 'give me a name for this file (which you can't identify)'
			// situations, only save the files which we have a name for already (unless @a include_unnamed_files)
			if (file_is_unnamed(loaded_file) && ! include_unnamed_files) {
				// Skip the unnamed file.
			} else {
				// Save the feature collection, in place or with dialog, with GUI feedback.
				bool ok = save_file_as_appropriate(loaded_file);
				// save_all() needs to report any failures.
				if ( ! ok) {
					all_ok = false;
				}

			}
		}
	}

	// Some more user feedback in the status message.
	if (all_ok) {
		viewport_window().status_message("Files were saved successfully.", 2000);
	} else {
		viewport_window().status_message("Some files could not be saved.");
	}

	return all_ok;
}


void
GPlatesGui::FileIOFeedback::try_catch_file_load_with_feedback(
		boost::function<void ()> file_load_func)
{
	// Pop-up error dialogs need a parent so that they don't just blindly appear in the centre of
	// the screen.
	QWidget *parent_widget = &(viewport_window());

	// FIXME: Try to ensure the filename is in these error dialogs.
	try
	{
		file_load_func();
	}
	catch (GPlatesFileIO::ErrorOpeningPipeFromGzipException &e)
	{
		QString message = tr("GPlates was unable to use the '%1' program to read the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to open"
				" files which are not compressed.")
				.arg(e.command())
				.arg(e.filename());
		QMessageBox::critical(parent_widget, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &)
	{
		QString message = tr("Error: Loading files in this format is currently not supported.");
		QMessageBox::critical(parent_widget, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
	{
		QString message = tr("Error: GPlates was unable to read the file '%1'.")
				.arg(e.filename());
		QMessageBox::critical(parent_widget, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesGlobal::Exception &e)
	{
		QString message = tr("Error: Unexpected error loading file - ignoring file.");
		QMessageBox::critical(parent_widget, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);

		std::cerr << "Caught exception: " << e << std::endl;
	}
}



GPlatesAppLogic::ApplicationState &
GPlatesGui::FileIOFeedback::app_state()
{
	return *d_app_state_ptr;
}


GPlatesQtWidgets::ManageFeatureCollectionsDialog &
GPlatesGui::FileIOFeedback::manage_feature_collections_dialog()
{
	// Obtain a pointer to the dialog once, via the ViewportWindow and Qt magic.
	static GPlatesQtWidgets::ManageFeatureCollectionsDialog *dialog_ptr = 
			viewport_window().findChild<GPlatesQtWidgets::ManageFeatureCollectionsDialog *>(
					"ManageFeatureCollectionsDialog");
	// The dialog not existing is a serious error.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			dialog_ptr != NULL,
			GPLATES_ASSERTION_SOURCE);
	return *dialog_ptr;
}



GPlatesGui::UnsavedChangesTracker &
GPlatesGui::FileIOFeedback::unsaved_changes_tracker()
{
	// Obtain a pointer to the thing once, via the ViewportWindow and Qt magic.
	static GPlatesGui::UnsavedChangesTracker *tracker_ptr = 
			viewport_window().findChild<GPlatesGui::UnsavedChangesTracker *>(
					"UnsavedChangesTracker");
	// The thing not existing is a serious error.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			tracker_ptr != NULL,
			GPLATES_ASSERTION_SOURCE);
	return *tracker_ptr;
}


