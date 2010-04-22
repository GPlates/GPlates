/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <QDebug>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <iostream>
#include <boost/bind.hpp>

#include "app-logic/FeatureCollectionFileIO.h"
#include "file-io/FileInfo.h"
#include "file-io/ExternalProgram.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/ErrorOpeningPipeToGzipException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/GpmlOnePointSixOutputVisitor.h"
#include "file-io/OgrException.h"
#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/InvalidFeatureCollectionException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"
#include "gui/UnsavedChangesTracker.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/ManageFeatureCollectionsDialog.h"
#include "qt-widgets/GMTHeaderFormatDialog.h"

#include "FileIOFeedback.h"


namespace
{
	/**
	 * Builds the specially-formatted list of suitable output filters given a file
	 * to be saved. The result can be fed into the Save As or Save a Copy dialogs.
	 */
	GPlatesQtWidgets::SaveFileDialog::filter_list_type
	get_output_filters_for_file(
			GPlatesAppLogic::FeatureCollectionFileState &file_state,
			GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_iter,
			bool has_gzip)
	{
		using std::make_pair;

		// Appropriate filters for available output formats.
		static const QString filter_gmt(QObject::tr("GMT xy (*.xy)"));
		static const QString filter_gmt_ext(QObject::tr("xy"));
		static const QString filter_line(QObject::tr("PLATES4 line (*.dat *.pla)"));
		static const QString filter_line_ext(QObject::tr("dat"));
		static const QString filter_rotation(QObject::tr("PLATES4 rotation (*.rot)"));
		static const QString filter_rotation_ext(QObject::tr("rot"));
		static const QString filter_shapefile(QObject::tr("ESRI shapefile (*.shp)"));
		static const QString filter_shapefile_ext(QObject::tr("shp"));
		static const QString filter_gpml(QObject::tr("GPlates Markup Language (*.gpml)"));
		static const QString filter_gpml_ext(QObject::tr("gpml"));
		static const QString filter_gpml_gz(QObject::tr("Compressed GPML (*.gpml.gz)"));
		static const QString filter_gpml_gz_ext(QObject::tr("gpml.gz"));
		static const QString filter_all(QObject::tr("All files (*)"));
		static const QString filter_all_ext(QObject::tr(""));
		
		// Determine whether file contains reconstructable and/or reconstruction features.
		const GPlatesAppLogic::ClassifyFeatureCollection::classifications_type classification =
				file_state.get_feature_collection_classification(file_iter);
		const bool has_features_with_geometry =
				GPlatesAppLogic::ClassifyFeatureCollection::found_geometry_feature(classification);
		const bool has_reconstruction_features =
				GPlatesAppLogic::ClassifyFeatureCollection::found_reconstruction_feature(classification);

		GPlatesQtWidgets::SaveFileDialog::filter_list_type filters;
		
		// Build the list of filters depending on the original file format.
		switch ( file_iter->get_loaded_file_format() )
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
			{
				filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				filters.push_back(make_pair(filter_line, filter_line_ext));
				filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
			{
				filters.push_back(make_pair(filter_line, filter_line_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
				filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
			{
				filters.push_back(make_pair(filter_rotation, filter_rotation_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;
			
		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			{
				filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				filters.push_back(make_pair(filter_line, filter_line_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;
		case GPlatesFileIO::FeatureCollectionFileFormat::GMAP:
			{
#if 0			
				// Disable any kind of export from GMAP data. 
				break;
#else			
				// No GMAP writing yet. (Ever?).
				// Offer gmpl/gpml_gz export.
				// 
				// (Probably not useful to export these in shapefile or plates formats
				// for example - users would get the geometries but nothing else).
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				break;
#endif
			}
		case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
		case GPlatesFileIO::FeatureCollectionFileFormat::UNKNOWN:
		default: // If no file extension then use same options as GMPL.
			{
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext)); // Save uncompressed by default, same as original
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext)); // Option to change to compressed version.
				}
				if (has_features_with_geometry)
				{
					// Only offer to save in line-only formats if feature collection
					// actually has reconstructable features in it!
					filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
					filters.push_back(make_pair(filter_line, filter_line_ext));
					filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				}
				if (has_reconstruction_features)
				{
					// Only offer to save as PLATES4 .rot if feature collection
					// actually has rotations in it!
					filters.push_back(make_pair(filter_rotation, filter_rotation_ext));
				}
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
		case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
			{
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext)); // Save compressed by default, assuming we can.
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext)); // Option to change to uncompressed version.
				if (has_features_with_geometry)
				{
					// Only offer to save in line-only formats if feature collection
					// actually has reconstructable features in it!
					filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
					filters.push_back(make_pair(filter_line, filter_line_ext));
					filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				}
				if (has_reconstruction_features)
				{
					// Only offer to save as PLATES4 .rot if feature collection
					// actually has rotations in it!
					filters.push_back(make_pair(filter_rotation, filter_rotation_ext));
				}
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;
		}

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
			GPlatesAppLogic::FeatureCollectionFileState::file_iterator file)
	{
		// Get the QFileInfo, to determine unnamed state.
		const QFileInfo &qfileinfo = file->get_file_info().get_qfileinfo();
		return qfileinfo.fileName().isEmpty();
	}
}



GPlatesGui::FileIOFeedback::FileIOFeedback(
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesAppLogic::FeatureCollectionFileState &file_state_,
		GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io_,
		QObject *parent_):
	QObject(parent_),
	d_viewport_window_ptr(&viewport_window_),
	d_file_state_ptr(&file_state_),
	d_feature_collection_file_io_ptr(&feature_collection_file_io_),
	d_save_file_as_dialog_ptr(
			GPlatesQtWidgets::SaveFileDialog::get_save_file_dialog(
				d_viewport_window_ptr,
				"Save File As",
				GPlatesQtWidgets::SaveFileDialog::filter_list_type())),
	d_save_file_copy_dialog_ptr(
			GPlatesQtWidgets::SaveFileDialog::get_save_file_dialog(
				d_viewport_window_ptr,
				"Save a copy of the file with a different name",
				GPlatesQtWidgets::SaveFileDialog::filter_list_type())),
	d_gzip_available(false),
	d_open_file_path("")
{
	setObjectName("FileIOFeedback");

	// Test if we can offer on-the-fly gzip compression.
	// FIXME: Ideally we should let the user know WHY we're concealing this option.
	// The user will still be able to type in a .gpml.gz file name and activate the
	// gzipped saving code, however this will produce an exception which pops up
	// a suitable message box (See ViewportWindow.cc)
	d_gzip_available = GPlatesFileIO::GpmlOnePointSixOutputVisitor::gzip_program().test();
}



void
GPlatesGui::FileIOFeedback::open_files()
{
	// FIXME: Move up to the anon namespace area with the save filters.
	static const QString filters = tr(
			"All loadable files (*.dat *.pla *.rot *.shp *.gpml *.gpml.gz *.vgp);;"
			"PLATES4 line (*.dat *.pla);;"
			"PLATES4 rotation (*.rot);;"
			"ESRI shapefile (*.shp);;"
			"GPlates Markup Language (*.gpml *.gpml.gz);;"
			"GMAP VGP file (*.vgp);;"
			"All files (*)" );
	
	QStringList filenames = QFileDialog::getOpenFileNames(&(viewport_window()),
			tr("Open Files"), d_open_file_path, filters);

	open_files(filenames);
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

	// Set open file path to the last opened file.
	if ( ! filenames.isEmpty() )
	{
		QFileInfo last_opened_file(filenames.last());
		d_open_file_path = last_opened_file.path();
	}
	// FIXME: Dropped during refactoring. Was probably unnecessary.. right? //highlight_unsaved_changes();
}


void
GPlatesGui::FileIOFeedback::reload_file(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator &file)
{
	try_catch_file_load_with_feedback(
			boost::bind(
					&GPlatesAppLogic::FeatureCollectionFileIO::reload_file,
					d_feature_collection_file_io_ptr,
					file));
}


bool
GPlatesGui::FileIOFeedback::save_file_as_appropriate(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file)
{
	if (file_is_unnamed(file)) {
		return save_file_as(file);
	} else {
		return save_file_in_place(file);
	}
}


bool
GPlatesGui::FileIOFeedback::save_file_in_place(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file)
{
	// Get the format to write feature collection in.
	// This is usually determined by file extension but some format also
	// require user preferences (eg, style of feature header in file).
	GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format =
		get_feature_collection_write_format(file->get_file_info());
	
	// Save the feature collection with GUI feedback.
	bool ok = save_file(
			file->get_file_info(),
			file->get_feature_collection(),
			feature_collection_write_format);
	return ok;
}


bool
GPlatesGui::FileIOFeedback::save_file_as(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file)
{
	// Configure and open the Save As dialog.
	d_save_file_as_dialog_ptr->set_filters(
			get_output_filters_for_file(
				*d_file_state_ptr,
				file,
				d_gzip_available));
	QString file_path = file->get_file_info().get_qfileinfo().filePath();
	if (file_path != "")
	{
		d_save_file_as_dialog_ptr->select_file(file_path);
	}
	boost::optional<QString> filename_opt = d_save_file_as_dialog_ptr->get_file_name();

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
			filename, file->get_file_info());

	// Get the format to write feature collection in.
	// This is usually determined by file extension but some format also
	// require user preferences (eg, style of feature header in file).
	GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format =
		get_feature_collection_write_format(new_fileinfo);

	// Save the feature collection, with GUI feedback.
	bool ok = save_file(
			new_fileinfo,
			file->get_feature_collection(),
			feature_collection_write_format);
	
	// If there was an error saving, don't change the fileinfo.
	if ( ! ok) {
		return false;
	}

	// Wrap the same feature collection up with the new FileInfo.
	const GPlatesFileIO::File::shared_ref new_file = GPlatesFileIO::File::create_save_file(
			*file, new_fileinfo);

	// Update the name of the file by replacing with the new File object.
	// NOTE: this also removes the old file object thus making 'file'
	// a dangling reference.
	// FIXME: Is this a FIXME? I can't grok what this implies right now. -JC
	d_file_state_ptr->reset_file(file, new_file);
	
	return true;
}


bool
GPlatesGui::FileIOFeedback::save_file_copy(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file)
{
	// Configure and pop up the Save a Copy dialog.
	d_save_file_copy_dialog_ptr->set_filters(
			get_output_filters_for_file(
				*d_file_state_ptr,
				file,
				d_gzip_available));
	QString file_path = file->get_file_info().get_qfileinfo().filePath();
	if (file_path != "")
	{
		d_save_file_copy_dialog_ptr->select_file(file_path);
	}
	boost::optional<QString> filename_opt = d_save_file_copy_dialog_ptr->get_file_name();

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
			filename, file->get_file_info());

