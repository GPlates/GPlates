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
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QString>
#include <QTextStream>

#include "FileIOFeedback.h"

#include "Dialogs.h"
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
#include "file-io/ErrorWritingFeatureCollectionToFileFormatException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/FileLoadAbortedException.h"
#include "file-io/GpmlOutputVisitor.h"
#include "file-io/OgrException.h"
#include "file-io/File.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/InvalidFeatureCollectionException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"

#include "model/Gpgim.h"
#include "model/GpgimVersion.h"

#include "qt-widgets/GpgimVersionWarningDialog.h"
#include "qt-widgets/FileDialogFilter.h"
#include "qt-widgets/ManageFeatureCollectionsDialog.h"
#include "qt-widgets/ViewportWindow.h"

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


	/**
	 * Returns the list of filenames for files with a different GPGIM version than the current
	 * GPGIM version (built into this GPlates).
	 *
	 * If @a only_unsaved_changes is true then only files with unsaved changes will be checked.
	 *
	 * Returns true if there are any files in the list(s).
	 */
	bool
	get_files_with_different_gpgim_version(
			const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &files,
			bool only_unsaved_changes,
			QStringList &older_version_filenames,
			QStringList &newer_version_filenames)
	{
		const GPlatesModel::GpgimVersion &current_gpgim_version = GPlatesModel::Gpgim::instance().get_version();

		// Iterate over the files.
		BOOST_FOREACH(const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file, files)
		{
			const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
					file.get_file().get_feature_collection();

			// If we are only getting files with unsaved changes then skip those that have no changes.
			if (only_unsaved_changes && !feature_collection_ref->contains_unsaved_changes())
			{
				continue;
			}

			// Look for the GPGIM version tag in the feature collection.
			GPlatesModel::FeatureCollectionHandle::tags_type::const_iterator tag_iter =
					feature_collection_ref->tags().find(GPlatesModel::GpgimVersion::FEATURE_COLLECTION_TAG);

			// If the feature collection does not contain this tag then it is assumed to be current
			// GPGIM version since new (empty) feature collections created by this instance of GPlates
			// will have features added according to the GPGIM version built into this instance of GPlates.
			if (tag_iter == feature_collection_ref->tags().end())
			{
				continue;
			}

			// Get the GPGIM version of the current file.
			const boost::any &tag = tag_iter->second;
			const GPlatesModel::GpgimVersion &file_gpgim_version =
					boost::any_cast<const GPlatesModel::GpgimVersion &>(tag);

			// If the file GPGIM version does not match the current GPGIM version then we
			// need to warn the user.
			if (file_gpgim_version == current_gpgim_version)
			{
				continue;
			}

			const QString filename = file.get_file().get_file_info().get_display_name(false) +
					" (" + file_gpgim_version.version_string() + ")";

			if (file_gpgim_version < current_gpgim_version)
			{
				older_version_filenames.append(filename);
			}
			else
			{
				newer_version_filenames.append(filename);
			}
		}

		return older_version_filenames.count() > 0 || newer_version_filenames.count() > 0;
	}


	/**
	 * Shows the GPGIM version warning dialog, if necessary, to inform the user that there exist
	 * files with a different GPGIM version than the current GPGIM version (built into this GPlates).
	 *
	 * If @a only_unsaved_changes is true then only files with unsaved changes will be saved.
	 *
	 * Returns true if the files should be saved.
	 */
	bool
	show_save_files_gpgim_version_dialog_if_necessary(
			const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &files,
			bool only_unsaved_changes,
			GPlatesQtWidgets::GpgimVersionWarningDialog *gpgim_version_warning_dialog)
	{
		QStringList older_version_filenames;
		QStringList newer_version_filenames;

		// If there are no older or newer versions then we can save the files without querying the user.
		if (!get_files_with_different_gpgim_version(
			files,
			only_unsaved_changes,
			older_version_filenames,
			newer_version_filenames))
		{
			return true;
		}

		// Set up the GPGIM version warning dialog.
		gpgim_version_warning_dialog->set_action_requested(
				GPlatesQtWidgets::GpgimVersionWarningDialog::SAVE_FILES,
				older_version_filenames,
				newer_version_filenames);

		// Exec the dialog and return true if the files should be saved.
		return gpgim_version_warning_dialog->exec() == QDialogButtonBox::Save;
	}


	/**
	 * Shows the GPGIM version warning dialog, if necessary, to inform the user that there exist
	 * files with a different GPGIM version than the current GPGIM version (built into this GPlates).
	 */
	void
	show_open_files_gpgim_version_dialog_if_necessary(
			const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &files,
			GPlatesQtWidgets::GpgimVersionWarningDialog *gpgim_version_warning_dialog)
	{
		// Do not warn the user if requested us to stop bothering them.
		// Not that we only have this option for loading files.
		// When saving files the user is always warned.
		if (gpgim_version_warning_dialog->do_not_show_dialog_on_loading_files())
		{
			return;
		}

		QStringList older_version_filenames;
		QStringList newer_version_filenames;

		// If there are no older or newer versions then we don't need to warn the user.
		if (!get_files_with_different_gpgim_version(
			files,
			false/*only_unsaved_changes*/, // Include all files (saved or unsaved).
			older_version_filenames,
			newer_version_filenames))
		{
			return;
		}

		// Set up the GPGIM version warning dialog.
		gpgim_version_warning_dialog->set_action_requested(
				GPlatesQtWidgets::GpgimVersionWarningDialog::LOAD_FILES,
				older_version_filenames,
				newer_version_filenames);

		// Exec the dialog - it's just an informational dialog so we're not interested in the return code.
		gpgim_version_warning_dialog->exec();
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
			view_state_),
	d_gpgim_version_warning_dialog_ptr(
			new GPlatesQtWidgets::GpgimVersionWarningDialog(
					&viewport_window()))
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

	// Collect the files loaded over the current scope.
	CollectLoadedFilesScope collect_loaded_files_scope(d_file_state_ptr);

	try_catch_file_or_session_load_with_feedback(
			boost::bind(
					&GPlatesAppLogic::FeatureCollectionFileIO::load_files,
					d_feature_collection_file_io_ptr,
					filenames));

	// Warn the user if they have loaded files with different GPGIM versions than the files
	// were originally created with. The user might then decide not to modify files since they
	// could then only be saved using the current GPGIM version potentially causing problems for
	// other (older) versions of GPlates.
	show_open_files_gpgim_version_dialog_if_necessary(
			collect_loaded_files_scope.get_loaded_files(),
			d_gpgim_version_warning_dialog_ptr);
}


