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

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTopologicalSection.h"

#include "file-io/FileInfo.h"

#include "utils/StringFormattingUtils.h"


namespace GPlatesFileIO
{
	namespace GMTFormatResolvedTopologicalGeometryExport
	{
		namespace
		{
			//! Convenience typedef for a sequence of RTGs.
			typedef std::vector<const GPlatesAppLogic::ReconstructionGeometry *> resolved_topologies_seq_type;


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

				GPlatesFileIO::GMTFormatHeader::add_filenames_to_header(header_lines,referenced_files);
				GPlatesFileIO::GMTFormatHeader::add_filenames_to_header(header_lines,active_reconstruction_files);
			}
		}
	}
}


void
GPlatesFileIO::GMTFormatResolvedTopologicalGeometryExport::export_resolved_topological_geometries(
		const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const referenced_files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation)
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
		resolved_topologies_seq_type::const_iterator rt_iter;
		for (rt_iter = feature_geom_group.recon_geoms.begin();
			rt_iter != feature_geom_group.recon_geoms.end();
			++rt_iter)
		{
			const GPlatesAppLogic::ReconstructionGeometry *rt = *rt_iter;

			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> resolved_topology_geometry =
					GPlatesAppLogic::ReconstructionGeometryUtils::get_resolved_topological_boundary_or_line_geometry(rt);
			if (!resolved_topology_geometry)
			{
				continue;
			}

			// Print the header lines.
			gmt_header_printer.print_feature_header_lines(output_stream, header_lines);

			// Orient polygon if forcing orientation and geometry is a polygon.
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type resolved_geometry =
					force_polygon_orientation
					? GPlatesAppLogic::GeometryUtils::convert_geometry_to_oriented_geometry(
							resolved_topology_geometry.get(),
							force_polygon_orientation.get())
					: resolved_topology_geometry.get();

			// Write the resolved geometry.
			geom_exporter.export_geometry(resolved_geometry);
		}
	}
}


void
GPlatesFileIO::GMTFormatResolvedTopologicalGeometryExport::export_resolved_topological_sections(
		const std::vector<const GPlatesAppLogic::ResolvedTopologicalSection *> &resolved_topological_sections,
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

	// Even though we're printing out resolved geometry rather than present day geometry we still
	// write out the verbose properties of the feature.
	GMTFormatVerboseHeader gmt_header;

	// Iterate through the resolved topological section sub-segments and write to output.
	std::vector<const GPlatesAppLogic::ResolvedTopologicalSection *>::const_iterator sections_iter;
	for (sections_iter = resolved_topological_sections.begin();
		sections_iter != resolved_topological_sections.end();
		++sections_iter)
	{
		const GPlatesAppLogic::ResolvedTopologicalSection *section = *sections_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref = section->get_feature_ref();
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Get the header lines.
		std::vector<QString> header_lines;
		gmt_header.get_feature_header_lines(feature_ref, header_lines);

		// Iterate through the sub-segments of the current section.
		const std::vector<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment> &sub_segments =
				section->get_shared_sub_segments();
		std::vector<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment>::const_iterator sub_segments_iter;
		for (sub_segments_iter = sub_segments.begin(); sub_segments_iter != sub_segments.end(); ++sub_segments_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &sub_segment = *sub_segments_iter;

			// Print the header lines.
			gmt_header_printer.print_feature_header_lines(output_stream, header_lines);

			// Write the sub-segment geometry.
			geom_exporter.export_geometry(sub_segment.get_geometry());
		}
	}
}
