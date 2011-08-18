/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_MANAGEFEATURECOLLECTIONSEDITCONFIGURATIONS_H
#define GPLATES_QT_WIDGETS_MANAGEFEATURECOLLECTIONSEDITCONFIGURATIONS_H

#include <boost/shared_ptr.hpp>
#include <QWidget>

#include "app-logic/FeatureCollectionFileState.h"

#include "file-io/FeatureCollectionFileFormatConfigurations.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"


namespace GPlatesQtWidgets
{
	class ManageFeatureCollectionsDialog;

	namespace ManageFeatureCollections
	{
		/**
		 * Base class for editing a file's configuration in the Manage Feature Collection dialog.
		 */
		class EditConfiguration
		{
		public:
			//! Typedef for a shared pointer to const @a EditConfiguration.
			typedef boost::shared_ptr<const EditConfiguration> shared_ptr_to_const_type;
			//! Typedef for a shared pointer to @a EditConfiguration.
			typedef boost::shared_ptr<EditConfiguration> shared_ptr_type;

			virtual
			~EditConfiguration()
			{  }

			/**
			 * Allow the user to edit @a current_configuration.
			 *
			 * The edited configuration is returned (or @a current_configuration if it wasn't edited).
			 *
			 * NOTE: The original @a current_configuration should be returned if its derived type
			 * is not what was expected - this can happen when files are saved to different formats.
			 */
			virtual
			GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
			edit_configuration(
					GPlatesFileIO::File::Reference &file_reference,
					const GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type &
							current_configuration,
					QWidget *parent_widget) = 0;
		};


		/**
		 * Registers the default edit configurations for those file formats that have configurations.
		 */
		void
		register_default_edit_configurations(
				ManageFeatureCollectionsDialog &manage_feature_collections_dialog,
				GPlatesModel::ModelInterface &model);


		/**
		 * Handles output options when writing to the write-only GMT ".xy" file format.
		 */
		class GMTEditConfiguration :
				public EditConfiguration
		{
		public:
			typedef boost::shared_ptr<const GMTEditConfiguration> shared_ptr_to_const_type;
			typedef boost::shared_ptr<GMTEditConfiguration> shared_ptr_type;

			virtual
			GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
			edit_configuration(
					GPlatesFileIO::File::Reference &file_reference,
					const GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type &
							current_configuration,
					QWidget *parent_widget);
		};


		/**
		 * Handles input/output options for the Shapefile format.
		 */
		class ShapefileEditConfiguration :
				public EditConfiguration
		{
		public:
			typedef boost::shared_ptr<const ShapefileEditConfiguration> shared_ptr_to_const_type;
			typedef boost::shared_ptr<ShapefileEditConfiguration> shared_ptr_type;

			explicit
			ShapefileEditConfiguration(
					GPlatesModel::ModelInterface &model) :
				d_model(model)
			{  }

			virtual
			GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
			edit_configuration(
					GPlatesFileIO::File::Reference &file_reference,
					const GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type &
							current_configuration,
					QWidget *parent_widget);

		private:
			GPlatesModel::ModelInterface d_model;
		};
	}
}

#endif // GPLATES_QT_WIDGETS_MANAGEFEATURECOLLECTIONSEDITCONFIGURATIONS_H
