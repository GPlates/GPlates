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

#ifndef GPLATES_APP_LOGIC_FEATURECOLLECTIONFILEIO_H
#define GPLATES_APP_LOGIC_FEATURECOLLECTIONFILEIO_H

#include <list>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QList>
#include <QtGlobal>

#include "app-logic/FeatureCollectionFileState.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"

#include "model/ModelInterface.h"
#include "model/FeatureCollectionHandle.h"


namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class Registry;
	}

	struct ReadErrorAccumulation;
}

namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesAppLogic
{
	/**
	 * Handles feature collection file loading/saving.
	 *
	 * Loaded files are then added to @a FeatureCollectionFileState for access
	 * by other objects in the application.
	 */
	class FeatureCollectionFileIO :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT


	public:
		FeatureCollectionFileIO(
				const GPlatesModel::Gpgim &gpgim,
				GPlatesModel::ModelInterface &model,
				GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
				GPlatesAppLogic::FeatureCollectionFileState &file_state);


		/**
		 * Loads feature collections from multiple files named @a file_names and adds them
		 * to the application state.
		 *
		 * NOTE: if you are loading multiple files in one group then use this method
		 * instead of multiple calls to @a load_file so that the application state
		 * sends one notification instead of multiple notifications which is needed in
		 * some cases where the files in the group depend on each other - an example is
		 * topological boundary features which get resolved after the notification and
		 * require any referenced features to be loaded into the model (and they might
		 * be in other files in the group).
		 */
		void
		load_files(
				const QStringList &file_names);


		/**
		 * Loads a feature collection from the file names @a file_name and adds it
		 * to the application state.
		 *
		 * NOTE: if you are loading multiple files in one group then use @a load_files
		 * instead so that the application state sends one notification instead of
		 * multiple notifications (for each @a load_file) which is beneficial
		 * if some files in the group depend on each other.
		 *
		 * The file is read using the default file configuration options for its file format
		 * as currently set at GPlatesFileIO::FeatureCollectionFileFormat::Registry.
		 */
		void
		load_file(
				const QString &filename);


		/**
		 * As @a load_files, but for QUrl instances of file:// urls.
		 * Included for drag and drop support.
		 *
		 * The file is read using the default file configuration options for its file format
		 * as currently set at GPlatesFileIO::FeatureCollectionFileFormat::Registry.
		 */
		void
		load_urls(
				const QList<QUrl> &urls);


		/**
		 * Given a file_reference, reloads the data for that file from disk,
		 * replacing the feature collection associated with that file_reference in
		 * the application state.
		 */
		void
		reload_file(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);


		/**
		 * This method simply delegates to @a FeatureCollectionFileState and removes the file from it.
		 */
		void
		unload_file(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);


		/**
		 * Creates a fresh, empty, FeatureCollection. Associates a 'dummy'
		 * FileInfo for it, and registers it with FeatureCollectionFileState.
		 */
		GPlatesAppLogic::FeatureCollectionFileState::file_reference
		create_empty_file();


		/**
		 * Optionally saves the feature collection in @a file to the filename in @a file, and
		 * registers the file with FeatureCollectionFileState.
		 *
		 * This method is useful when you want to save a feature collection that was not
		 * originally loaded from a file - and you want the new file to appear in the
		 * list of loaded files maintained by FeatureCollectionFileState.
		 */
		GPlatesAppLogic::FeatureCollectionFileState::file_reference
		create_file(
				const GPlatesFileIO::File::non_null_ptr_type &file,
				bool save = true);


		/**
		 * Write the feature collection in @a file_ref to the filename in @a file_ref.
		 *
		 * NOTE: this differs from @a create_file in that it only saves the feature collection
		 * to the file and doesn't register with FeatureCollectionFileState.
		 *
		 * NOTE: @a clear_unsaved_changes can be set to false when saving a *copy* of a
		 * feature collection - that is the original file has not been saved and so it still
		 * has unsaved changes.
		 */
		void
		save_file(
				GPlatesFileIO::File::Reference &file_ref,
				bool clear_unsaved_changes = true);


		/*
		* Returns the number of features in the xml data @a data;
		*/
		int 
		count_features_in_xml_data(
				QByteArray &data);

		/*
		* Load xml data in QByteArray.
		*/
		void
		load_xml_data(
				const QString& filename,
				QByteArray &data);

	Q_SIGNALS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_read_errors(
				GPlatesAppLogic::FeatureCollectionFileIO &,
				const GPlatesFileIO::ReadErrorAccumulation &read_errors);

	private:
		//! Typedef for a sequence of file shared refs.
		typedef std::vector<GPlatesFileIO::File::non_null_ptr_type> file_seq_type;


		const GPlatesModel::Gpgim &d_gpgim;

		GPlatesModel::ModelInterface d_model;

		/**
		 * A registry of the file formats for reading/writing feature collections.
		 */
		GPlatesFileIO::FeatureCollectionFileFormat::Registry &d_file_format_registry;

		/**
		 * The loaded feature collection files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState &d_file_state;


		file_seq_type
		read_feature_collections(
				const QStringList &filenames);

		/**
		 * Read new features from file into @a file_ref.
		 */
		void
		read_feature_collection(
				GPlatesFileIO::File::Reference &file_ref);

		void
		emit_handle_read_errors_signal(
				const GPlatesFileIO::ReadErrorAccumulation &read_errors);
	};
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONFILEIO_H