void
GPlatesGui::FileIOFeedback::open_urls(
		const QList<QUrl> &urls)
{
	if (urls.isEmpty())
	{
		return;
	}

	try_catch_file_or_session_load_with_feedback(
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
		
		// Collect the files loaded over the current scope.
		//
		// TODO: Once the Scribe session save/restore functionality is merged we won't be able
		// to rely on file added signals so we'll then need to query all loaded files (since the
		// session restore replaces all loaded files).
		CollectLoadedFilesScope collect_loaded_files_scope(d_file_state_ptr);

		// Load the new session.
		try_catch_file_or_session_load_with_feedback(
				boost::bind(
						&GPlatesAppLogic::SessionManagement::load_previous_session,
						&sm,
						session_slot_to_load));

		// Warn the user if they have loaded files with different GPGIM versions than the files
		// were originally created with. The user might then decide not to modify files since they
		// could then only be saved using the current GPGIM version potentially causing problems for
		// other (older) versions of GPlates.
		show_open_files_gpgim_version_dialog_if_necessary(
				collect_loaded_files_scope.get_loaded_files(),
				d_gpgim_version_warning_dialog_ptr);
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
			focused_property_name = (*prop_iter)->get_property_name();
		}
	}

	try_catch_file_or_session_load_with_feedback(
			boost::bind(
					&GPlatesAppLogic::FeatureCollectionFileIO::reload_file,
					d_feature_collection_file_io_ptr,
					file),
			file.get_file().get_file_info().get_display_name(false/*use_absolute_path_name*/));

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
					if ((*iter)->get_property_name() == focused_property_name)
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
		return save_file(file);
	}
}


