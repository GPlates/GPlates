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

#include "FeatureCollectionFileIO.h"

#include "app-logic/AppLogicUtils.h"

#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/FeatureCollectionReaderWriter.h"
#include "file-io/ShapefileReader.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/InvalidFeatureCollectionException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"

#include "model/Model.h"


GPlatesAppLogic::FeatureCollectionFileIO::FeatureCollectionFileIO(
		GPlatesModel::ModelInterface &model,
		GPlatesAppLogic::FeatureCollectionFileState &file_state) :
	d_model(model),
	d_file_state(file_state),
	d_modify_filter(new ModifyFilter()/*default filter does not modify*/)
{
}


void
GPlatesAppLogic::FeatureCollectionFileIO::set_modify_filter(
		const boost::shared_ptr<ModifyFilter> &modify_filter)
{
	d_modify_filter = modify_filter;
}


void
GPlatesAppLogic::FeatureCollectionFileIO::load_files(
		const QStringList &filenames)
{
	// Read all the files before we add them to the application state.
	const file_seq_type loaded_files = read_feature_collections(filenames);

	// See if any loaded feature collections should be modified.
	modify_feature_collections(loaded_files);

	// Add files to the application state in one call.
	//
	// NOTE: It is important to load multiple files in one group here rather than
	// reuse load_file() for each file because the application state will then send
	// only one notification instead of multiple notifications which is needed in
	// some cases where the files in the group depend on each other - an example is
	// topological boundary features which get resolved after the notification and
	// require any referenced features to be loaded into the model (and they might
	// be in other files in the group).
	d_file_state.add_files(loaded_files);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::load_file(
		const QString &filename)
{
	const GPlatesFileIO::FileInfo file_info(filename);

	// Read the feature collection from file_info.
	const GPlatesFileIO::File::shared_ref loaded_file = read_feature_collection(file_info);

	// See if loaded feature collection should be modified.
	modify_feature_collection(loaded_file);

	d_file_state.add_file(loaded_file);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::reload_file(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file)
{
	// Read the feature collection from file.
	const GPlatesFileIO::File::shared_ref reloaded_file = read_feature_collection(
			file->get_file_info());

	// See if loaded feature collection should be modified.
	modify_feature_collection(reloaded_file);

	// Replace old file with reloaded file.
	// This will emit signals for anyone interested.
	d_file_state.reset_file(file, reloaded_file);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::unload_file(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator loaded_file)
{
	// Remove the loaded file from the application state - if the application state
	// holds the only file reference (which it should unless some other code is still
	// using it - which is also ok but might be by accident) then the feature collection
	// will automatically get unloaded.
	d_file_state.remove_file(loaded_file);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::save_file(
		const GPlatesFileIO::FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format)
{
	// The following check is commented out because it fails in certain circumstances
	// on newer versions of Windows. We'll just try and open the file for writing
	// and throw an exception if it fails.
#if 0
	if (!GPlatesFileIO::is_writable(file_info))
	{
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
				file_info.get_qfileinfo().filePath());
	}
#endif

	if (!feature_collection.is_valid())
	{
		throw GPlatesGlobal::InvalidFeatureCollectionException(GPLATES_EXCEPTION_SOURCE,
				"Attempted to write an invalid feature collection.");
	}
	
	boost::shared_ptr<GPlatesModel::ConstFeatureVisitor> feature_collection_writer =
			GPlatesFileIO::get_feature_collection_writer(
					file_info,
					feature_collection,
					feature_collection_write_format);

	// Write the feature collection.
	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			feature_collection, *feature_collection_writer);

	feature_collection->clear_unsaved_changes();
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator
GPlatesAppLogic::FeatureCollectionFileIO::create_empty_file()
{
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			GPlatesModel::FeatureCollectionHandle::create(d_model->root());

	// Make sure feature collection gets unloaded when it's no longer needed.
	GPlatesModel::FeatureCollectionHandleUnloader::shared_ref feature_collection_unloader =
			GPlatesModel::FeatureCollectionHandleUnloader::create(feature_collection);

	GPlatesFileIO::File::shared_ref file = GPlatesFileIO::File::create_empty_file(
			feature_collection_unloader);

	// The file contains no features yet so it won't get classified when we add it (actually
	// I think at the moment it will get classified as reconstructable since it contains
	// no reconstruction features - but this way of classifying could easily change).
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_file_it =
			d_file_state.add_file(file);

	return new_file_it;
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator
GPlatesAppLogic::FeatureCollectionFileIO::create_file(
		const GPlatesFileIO::FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format format)
{
	// Make sure feature collection gets unloaded when it's no longer needed.
	GPlatesModel::FeatureCollectionHandleUnloader::shared_ref feature_collection_unloader =
			GPlatesModel::FeatureCollectionHandleUnloader::create(feature_collection);

	GPlatesFileIO::File::shared_ref file = GPlatesFileIO::File::create_loaded_file(
			feature_collection_unloader, file_info);

	save_file(file_info, feature_collection, format);

	GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_file_it =
			d_file_state.add_file(file);

	return new_file_it;
}


void
GPlatesAppLogic::FeatureCollectionFileIO::remap_shapefile_attributes(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it)
{
	GPlatesFileIO::ReadErrorAccumulation read_errors;
	GPlatesFileIO::ShapefileReader::remap_shapefile_attributes(
			*file_it,
			d_model,
			read_errors);

	emit_handle_read_errors_signal(read_errors);

	emit remapped_shapefile_attributes(*this, file_it);

	// Re-classify the feature collection.
	d_file_state.reclassify_feature_collection(file_it);
}


GPlatesAppLogic::FeatureCollectionFileIO::file_seq_type
GPlatesAppLogic::FeatureCollectionFileIO::read_feature_collections(
		const QStringList &filenames)
{
	file_seq_type files;

	GPlatesFileIO::ReadErrorAccumulation read_errors;

	QStringList::const_iterator filename_iter = filenames.begin();
	QStringList::const_iterator filename_end = filenames.end();
	for ( ; filename_iter != filename_end; ++filename_iter)
	{
		const QString &filename = *filename_iter;

		GPlatesFileIO::FileInfo file_info(filename);

		// Read the feature collection from file.
		const GPlatesFileIO::File::shared_ref file = GPlatesFileIO::read_feature_collection(
				file_info, d_model, read_errors);

		files.push_back(file);
	}

	// Emit one signal for all loaded files.
	emit_handle_read_errors_signal(read_errors);

	return files;
}


const GPlatesFileIO::File::shared_ref
GPlatesAppLogic::FeatureCollectionFileIO::read_feature_collection(
		const GPlatesFileIO::FileInfo &file_info)
{
	GPlatesFileIO::ReadErrorAccumulation read_errors;

	// Read the feature collection from file.
	const GPlatesFileIO::File::shared_ref file = GPlatesFileIO::read_feature_collection(
			file_info, d_model, read_errors);

	emit_handle_read_errors_signal(read_errors);

	return file;
}


void
GPlatesAppLogic::FeatureCollectionFileIO::modify_feature_collections(
		const file_seq_type &files)
{
	d_modify_filter->modify_loaded_files(files);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::modify_feature_collection(
		const GPlatesFileIO::File::shared_ref &file)
{
	// Create a vector with one file so we can reuse 'modify_feature_collections()'.
	const file_seq_type file_seq(1, file);

	modify_feature_collections(file_seq);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::emit_handle_read_errors_signal(
		const GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	// Here we also emit a signal if there were any read errors/warnings.
	// This is useful for client code interested in displaying errors to the user.
	if (!read_errors.is_empty())
	{
		emit handle_read_errors(*this, read_errors);
	}
}
