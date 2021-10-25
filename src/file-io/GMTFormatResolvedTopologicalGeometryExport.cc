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

		std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> shared_sub_segment_geometries;

		// Iterate through the shared sub-segments of the current section.
		const GPlatesAppLogic::shared_sub_segment_seq_type &shared_sub_segments = section->get_shared_sub_segments();
		GPlatesAppLogic::shared_sub_segment_seq_type::const_iterator shared_sub_segments_iter;
		for (shared_sub_segments_iter = shared_sub_segments.begin();
			shared_sub_segments_iter != shared_sub_segments.end();
			++shared_sub_segments_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::non_null_ptr_type &
					shared_sub_segment = *shared_sub_segments_iter;

			// If the shared sub-segment has any of its own child sub-segments in turn
			// (because it's from a resolved topological line) then process those instead.
			// This essentially is the same as simply using the parent sub-segment except that the plate IDs will
			// come from the child sub-segment features (which is more representative of the reconstructed geometry.
			const boost::optional<GPlatesAppLogic::sub_segment_seq_type> &sub_sub_segments =
					shared_sub_segment->get_sub_sub_segments();
			if (sub_sub_segments)
			{
				// Visit each sub-sub-segment geometry.
				GPlatesAppLogic::sub_segment_seq_type::const_iterator sub_sub_segments_iter = sub_sub_segments->begin();
				GPlatesAppLogic::sub_segment_seq_type::const_iterator sub_sub_segments_end = sub_sub_segments->end();
				for (; sub_sub_segments_iter != sub_sub_segments_end; ++sub_sub_segments_iter)
				{
					const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &
							sub_sub_segment = *sub_sub_segments_iter;

					const GPlatesModel::FeatureHandle::const_weak_ref &sub_sub_segment_feature_ref =
							sub_sub_segment->get_feature_ref();
					if (!sub_sub_segment_feature_ref.is_valid())
					{
						continue;
					}

					//
					// Each (child) sub-sub-segment potentially belongs to a different feature
					// (unlike the parent sub-segments) and hence needs its own header.
					//

					// Get the header lines.
					std::vector<QString> header_lines;
					gmt_header.get_feature_header_lines(sub_sub_segment_feature_ref, header_lines);

					// Print the header lines.
					gmt_header_printer.print_feature_header_lines(output_stream, header_lines);

					// Write (child) sub-sub-segment geometries out immediately (since each has its own header).
					geom_exporter.export_geometry(sub_sub_segment->get_sub_segment_geometry());
				}
			}
			else
			{
				// Wait and write all shared (parent) sub-segment geometries together as a single feature (with same header).
				shared_sub_segment_geometries.push_back(shared_sub_segment->get_shared_sub_segment_geometry());
			}
		}

		// Write the shared sub-segment geometries as a single feature since these shared (parent) sub-segments
		// all come from the same topological section feature (and hence have same header).
		if (!shared_sub_segment_geometries.empty())
		{
			// Get the header lines.
			std::vector<QString> header_lines;
			gmt_header.get_feature_header_lines(feature_ref, header_lines);

			// Print the header lines.
			gmt_header_printer.print_feature_header_lines(output_stream, header_lines);

			// Write the shared sub-segment geometries.
			const unsigned int num_shared_sub_segment_geometries = shared_sub_segment_geometries.size();
			for (unsigned int n = 0; n < num_shared_sub_segment_geometries; ++n)
			{
				geom_exporter.export_geometry(shared_sub_segment_geometries[n]);
			}
		}
	}
}
