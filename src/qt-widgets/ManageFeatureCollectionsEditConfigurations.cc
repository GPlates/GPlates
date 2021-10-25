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

#include <QDebug>
#include <QMessageBox>

#include "ManageFeatureCollectionsEditConfigurations.h"

#include "GMTFileFormatConfigurationDialog.h"
#include "ManageFeatureCollectionsDialog.h"
#include "ShapefileFileFormatConfigurationDialog.h"

#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/OgrReader.h"


void
GPlatesQtWidgets::ManageFeatureCollections::register_default_edit_configurations(
		ManageFeatureCollectionsDialog &manage_feature_collections_dialog,
		GPlatesModel::ModelInterface &model)
{
	manage_feature_collections_dialog.register_edit_configuration(
			GPlatesFileIO::FeatureCollectionFileFormat::WRITE_ONLY_XY_GMT,
			GMTEditConfiguration::shared_ptr_type(new GMTEditConfiguration()));

	// TODO: Other OGR-supported formats have attributes and are mapped to the model in the same way
	// as shapefile attributes. So:
	// * variable/class names should be changed to reflect this (e.g. OgrEditConfiguration)
	// * the mapping/re-mapping UI's labels, titles etc (for attribute-mapping) should reflect this (currently it says "Shapefile blah" everywhere).
	manage_feature_collections_dialog.register_edit_configuration(
			GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE,
			ShapefileEditConfiguration::shared_ptr_type(new ShapefileEditConfiguration(model)));

	manage_feature_collections_dialog.register_edit_configuration(
				GPlatesFileIO::FeatureCollectionFileFormat::OGRGMT,
				ShapefileEditConfiguration::shared_ptr_type(new ShapefileEditConfiguration(model)));

	manage_feature_collections_dialog.register_edit_configuration(
				GPlatesFileIO::FeatureCollectionFileFormat::GEOJSON,
				ShapefileEditConfiguration::shared_ptr_type(new ShapefileEditConfiguration(model)));
}


GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
GPlatesQtWidgets::ManageFeatureCollections::GMTEditConfiguration::edit_configuration(
		GPlatesFileIO::File::Reference &file_reference,
		const GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type &
				current_configuration,
		QWidget *parent_widget)
{
	// Cast to the correct derived configuration type.
	boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_to_const_type>
			current_gmt_configuration =
					GPlatesFileIO::FeatureCollectionFileFormat::dynamic_cast_configuration<
							const GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration>(
									current_configuration);
	// If, for some reason, we were passed the wrong derived type then just return the configuration passed to us.
	if (!current_gmt_configuration)
	{
		return current_configuration;
	}

	GMTFileFormatConfigurationDialog dialog(current_gmt_configuration.get(), parent_widget);
	dialog.exec();

	return dialog.get_configuration();
}


GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
GPlatesQtWidgets::ManageFeatureCollections::ShapefileEditConfiguration::edit_configuration(
		GPlatesFileIO::File::Reference &file,
		const GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type &
				original_configuration,
		QWidget *parent_widget)
{
	// Copy cast to the correct derived configuration type.
	boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_type>
			current_ogr_configuration =
					GPlatesFileIO::FeatureCollectionFileFormat::copy_cast_configuration<
							GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration>(
									original_configuration);
	// If, for some reason, we were passed the wrong derived type then just return the configuration passed to us.
	if (!current_ogr_configuration)
	{
		return original_configuration;
	}

	bool wrap_to_dateline = current_ogr_configuration.get()->get_wrap_to_dateline();

	// FIXME: These errors should get reported somehow.
	GPlatesFileIO::ReadErrorAccumulation read_errors;

	// It's possible that another file format is 'save as' a shapefile but the shapefile has
	// more than one layer - in this case the shapefile writer writes out multiple shapefiles
	// (eg, <filename>_point.shp and <filename>_polyline.shp).
	//
	// NOTE: Not using an early test with 'GPlatesFileIO::file_exists(file.get_file_info())' because
	// that uses 'QFileInfo::exists()' which uses a cached result and it's still possible someone
	// could delete the Shapefile in the file system and then click 'Edit Configuration' to get here.
	try
	{
		const QString filename = file.get_file_info().get_qfileinfo().fileName();

		const QStringList field_names =
				GPlatesFileIO::OgrReader::read_field_names(file, d_model, read_errors);

		// This is the model-to-attribute map that will be modified.
		// NOTE: We're modifying the new file configuration in-place.
		QMap<QString,QString> &model_to_attribute_map =
				current_ogr_configuration.get()->get_model_to_attribute_map();

		ShapefileFileFormatConfigurationDialog dialog(parent_widget);
		dialog.setup(wrap_to_dateline, filename, field_names, model_to_attribute_map);
		dialog.exec();

		// If the user cancelled then just return the configuration passed to us.
		if (dialog.result() == QDialog::Rejected)
		{
			return original_configuration;
		}

		// Get the updated wrap-to-dateline option.
		wrap_to_dateline = dialog.get_wrap_to_dateline();

		// Store the updated wrap-to-dateline flag.
		current_ogr_configuration.get()->set_wrap_to_dateline(wrap_to_dateline);

		// Store the (potentially) updated file configuration back in the file.
		// We need to do this here before we remap the model with the updated attributes because
		// 'ShapefileReader::remap_shapefile_attributes' looks at the file configuration on the file reference.
		GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
				file_configuration = current_ogr_configuration.get();
		file.set_file_info(file.get_file_info(), file_configuration);

		// Remap the model with the updated attributes.
		GPlatesFileIO::OgrReader::remap_shapefile_attributes(file, d_model, read_errors);
	}
	catch (GPlatesFileIO::ErrorOpeningFileForReadingException &exc)
	{
		QString message =
				QObject::tr("Error: File '%1' does not exist: \nUnable to edit its configuration.")
						.arg(exc.filename());
		QMessageBox::critical(
				parent_widget, QObject::tr("Error Opening File"), message, QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.

		// Just return the original configuration passed to us.
		return original_configuration;
	}

	return current_ogr_configuration.get();
}
