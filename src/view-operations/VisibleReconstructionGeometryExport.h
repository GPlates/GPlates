/* $Id$ */

/**
 * \file Exports visible reconstructed feature geometries to a file.
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

#ifndef GPLATES_VIEWOPERATIONS_VISIBLERECONSTRUCTIONGEOMETRYEXPORT_H
#define GPLATES_VIEWOPERATIONS_VISIBLERECONSTRUCTIONGEOMETRYEXPORT_H

#include <vector>
#include <boost/optional.hpp>
#include <QDir>
#include <QString>

#include "file-io/File.h"

#include "maths/PolygonOrientation.h"

#include "model/types.h"

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class Registry;
	}
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;

	namespace VisibleReconstructionGeometryExport
	{
		//! Typedef for sequence of feature collection files.
		typedef std::vector<const GPlatesFileIO::File::Reference *> files_collection_type;


		/**
		 * Collects visible @a ReconstructedFeatureGeometry objects that are displayed
		 * using @a rendered_geom_collection and exports to a file depending on the
		 * file extension of @a filename.
		 *
		 * @param active_files used to determine which files the RFGs came from.
		 * @param active_reconstruction_files  the loaded and active reconstruction files in the reconstruction graph.
		 * @param reconstruction_anchor_plate_id the anchor plate id used in the reconstruction.
		 * @param reconstruction_time time at which the reconstruction took place.
		 * @param export_single_output_file write all geometries to a single file.
		 * @param export_per_input_file write output files corresponding to input files.
		 * @param export_separate_output_directory_per_input_file save each file to a different directory.
		 * @param wrap_to_dateline if true then exported geometries are wrapped/clipped to the dateline.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_visible_reconstructed_feature_geometries(
				const QString &filename,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
				const files_collection_type &active_files,
				const files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file,
				bool wrap_to_dateline);


		/**
		 * Collects visible @a ReconstructedFeatureGeometry objects that are displayed
		 * using @a rendered_geom_collection and exports to a file depending on the
		 * file extension of @a filename.
		 *
		 * @param active_files used to determine which files the RFGs came from.
		 * @param active_reconstruction_files  the loaded and active reconstruction files in the reconstruction graph.
		 * @param reconstruction_anchor_plate_id the anchor plate id used in the reconstruction.
		 * @param reconstruction_time time at which the reconstruction took place.
		 * @param export_single_output_file write all geometries to a single file.
		 * @param export_per_input_file write output files corresponding to input files.
		 * @param export_separate_output_directory_per_input_file save each file to a different directory.
		 * @param wrap_to_dateline if true then exported geometries are wrapped/clipped to the dateline.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_visible_reconstructed_flowlines(
				const QString &filename,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
				const files_collection_type &active_files,
				const files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file,
				bool wrap_to_dateline);


		/**
		 * Collects visible @a ReconstructedMotionPath objects that are displayed
		 * using @a rendered_geom_collection and exports to a file depending on the
		 * file extension of @a filename.
		 *
		 * @param active_files used to determine which files the RFGs came from.
		 * @param active_reconstruction_files  the loaded and active reconstruction files in the reconstruction graph.
		 * @param reconstruction_anchor_plate_id the anchor plate id used in the reconstruction.
		 * @param reconstruction_time time at which the reconstruction took place.
		 * @param export_single_output_file write all geometries to a single file.
		 * @param export_per_input_file write output files corresponding to input files.
		 * @param export_separate_output_directory_per_input_file save each file to a different directory.
		 * @param wrap_to_dateline if true then exported geometries are wrapped/clipped to the dateline.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_visible_reconstructed_motion_paths(
				const QString &filename,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
				const files_collection_type &active_files,
				const files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file,
				bool wrap_to_dateline);


		/**
		 * Collects visible resolved topologies including @a ResolvedTopologicalLine,
		 * @a ResolvedTopologicalBoundary and @a ResolvedTopologicalNetwork objects that are
		 * displayed using @a rendered_geom_collection and exports to a file depending on the
		 * file extension of @a filename.
		 *
		 * @param active_files used to determine which files the resolved topologies came from.
		 * @param reconstruction_anchor_plate_id the anchor plate id used in the reconstruction.
		 * @param reconstruction_time time at which the reconstruction took place.
		 * @param export_single_output_file write all geometries to a single file.
		 * @param export_per_input_file write output files corresponding to input files.
		 * @param export_separate_output_directory_per_input_file save each file to a different directory.
		 * @param export_topological_lines export resolved topological lines.
		 * @param export_topological_polygons export resolved topological polygons.
		 * @param export_topological_networks export resolved topological networks.
		 * @param export_topological_sections export resolved topological sections (*shared* sub-segments).
		 * @param export_topological_line_sub_segments Export the individual sub-segments of each boundary segment
		 *                                             that came from a resolved topological line.
		 *                                             If false then a single geometry per boundary segment is exported.
		 * @param force_polygon_orientation optionally force polygon orientation (clockwise or counter-clockwise).
		 * @param wrap_to_dateline if true then exported geometries are wrapped/clipped to the dateline.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_visible_resolved_topologies(
				const QDir &target_dir,
				const QString &file_basename,
				const QString &placeholder_format_string,
				const QString &placeholder_topological_geometries,
				const QString &placeholder_topological_sections,
				const QString &placeholder_topological_sections_subduction,
				const QString &placeholder_topological_sections_subduction_left,
				const QString &placeholder_topological_sections_subduction_right,
				const QString &placeholder_topological_sections_ridge_transform,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
				const files_collection_type &active_files,
				const files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file,
				bool export_topological_lines,
				bool export_topological_polygons,
				bool export_topological_networks,
				bool export_topological_sections,
				bool export_topological_line_sub_segments,
				boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation,
				bool wrap_to_dateline);
	}
}

#endif // GPLATES_VIEWOPERATIONS_VISIBLERECONSTRUCTIONFEATUREGEOMETRYEXPORT_H
