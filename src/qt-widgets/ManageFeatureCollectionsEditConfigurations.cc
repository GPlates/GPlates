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
#include "ShapefileAttributeRemapperDialog.h"

#include "file-io/ShapefileReader.h"


void
GPlatesQtWidgets::ManageFeatureCollections::register_default_edit_configurations(
		ManageFeatureCollectionsDialog &manage_feature_collections_dialog,
		GPlatesModel::ModelInterface &model)
{
	manage_feature_collections_dialog.register_edit_configuration(
			GPlatesFileIO::FeatureCollectionFileFormat::WRITE_ONLY_XY_GMT,
			GMTEditConfiguration::shared_ptr_type(new GMTEditConfiguration()));

	manage_feature_collections_dialog.register_edit_configuration(
			GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE,
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
				current_configuration,
		QWidget *parent_widget)
{
	// Copy cast to the correct derived configuration type.
	boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_type>
			current_ogr_configuration =
					GPlatesFileIO::FeatureCollectionFileFormat::copy_cast_configuration<
							GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration>(
									current_configuration);
	// If, for some reason, we were passed the wrong derived type then just return the configuration passed to us.
	if (!current_ogr_configuration)
	{
		return current_configuration;
	}

	// It's possible that another file format is 'save as' a shapefile but the shapefile has
	// more than one layer - in this case the shapefile writer writes out multiple shapefiles
	// (eg, <filename>_point.shp and <filename>_polyline.shp)
	if (!GPlatesFileIO::file_exists(file.get_file_info()))
	{
		QString message =
				QObject::tr("Error: File '%1' does not exist: \nUnable to edit its configuration.")
						.arg(file.get_file_info().get_display_name(false/*use_absolute_path_name*/));
		QMessageBox::critical(
				parent_widget, QObject::tr("Error Opening File"), message, QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.

		return current_configuration;
	}

	// FIXME: These errors should get reported somehow.
	GPlatesFileIO::ReadErrorAccumulation read_errors;

	const QString filename = file.get_file_info().get_qfileinfo().fileName();
	const QStringList field_names =
			GPlatesFileIO::ShapefileReader::read_field_names(file, d_model, read_errors);

	// This is the model-to-attribute map that will be modified.
	// NOTE: We're modifying the new file configuration in-place.
	QMap<QString,QString> &model_to_attribute_map =
			current_ogr_configuration.get()->get_model_to_attribute_map();

	ShapefileAttributeRemapperDialog dialog(parent_widget);
	dialog.setup(filename, field_names, model_to_attribute_map);
	dialog.exec();

	// If the user cancelled then just return the configuration passed to us.
	if (dialog.result() == QDialog::Rejected)
	{
		return current_configuration;
	}

	// Store the (potentially) updated file configuration back in the file.
	// We need to do this here before we remap the model with the updated attributes because
	// 'ShapefileReader::remap_shapefile_attributes' looks at the file configuration on the file reference.
	GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
			file_configuration = current_ogr_configuration.get();
	file.set_file_info(file.get_file_info(), file_configuration);

	// Remap the model with the updated attributes.
	GPlatesFileIO::ShapefileReader::remap_shapefile_attributes(file, d_model, read_errors);

	return file_configuration;
}
