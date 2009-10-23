/* $Id$ */

/**
 * \file Dialog for handling exporting of reconstruction.
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
#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QStringList>

#include "ExportReconstructedFeatureGeometryDialog.h"

#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "view-operations/VisibleReconstructedFeatureGeometryExport.h"


namespace
{
	/**
	 * Returns the file extension from a @a filename, or the empty string if
	 * there is no file extension.
	 */
	QString
	get_file_extension(
		const QString &filename)
	{
		QStringList parts = filename.split(".");
		if (parts.size() > 1)
		{
			return parts.at(parts.size() - 1);
		}
		else
		{
			return QString();
		}
	}

	QString
	get_output_filters()
	{
		static const QString filter_gmt(QObject::tr(
			GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::FILTER_GMT));
		static const QString filter_shp(QObject::tr(
			GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::FILTER_SHP));

		QStringList file_filters;
		file_filters << filter_gmt;
		file_filters << filter_shp;
		
		return file_filters.join(";;");
	}
}

GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::ExportReconstructedFeatureGeometryDialog(
	QWidget *parent_) :
d_parent_ptr(parent_)
{
}

const char *GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::FILTER_GMT = "GMT xy (*.xy)";
const char *GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::FILTER_SHP = "ESRI Shapefile (*.shp)";

void
GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::export_visible_reconstructed_feature_geometries(
		const GPlatesModel::Reconstruction &reconstruction,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesViewOperations::VisibleReconstructedFeatureGeometryExport::
				files_collection_type &active_reconstructable_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	QString file_ext = get_file_extension(d_last_file_name);
	QString selected_filter;
	if (file_ext == "xy")
	{
		selected_filter = FILTER_GMT;
	}
	else if (file_ext == "shp")
	{
		selected_filter = FILTER_SHP;
	}
	QString filename = QFileDialog::getSaveFileName(
		d_parent_ptr,
		"Select a filename for exporting",
		d_last_file_name,
		get_output_filters(),
		&selected_filter);

	if (filename.isEmpty())
	{
		return;
	}
	d_last_file_name = filename;

	try
	{
		GPlatesViewOperations::VisibleReconstructedFeatureGeometryExport::export_visible_geometries(
				filename,
				rendered_geom_collection,
				active_reconstructable_files,
				reconstruction_anchor_plate_id,
				reconstruction_time);
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &e)
	{
		QString message = QObject::tr("An error occurred while saving the file '%1'")
				.arg(e.filename());
		QMessageBox::critical(NULL, QObject::tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
				
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = QObject::tr("Error: Writing files in this format is currently not supported.");
		QMessageBox::critical(NULL, QObject::tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
}
