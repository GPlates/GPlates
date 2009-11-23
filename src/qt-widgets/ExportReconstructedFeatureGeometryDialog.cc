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

GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::ExportReconstructedFeatureGeometryDialog(
		QWidget *parent)
{
	// create filters for save dialog box
	std::vector<std::pair<QString, QString> > filters;
	filters.push_back(std::make_pair("GMT xy (*.xy)", "xy"));
	filters.push_back(std::make_pair("ESRI Shapefile (*.shp)", "shp"));

	d_save_file_dialog_ptr = SaveFileDialog::get_save_file_dialog(
				parent,
				"Select a filename for exporting",
				filters);
}

void
GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::export_visible_reconstructed_feature_geometries(
		const GPlatesModel::Reconstruction &reconstruction,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesViewOperations::VisibleReconstructedFeatureGeometryExport::
				files_collection_type &active_reconstructable_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	boost::optional<QString> filename = d_save_file_dialog_ptr->get_file_name();
	if (!filename)
	{
		return;
	}

	try
	{
		GPlatesViewOperations::VisibleReconstructedFeatureGeometryExport::export_visible_geometries(
				*filename,
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