bool
GPlatesGui::FileIOFeedback::save_file_in_place(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	// Warn the user if they are about to save the file using a different GPGIM version than the file
	// was originally created with.
	if (!show_save_files_gpgim_version_dialog_if_necessary(
			std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>(1, file),
			// We're saving whether the file has unsaved changes or not...
			false/*only_unsaved_changes*/,
			d_gpgim_version_warning_dialog_ptr))
	{
		// Return without saving.
		return false;
	}

	file.get_file().get_feature_collection();

	// Save the feature collection with GUI feedback.
	return save_file(file.get_file());
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
	const GPlatesFileIO::FileInfo new_fileinfo(filename);

	// Create a temporary file reference to contain the relevant file information.
	//
	// NOTE: We use the file configuration of original file even though it may be for a different
	// file format because it might still be usable in a new file format (eg, model-to-attribute
	// mapping can be shared across the variety of OGR file formats) - if it doesn't match then
	// it will just get replace by the file writer.
	GPlatesFileIO::File::Reference::non_null_ptr_type file_ref =
			GPlatesFileIO::File::create_file_reference(
					new_fileinfo,
					file.get_file().get_feature_collection(),
					file.get_file().get_file_configuration());

	// Save the feature collection, with GUI feedback.
	bool ok = save_file(*file_ref);
	
	// If there was an error saving, don't change the fileinfo.
	if ( ! ok) {
		return false;
	}

	// Change the file info in the file - this will emit signals to interested observers.
	//
	// NOTE: We get the file configuration from the temporary file reference because the file
	// writer may have created a new file configuration and attached it there.
	file.set_file_info(new_fileinfo, file_ref->get_file_configuration());
	
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
	const GPlatesFileIO::FileInfo new_fileinfo(filename);

	// Create a temporary file reference to contain the relevant file information.
	//
	// NOTE: We use the file configuration of original file even though it may be for a different
	// file format because it might still be usable in a new file format (eg, model-to-attribute
	// mapping can be shared across the variety of OGR file formats) - if it doesn't match then
	// it will just get replace by the file writer.
	GPlatesFileIO::File::Reference::non_null_ptr_type file_ref =
			GPlatesFileIO::File::create_file_reference(
					new_fileinfo,
					file.get_file().get_feature_collection(),
					file.get_file().get_file_configuration());

	// Save the feature collection, with GUI feedback.
	//
	// NOTE: The 'clear_unsaved_changes' flag is set to false because we are not really
	// saving the changes to the original file (only making a copy) whereas the original file
	// is still associated with the unsaved feature collection.
	save_file(*file_ref, false/*clear_unsaved_changes*/);

	return true;
}


bool
GPlatesGui::FileIOFeedback::save_file(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	bool ok = save_file(file.get_file());
	if (!ok)
	{
		return false;
	}

	// Let FeatureCollectionFileState and all its listeners (like ManageFeatureCollectionsDialog)
	// know that the file has been written to since it's possible that the file did not exist
	// before now and hence "New Feature Collection" will get displayed even though the file now
	// exists and has a proper filename.
	//
	// Setting the file info will cause the filenames (ManageFeatureCollectionsDialog) to get re-populated.
	// TODO: Find a better way to do this.
	file.set_file_info(file.get_file().get_file_info());

	return true;
}


