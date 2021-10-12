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
	 * Return the first suffix found in the filter string.
	 * 
	 * Looks for a string contained between "(*." and ")", and interprets this as
	 * the suffix. 
	 * 
	 * This is a bit naff but I can't find a way to automatically set the suffix if the 
	 * user hasn't explicitly provided one.  
	 */
	QString
	get_suffix_from_filter(
		const QString &filter)
	{
		QStringList string_list = filter.split("(*.");
		if (string_list.size() > 1)
		{
			QString second_string = string_list.at(1);
			QStringList suffix_list = second_string.split(")");
			if (!suffix_list.empty())
			{
				QString suffix_string = suffix_list.at(0);
				return suffix_string;
			}
			
		} 
		return QString();
	}

	QString
	get_output_filters()
	{
		static const QString filter_gmt(QObject::tr("GMT xy (*.xy)"));
		static const QString filter_shp(QObject::tr("ESRI Shapefile (*.shp)"));

		QStringList file_filters;
		file_filters << filter_gmt;
		file_filters << filter_shp;
		
		return file_filters.join(";;");
	}
}

GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::ExportReconstructedFeatureGeometryDialog(
	QWidget *parent_) :
d_export_file_dialog(new QFileDialog(
		parent_,
		QObject::tr("Select a filename for exporting"),
		QString(),
		get_output_filters()))
{
	d_export_file_dialog->setAcceptMode(QFileDialog::AcceptSave);

	// Set the default suffix to that contained in the initial filter string. 
	handle_filter_changed();

	QObject::connect(d_export_file_dialog, SIGNAL(filterSelected(const QString&)), 
		this, SLOT(handle_filter_changed()));
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
	if (!d_export_file_dialog->exec())
	{
		return;
	}

	QString filename = d_export_file_dialog->selectedFiles().front();

	if (filename.isEmpty())
	{
		return;
	}

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

void
GPlatesQtWidgets::ExportReconstructedFeatureGeometryDialog::handle_filter_changed()
{
	QString filter = d_export_file_dialog->selectedFilter();

	QString suffix_string = get_suffix_from_filter(filter);

	d_export_file_dialog->setDefaultSuffix(suffix_string);
}