	// Get the format to write feature collection in.
	// This is usually determined by file extension but some format also
	// require user preferences (eg, style of feature header in file).
	GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format =
			get_feature_collection_write_format(new_fileinfo);

	// Save the feature collection, with GUI feedback.
	save_file(
			new_fileinfo,
			file->get_feature_collection(),
			feature_collection_write_format);

	return true;
}




bool
GPlatesGui::FileIOFeedback::save_file(
		const GPlatesFileIO::FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format)
{
	// Pop-up error dialogs need a parent so that they don't just blindly appear in the centre of
	// the screen.
	QWidget *parent_widget = &(viewport_window());

	try
	{
		// Save the feature collection. This is where we finally dip down into the file-io level.
		GPlatesAppLogic::FeatureCollectionFileIO::save_file(
				file_info,
				feature_collection,
				feature_collection_write_format);
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
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range it_range =
			d_file_state_ptr->get_loaded_files();
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator it = it_range.begin;
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator end = it_range.end;

	// Return true only if all files saved without issue.
	bool all_ok = true;
	
	for (; it != end; ++it) {
		// Attempt to ensure GUI still gets updates... FIXME, it's not enough.
		QCoreApplication::processEvents();

		// Get the FeatureCollectionHandle, to determine unsaved state.
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref = it->get_feature_collection();

		// Does this file need saving?
		if (feature_collection_ref.is_valid() && feature_collection_ref->contains_unsaved_changes()) {
			// For now, to avoid pointless 'give me a name for this file (which you can't identify)'
			// situations, only save the files which we have a name for already (unless @a include_unnamed_files)
			if (file_is_unnamed(it) && ! include_unnamed_files) {
				// Skip the unnamed file.
			} else {
				// Save the feature collection, in place or with dialog, with GUI feedback.
				bool ok = save_file_as_appropriate(it);
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




// FIXME: a few problems with this method.
//  - it doesn't remember the GMT header setting. Should be stored in file info somewhere.
//  - because it's here, it pops up on Save In Place as well as Save As. Save As is fine,
//    but Save In Place causes problems with Save All, because the user will have no clue
//    which file is popping up this dialog.
//  - maybe it could be merged into the shapefile configuration button, but that would ideally
//    depend on being able to *read* and write gmt xy files.
GPlatesFileIO::FeatureCollectionWriteFormat::Format
GPlatesGui::FileIOFeedback::get_feature_collection_write_format(
		const GPlatesFileIO::FileInfo& file_info)
{
	switch (get_feature_collection_file_format(file_info) )
	{
	case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
		{
			GPlatesQtWidgets::GMTHeaderFormatDialog gmt_header_format_dialog(d_viewport_window_ptr);
			gmt_header_format_dialog.exec();

			return gmt_header_format_dialog.get_header_format();
		}

	default:
		return GPlatesFileIO::FeatureCollectionWriteFormat::USE_FILE_EXTENSION;
	}
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
	catch (GPlatesGlobal::Exception &e)
	{
		QString message = tr("Error: Unexpected error loading file - ignoring file.");
		QMessageBox::critical(parent_widget, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);

		std::cerr << "Caught exception: " << e << std::endl;
	}
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