bool
GPlatesGui::FileIOFeedback::save_file(
		GPlatesFileIO::File::Reference &file_ref,
		bool clear_unsaved_changes)
{
	// Pop-up error dialogs need a parent so that they don't just blindly appear in the centre of
	// the screen.
	QWidget *parent_widget = &(viewport_window());

	try
	{
		// Save the feature collection. This is where we finally dip down into the file-io level.
		d_feature_collection_file_io_ptr->save_file(file_ref, clear_unsaved_changes);
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &exc)
	{
		QString message;
		QTextStream(&message)
				<< tr("An error occurred while saving the file '%1': \n").arg(exc.filename())
				<< exc;
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
		return false;
	}
	catch (GPlatesFileIO::ErrorOpeningPipeToGzipException &exc)
	{
		QString message;
		QTextStream(&message)
				<< tr("GPlates was unable to use the '%1' program to save the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to save"
				" files without compression: \n")
						.arg(exc.command())
						.arg(exc.filename())
				<< exc;
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
		return false;
	}
	catch (GPlatesGlobal::InvalidFeatureCollectionException &exc)
	{
		QString message;
		QTextStream(&message)
				<< tr("Error: Attempted to write an invalid feature collection to '%1': \n")
						.arg(file_ref.get_file_info().get_display_name(false/*use_absolute_path_name*/))
				<< exc;
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
		return false;
	}
	catch (GPlatesGlobal::UnexpectedEmptyFeatureCollectionException &exc)
	{
		QString message;
		QTextStream(&message)
				<< tr("Error: Attempted to write an empty feature collection to '%1': \n")
						.arg(file_ref.get_file_info().get_display_name(false/*use_absolute_path_name*/))
				<< exc;
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
		return false;
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &exc)
	{
		QString message;
		QTextStream(&message)
				<< tr("Error: Writing files in the format of '%1' is currently not supported: \n")
						.arg(file_ref.get_file_info().get_display_name(false/*use_absolute_path_name*/))
				<< exc;
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
		return false;
	}
	catch (GPlatesFileIO::OgrException &exc)
	{
		QString message;
		QTextStream(&message)
				<< tr("An OGR error occurred while saving the file '%1': \n")
						.arg(file_ref.get_file_info().get_display_name(false/*use_absolute_path_name*/))
				<< exc;
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
		return false;
	}
	catch (GPlatesFileIO::ErrorWritingFeatureCollectionToFileFormatException &exc)
	{
		// Remove the file on disk in case it was partially written.
		QFile(file_ref.get_file_info().get_qfileinfo().filePath()).remove();

		QString message;
		QTextStream(&message)
			<< tr("Error: Unable to write the file '%1' due to a file format limitation: \n")
						.arg(file_ref.get_file_info().get_display_name(false/*use_absolute_path_name*/))
				<< exc;
		QMessageBox::critical(parent_widget, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
		return false;
	}

	// Since a file has just been saved (successfully), we should let UnsavedChangesTracker know.
	unsaved_changes_tracker().handle_model_has_changed();

	return true;
}


bool
GPlatesGui::FileIOFeedback::save_files(
		const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &files,
		bool include_unnamed_files,
		bool only_unsaved_changes)
{
	// Warn the user if they are about to save files using a different GPGIM version than the files
	// were originally created with.
	if (!show_save_files_gpgim_version_dialog_if_necessary(
			files,
			only_unsaved_changes,
			d_gpgim_version_warning_dialog_ptr))
	{
		// Return without saving.
		//
		// Even if some of the files have the current GPGIM version (and hence would normally have
		// been saved without warning) those files should not be saved because we shouldn't save
		// some files but not others because it may not make logical sense to partially save
		// a group of files (they may become inconsistent with each other).
		return false;
	}

	viewport_window().status_message("GPlates is saving files...");

	// Return true only if all files saved without issue.
	bool all_ok = true;

	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file,
			files)
	{
		// Attempt to ensure GUI still gets updates... FIXME, it's not enough.
		QCoreApplication::processEvents();

		// Get the FeatureCollectionHandle, to determine unsaved state.
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
				file.get_file().get_feature_collection();
		if (!feature_collection_ref.is_valid())
		{
			continue;
		}
		
		// If we are only saving files with unsaved changes then skip those that have no changes.
		if (only_unsaved_changes && !feature_collection_ref->contains_unsaved_changes())
		{
			continue;
		}

		// Previously we only saved the file if there were unsaved changes.
		// However we now save regardless to ensure that the GPGIM version written to the file
		// is the current GPGIM version. It's possible the user loaded an old GPGIM-version file
		// and are now attempting to save it as the current GPGIM version.

		// For now, to avoid pointless 'give me a name for this file (which you can't identify)'
		// situations, only save the files which we have a name for already (unless @a include_unnamed_files)
		if (file_is_unnamed(file) && ! include_unnamed_files)
		{
			// Skip the unnamed file.
		}
		else
		{
			// Save the feature collection, in place or with dialog, with GUI feedback.
			bool ok = save_file_as_appropriate(file);
			// save_all() needs to report any failures.
			if ( ! ok)
			{
				all_ok = false;
			}

		}
	}

	// Some more user feedback in the status message.
	if (all_ok)
	{
		viewport_window().status_message("Files were saved successfully.", 2000);
	}
	else
	{
		viewport_window().status_message("Some files could not be saved.");
	}

	return all_ok;
}


bool
GPlatesGui::FileIOFeedback::save_all(
		bool include_unnamed_files,
		bool only_unsaved_changes)
{
	// For each loaded file; if it has unsaved changes, behave as though 'save in place' was clicked.
	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			d_file_state_ptr->get_loaded_files();

	return save_files(loaded_files, include_unnamed_files, only_unsaved_changes);
}


