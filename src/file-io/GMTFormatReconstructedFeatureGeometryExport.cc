/* $Id$ */

/**
 * \file Exports reconstructed feature geometries to a GMT format file.
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

#include <QFile>
#include <QStringList>
#include <QString>
#include <QTextStream>

#include "GMTFormatReconstructedFeatureGeometryExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "GMTFormatGeometryExporter.h"
#include "GMTFormatHeader.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "file-io/FileInfo.h"

#include "utils/StringFormattingUtils.h"


namespace GPlatesFileIO
{
	namespace GMTFormatReconstructedFeatureGeometryExport
	{
		namespace
		{
			//! Convenience typedef for a sequence of RFGs.
			typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *>
					reconstructed_feature_geom_seq_type;

			/*!
			 * Returns true if the feature-type of @a feature_ref is either
			 * flowline or motion path.
			 */
			bool
			feature_is_of_type_to_exclude(
					const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
			{
				static const GPlatesModel::FeatureType flowline_feature_type =
						GPlatesModel::FeatureType::create_gpml("Flowline");
				static const GPlatesModel::FeatureType motion_path_feature_type =
						GPlatesModel::FeatureType::create_gpml("MotionPath");

				if ((feature_ref->feature_type() == flowline_feature_type) ||
						(feature_ref->feature_type() == motion_path_feature_type))
				{
					return true;
				}
				return false;
			}


			/**
			 * Prints GMT format header at top of the exported file containing information
			 * about the reconstruction that is not per-feature information.
			 */
			void
			get_global_header_lines(
					std::vector<QString>& header_lines,
					const referenced_files_collection_type &referenced_files,
					const referenced_files_collection_type &active_reconstruction_files,
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
				// geometries came from, and the active reconstruction files.
				GPlatesFileIO::GMTFormatHeader::add_filenames_to_header(header_lines,referenced_files);
				GPlatesFileIO::GMTFormatHeader::add_filenames_to_header(header_lines,active_reconstruction_files);

			}
		}
	}
}


void
GPlatesFileIO::GMTFormatReconstructedFeatureGeometryExport::export_geometries(
		const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const referenced_files_collection_type &active_reconstruction_files,
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
			referenced_files, active_reconstruction_files,
			reconstruction_anchor_plate_id, reconstruction_time);
	gmt_header_printer.print_global_header_lines(output_stream, global_header_lines);

	// Used to write the reconstructed geometry in GMT format.
	GMTFormatGeometryExporter geom_exporter(output_stream);

	// Even though we're printing out reconstructed geometry rather than
	// present day geometry we still write out the verbose properties
	// of the feature (including the properties used to reconstruct
	// the geometries).
	GMTFormatVerboseHeader gmt_header;

	// Iterate through the reconstructed geometries and write to output.
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

		// Prevents us from exporting flowline/motion-path seed points.
		if (feature_is_of_type_to_exclude(feature_ref))
		{
			continue;
		}

		// Get the header lines.
		std::vector<QString> header_lines;
		gmt_header.get_feature_header_lines(feature_ref, header_lines);

		// Iterate through the reconstructed geometries of the current feature and write to output.
		reconstructed_feature_geom_seq_type::const_iterator rfg_iter;
		for (rfg_iter = feature_geom_group.recon_geoms.begin();
			rfg_iter != feature_geom_group.recon_geoms.end();
			++rfg_iter)
		{
			const GPlatesAppLogic::ReconstructedFeatureGeometry *rfg = *rfg_iter;

			// Print the header lines.
			gmt_header_printer.print_feature_header_lines(output_stream, header_lines);

			// Write the reconstructed geometry.
			geom_exporter.export_geometry(rfg->reconstructed_geometry()); 
		}
	}
}
