/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <QFile>
#include <QStringList>
#include <QString>
#include <QTextStream>

#include "GMTFormatResolvedTopologicalGeometryExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "GMTFormatGeometryExporter.h"
#include "GMTFormatHeader.h"

#include "app-logic/ResolvedTopologicalGeometry.h"

#include "file-io/FileInfo.h"

#include "utils/StringFormattingUtils.h"


namespace GPlatesFileIO
{
	namespace GMTFormatResolvedTopologicalGeometryExport
	{
		namespace
		{
			//! Convenience typedef for a sequence of RTGs.
			typedef std::vector<const GPlatesAppLogic::ResolvedTopologicalGeometry *>
					resolved_topological_geom_seq_type;


			/**
			 * Prints GMT format header at top of the exported file containing information
			 * about the reconstruction that is not per-feature information.
			 */
			void
			get_global_header_lines(
					std::vector<QString>& header_lines,
					const referenced_files_collection_type &referenced_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time)
			{
				// Print the anchor plate id.
				header_lines.push_back(
						QString("anchorPlateId ") + QString::number(reconstruction_anchor_plate_id));

				// Print the reconstruction time.
				header_lines.push_back(
						QString("reconstructionTime ") + QString::number(reconstruction_time));

				// Print the list of feature collection filenames that the exported
				// geometries came from.
				QStringList filenames;
				referenced_files_collection_type::const_iterator file_iter;
				for (file_iter = referenced_files.begin();
					file_iter != referenced_files.end();
					++file_iter)
				{
					const File::Reference *file = *file_iter;

					// Some files might not actually exist yet if the user created a new
					// feature collection internally and hasn't saved it to file yet.
					if (!GPlatesFileIO::file_exists(file->get_file_info()))
					{
						continue;
					}

					filenames << file->get_file_info().get_display_name(false/*use_absolute_path_name*/);
				}

				header_lines.push_back(filenames.join(" "));
			}
		}
	}
}


void
GPlatesFileIO::GMTFormatResolvedTopologicalGeometryExport::export_geometries(
		const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	// Open the file.
	QFile output_file(file_info.filePath());
	if ( ! output_file.open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			file_info.filePath());
	}

	QTextStream output_stream(&output_file);

	// Does the actual printing of GMT header to the output stream.
	GMTHeaderPrinter gmt_header_printer;

	// Write out the global header (at the top of the exported file).
	std::vector<QString> global_header_lines;
	get_global_header_lines(global_header_lines,
			referenced_files, reconstruction_anchor_plate_id, reconstruction_time);
	gmt_header_printer.print_global_header_lines(output_stream, global_header_lines);

	// Used to write the reconstructed geometry in GMT format.
	GMTFormatGeometryExporter geom_exporter(output_stream);

	// Even though we're printing out resolved geometry rather than present day geometry we still
	// write out the verbose properties of the feature.
	GMTFormatVerboseHeader gmt_header;

	// Iterate through the resolved geometries and write to output.
	std::list<feature_geometry_group_type>::const_iterator feature_iter;
	for (feature_iter = feature_geometry_group_seq.begin();
		feature_iter != feature_geometry_group_seq.end();
		++feature_iter)
	{
		const feature_geometry_group_type &feature_geom_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
				feature_geom_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Get the header lines.
		std::vector<QString> header_lines;
		gmt_header.get_feature_header_lines(feature_ref, header_lines);

		// Iterate through the resolved geometries of the current feature and write to output.
		resolved_topological_geom_seq_type::const_iterator rtg_iter;
		for (rtg_iter = feature_geom_group.recon_geoms.begin();
			rtg_iter != feature_geom_group.recon_geoms.end();
			++rtg_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalGeometry *rtg = *rtg_iter;

			// Print the header lines.
			gmt_header_printer.print_feature_header_lines(output_stream, header_lines);

			// Write the resolved geometry.
			geom_exporter.export_geometry(rtg->resolved_topology_geometry()); 
		}
	}
}