bool
GPlatesGui::FileIOFeedback::create_file(
		const GPlatesFileIO::File::non_null_ptr_type &file)
{
	const bool saved = save_file(file->get_reference());

	// Add the new file to the feature collection file state.
	// We don't save it because we've already saved it above.
	// The reason we save above is to pop up an error dialog if saving fails -
	// this won't happen if we save directly through 'FeatureCollectionFileIO'.
	if (saved)
	{
		d_feature_collection_file_io_ptr->create_file(file, false/*save*/);
	}

	return saved;
}


void
GPlatesGui::FileIOFeedback::try_catch_file_or_session_load_with_feedback(
		boost::function<void ()> file_load_func,
		boost::optional<QString> filename)
{
	// Pop-up error dialogs need a parent so that they don't just blindly appear in the centre of
	// the screen.
	QWidget *parent_widget = &(viewport_window());

	// FIXME: Try to ensure the filename is in these error dialogs.
	try
	{
		file_load_func();
	}
	catch (GPlatesFileIO::ErrorOpeningPipeFromGzipException &exc)
	{
		QString message;
		QTextStream(&message)
				<< tr("GPlates was unable to use the '%1' program to read the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to open"
				" files which are not compressed: \n")
						.arg(exc.command())
						.arg(exc.filename())
				<< exc;
		QMessageBox::critical(parent_widget, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &exc)
	{
		QString message;
		QTextStream message_stream(&message);
		if (filename)
		{
			message_stream << tr("Error: Loading files in the format of '%1' is currently not supported: \n")
						.arg(filename.get());
		}
		else
		{
			message_stream << tr("Error: Loading files in this format is currently not supported: \n");
		}
		message_stream << exc;
		QMessageBox::critical(parent_widget, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
	}
	catch (GPlatesFileIO::ErrorOpeningFileForReadingException &exc)
	{
		QString message;
		QTextStream(&message)
				<< tr("Error: GPlates was unable to read the file '%1': \n").arg(exc.filename())
				<< exc;
		QMessageBox::critical(parent_widget, tr("Error Opening File"), message,
							  QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
	}
	catch (GPlatesFileIO::FileLoadAbortedException &exc)
	{
		QString message;
		QTextStream(&message)
				<< tr("File load aborted when reading file '%1': \n").arg(exc.filename())
				<< exc;
		// Don't display a message box here. The only way this exception is thrown
		// at present is if the user cancels shapefile import (by cancelling the mapping dialog),
		// and this doesn't need message box -it makes it look like something bad has happened.
		//
		// Abandoning the mapping process isn't really an error so probably shouldn't throw,
		// but that's the easiest way of getting out of the file-load procedure at the moment.
		qWarning() << message; // Log the detailed error message.
	}
	catch (GPlatesGlobal::Exception &exc)
	{
		QString message;
		QTextStream message_stream(&message);
		if (filename)
		{
			message_stream << tr("Error: Unexpected error loading file '%1' - ignoring file: \n")
						.arg(filename.get());
		}
		else
		{
			message_stream << tr("Error: Unexpected error loading file - ignoring file: \n");
		}
		message_stream << exc;
		QMessageBox::critical(parent_widget, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
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
	return d_viewport_window_ptr->dialogs().manage_feature_collections_dialog();
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


GPlatesGui::CollectLoadedFilesScope::CollectLoadedFilesScope(
		GPlatesAppLogic::FeatureCollectionFileState *feature_collection_file_state)
{
	QObject::connect(
			feature_collection_file_state,
			SIGNAL(file_state_files_added(
					GPlatesAppLogic::FeatureCollectionFileState &,
					const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &)),
			this,
			SLOT(handle_file_state_files_added(
					GPlatesAppLogic::FeatureCollectionFileState &,
					const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &)));
}


const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &
GPlatesGui::CollectLoadedFilesScope::get_loaded_files() const
{
	return d_loaded_files;
}


void
GPlatesGui::CollectLoadedFilesScope::handle_file_state_files_added(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files)
{
	// Add to the list of new files.
	d_loaded_files.insert(d_loaded_files.end(), new_files.begin(), new_files.end());
}
