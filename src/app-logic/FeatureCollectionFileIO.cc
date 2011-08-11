/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "file-io/ArbitraryXmlReader.h"
#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/GeoscimlProfile.h"
#include "file-io/ShapefileReader.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/InvalidFeatureCollectionException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"

#include "model/ChangesetHandle.h"
#include "model/NotificationGuard.h"
#include "model/Model.h"


namespace
{
	/**
	 * Transforms a list of file:// urls into a list of pathnames in string form.
	 * Ignores any non-file url.
	 */
	QStringList
	extract_pathnames_from_file_urls(
			const QList<QUrl> &urls)
	{
		QStringList pathnames;
		Q_FOREACH(const QUrl &url, urls) {
			if (url.scheme() == "file") {
				pathnames.push_back(url.toLocalFile());
			}
		}
		return pathnames;	// QStringList implicitly shares data and uses pimpl idiom, return by value is fine.
	}
}


GPlatesAppLogic::FeatureCollectionFileIO::FeatureCollectionFileIO(
		GPlatesModel::ModelInterface &model,
		GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
		GPlatesAppLogic::FeatureCollectionFileState &file_state) :
	d_model(model),
	d_file_format_registry(file_format_registry),
	d_file_state(file_state)
{
}


void
GPlatesAppLogic::FeatureCollectionFileIO::load_files(
		const QStringList &filenames)
{
	// We want to merge model events across this scope so that only one model event
	// is generated instead of many in case we incrementally modify the features below.
	// Probably won't be modifying the model so much when loading but we should keep this anyway.
	GPlatesModel::NotificationGuard model_notification_guard(d_model.access_model());

	// Read all the files before we add them to the application state.
	file_seq_type loaded_files = read_feature_collections(filenames);

	// Add files to the application state in one call.
	//
	// NOTE: It is important to load multiple files in one group here rather than
	// reuse load_file() for each file because the file state will then send
	// only one notification (instead of multiple notifications) which is needed in
	// some cases where the files in the group depend on each other.
	d_file_state.add_files(loaded_files);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::load_file(
		const QString &filename)
{
	// We want to merge model events across this scope so that only one model event
	// is generated instead of many in case we incrementally modify the features below.
	// Probably won't be modifying the model so much when loading but we should keep this anyway.
	GPlatesModel::NotificationGuard model_notification_guard(d_model.access_model());

	const GPlatesFileIO::FileInfo file_info(filename);

	// Create a file with an empty feature collection.
	GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(file_info);

	// Read new features from the file into the empty feature collection.
	read_feature_collection(file->get_reference());

	d_file_state.add_file(file);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::load_urls(
		const QList<QUrl> &urls)
{
	// Transform file:// urls into pathnames; ignore any http:// etc urls.
	// Read all the files before we add them to the application state.
	QStringList filenames = extract_pathnames_from_file_urls(urls);
	if (filenames.isEmpty()) {
		return;
	}
	
	// Then proceed exactly as though we had called load_files(QStringList).
	load_files(filenames);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::reload_file(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	// We want the removal of all features from the feature collection and
	// subsequent addition of new features from the file loading code to
	// occupy a single changeset in the model.
	GPlatesModel::ChangesetHandle changeset(
			d_model.access_model(),
			"reload " + file.get_file().get_file_info().get_qfileinfo().fileName().toStdString());
	// Also want to merge model events across this scope.
	GPlatesModel::NotificationGuard model_notification_guard(d_model.access_model());

	//
	// By removing all features and then reading new features from the file
	// we get to keep the same feature collection handle which means we don't
	// need to notify clients that their feature collection weak ref no longer
	// points to the correct feature collection handle.
	//
	// This will register as a modification to the feature collection for any
	// model callbacks attached by client code.
	//

	// Remove all features from the feature collection first.
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			file.get_file().get_feature_collection();
	for (GPlatesModel::FeatureCollectionHandle::iterator feature_iter = feature_collection->begin();
		feature_iter != feature_collection->end();
		++feature_iter)
	{
		feature_collection->remove(feature_iter);
	}

	// Read new features from the file into the feature collection.
	read_feature_collection(file.get_file());
}


void
GPlatesAppLogic::FeatureCollectionFileIO::unload_file(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference loaded_file)
{
	// Remove the loaded file from the file state - also removes it from the model.
	d_file_state.remove_file(loaded_file);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::save_file(
		const GPlatesFileIO::FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// We want to merge model events across this scope so that only one model event
	// is generated instead of many in case we incrementally modify the features below.
	GPlatesModel::NotificationGuard model_notification_guard(d_model.access_model());

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
	
	d_file_format_registry.write_feature_collection(feature_collection, file_info);

	feature_collection->clear_unsaved_changes();
}


GPlatesAppLogic::FeatureCollectionFileState::file_reference
GPlatesAppLogic::FeatureCollectionFileIO::create_empty_file()
{
	// Create a file with an empty feature collection and no filename.
	GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file();

	return d_file_state.add_file(file);
}


GPlatesAppLogic::FeatureCollectionFileState::file_reference
GPlatesAppLogic::FeatureCollectionFileIO::create_file(
		const GPlatesFileIO::FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type &feature_collection)
{
	const GPlatesFileIO::File::non_null_ptr_type file =
			GPlatesFileIO::File::create_file(file_info, feature_collection);

	save_file(file_info, file->get_reference().get_feature_collection());

	return d_file_state.add_file(file);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::remap_shapefile_attributes(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref)
{
	GPlatesFileIO::ReadErrorAccumulation read_errors;
	GPlatesFileIO::ShapefileReader::remap_shapefile_attributes(
			file_ref.get_file(),
			d_model,
			read_errors);

	emit_handle_read_errors_signal(read_errors);

	emit remapped_shapefile_attributes(*this, file_ref);
}


void
GPlatesAppLogic::FeatureCollectionFileIO::load_xml_data(
		const QString& name,
		QByteArray &data)
{
	using namespace GPlatesFileIO;
	ReadErrorAccumulation read_errors;

	//create temp file
	QFile tmp_file(name);
	tmp_file.open(QIODevice::ReadWrite | QIODevice::Text);
	tmp_file.close();

	const FileInfo file_info(name);
	File::non_null_ptr_type file = File::create_file(file_info);

//qDebug() << "GPlatesAppLogic::FeatureCollectionFileIO::load_xml_data()";

	ArbitraryXmlReader::instance()->read_xml_data(
			file->get_reference(), 
			boost::shared_ptr<ArbitraryXmlProfile>(new GeoscimlProfile()), 
			d_model,
			data,
			read_errors);
	d_file_state.add_file(file);

	// Emit one signal for all loaded files.
	emit_handle_read_errors_signal(read_errors);
//qDebug() << "GPlatesAppLogic::FeatureCollectionFileIO::load_xml_data() END =========";
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

		const GPlatesFileIO::FileInfo file_info(filename);

		// Create a file with an empty feature collection.
		GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(file_info);

		// Read new features from the file into the feature collection.
		// Both the filename and target feature collection are in 'file'.
		d_file_format_registry.read_feature_collection(file->get_reference(), read_errors);

		// Files that have been freshly loaded from disk are by definition, clean.
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
				file->get_reference().get_feature_collection();
		if (feature_collection_ref.is_valid()) {
			feature_collection_ref->clear_unsaved_changes();
		}

		files.push_back(file);
	}

	// Emit one signal for all loaded files.
	emit_handle_read_errors_signal(read_errors);

	return files;
}


void
GPlatesAppLogic::FeatureCollectionFileIO::read_feature_collection(
		const GPlatesFileIO::File::Reference &file_ref)
{
	GPlatesFileIO::ReadErrorAccumulation read_errors;

	// Read new features from the file into the feature collection.
	// Both the filename and target feature collection are in 'file_ref'.
	d_file_format_registry.read_feature_collection(file_ref, read_errors);

	// Files that have been freshly loaded from disk are by definition, clean.
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
			file_ref.get_feature_collection();
	if (feature_collection_ref.is_valid()) {
		feature_collection_ref->clear_unsaved_changes();
	}

	emit_handle_read_errors_signal(read_errors);
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
