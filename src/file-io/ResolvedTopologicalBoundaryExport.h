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

#ifndef GPLATES_FILE_IO_RESOLVEDTOPOLOGICALBOUNDARYEXPORT_H
#define GPLATES_FILE_IO_RESOLVEDTOPOLOGICALBOUNDARYEXPORT_H

#include <vector>
#include <boost/optional.hpp>
#include <QDir>
#include <QFileInfo>
#include <QString>

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ResolvedTopologicalBoundary;
}

namespace GPlatesFileIO
{
	namespace ResolvedTopologicalBoundaryExport
	{
		//! Formats of files that can export resolved topological boundaries.
		enum Format
		{
			UNKNOWN,           //!< Format, or file extension, is unknown.

			GMT,               //!< '.xy' extension.
			
			SHAPEFILE          //!< '.shp' extension.
		};


		/**
		 * Determine type of export file format based on filename extension.
		 *
		 * @param file_info file whose extension used to determine file format.
		 */
		Format
		get_export_file_format(
				const QFileInfo& file_info);


		/**
		 * Fine-grained control over the types of output files generated.
		 *
		 * Each of these boost optionals determines if the specific type of
		 * output is exported.
		 *
		 * The boost optionals containing placeholder strings represent
		 * the filename replacement of the substring defined by 'placeholder_format_string'
		 * in the function @a export_resolved_topological_boundaries - the 'file_basename'
		 * parameter of @a export_resolved_topological_boundaries is expected to contain
		 * that substring.
		 */
		struct OutputOptions
		{
			explicit
			OutputOptions(
					bool export_all_plate_polygons_to_a_single_file_ = true,
					bool export_all_slab_polygons_to_a_single_file_ = true,
					bool export_individual_plate_polygon_files_ = true,
					bool export_individual_slab_polygon_files_ = true,
					bool export_plate_polygon_subsegments_to_lines_ = true,
					bool export_slab_polygon_subsegments_to_lines_ = true,
					bool export_ridge_transforms_ = true,
					bool export_subductions_ = true,
					bool export_left_subductions_ = true,
					bool export_right_subductions_ = true,
					bool export_slab_edge_leading_ = true,
					bool export_slab_edge_leading_left_ = true,
					bool export_slab_edge_leading_right_ = true,
					bool export_slab_edge_trench_ = true,
					bool export_slab_edge_side_ = true,
					const QString &placeholder_platepolygons_ = "platepolygons",
					const QString &placeholder_lines_ = "lines",
					const QString &placeholder_ridge_transforms_ = "ridge_transform_boundaries",
					const QString &placeholder_subductions_ = "subduction_boundaries",
					const QString &placeholder_left_subductions_ = "subduction_boundaries_sL",
					const QString &placeholder_right_subductions_ = "subduction_boundaries_sR",
					const QString &placeholder_slab_polygons_ = "slab_polygons",
					const QString &placeholder_slab_edge_leading_ = "slab_edges_leading",
					const QString &placeholder_slab_edge_leading_left_ = "slab_edges_leading_sL",
					const QString &placeholder_slab_edge_leading_right_ = "slab_edges_leading_sR",
					const QString &placeholder_slab_edge_trench_ = "slab_edges_trench",
					const QString &placeholder_slab_edge_side_ = "slab_edges_side") :
				placeholder_platepolygons(placeholder_platepolygons_),
				export_all_plate_polygons_to_a_single_file(export_all_plate_polygons_to_a_single_file_),
				export_individual_plate_polygon_files(export_individual_plate_polygon_files_),
				placeholder_slab_polygons(placeholder_slab_polygons_),
				export_all_slab_polygons_to_a_single_file(export_all_slab_polygons_to_a_single_file_),
				export_individual_slab_polygon_files(export_individual_slab_polygon_files_),
				placeholder_lines(placeholder_lines_),
				export_plate_polygon_subsegments_to_lines(export_plate_polygon_subsegments_to_lines_),
				export_slab_polygon_subsegments_to_lines(export_slab_polygon_subsegments_to_lines_),
				placeholder_ridge_transforms(placeholder_ridge_transforms_),
				export_ridge_transforms(export_ridge_transforms_),
				placeholder_subductions(placeholder_subductions_),
				export_subductions(export_subductions_),
				placeholder_left_subductions(placeholder_left_subductions_),
				export_left_subductions(export_left_subductions_),
				placeholder_right_subductions(placeholder_right_subductions_),
				export_right_subductions(export_right_subductions_),
				placeholder_slab_edge_leading(placeholder_slab_edge_leading_),
				export_slab_edge_leading(export_slab_edge_leading_),
				placeholder_slab_edge_leading_left(placeholder_slab_edge_leading_left_),
				export_slab_edge_leading_left(export_slab_edge_leading_left_),
				placeholder_slab_edge_leading_right(placeholder_slab_edge_leading_right_),
				export_slab_edge_leading_right(export_slab_edge_leading_right_),
				placeholder_slab_edge_trench(placeholder_slab_edge_trench_),
				export_slab_edge_trench(export_slab_edge_trench_),
				placeholder_slab_edge_side(placeholder_slab_edge_side_),
				export_slab_edge_side(export_slab_edge_side_)
			{  }

			// Plate polygons placeholder string.
			QString placeholder_platepolygons;
			// If true then export all plate polygon to a single file.
			bool export_all_plate_polygons_to_a_single_file;
			// If true then export each plate polygon to its own file.
			bool export_individual_plate_polygon_files;

			// Slab polygons placeholder string.
			QString placeholder_slab_polygons;
			// If true then export all slab polygon to a single file.
			bool export_all_slab_polygons_to_a_single_file;
			// If true then export each slab polygon to its own file.
			bool export_individual_slab_polygon_files;

			// All subsegments placeholder string.
			//
			// Depending on @a export_plate_polygon_subsegments_to_lines and
			// @a export_slab_polygon_subsegments_to_lines.
			QString placeholder_lines;
			// If true then plate polygon subsegments also exported to 'lines' file.
			bool export_plate_polygon_subsegments_to_lines;
			// If true then slab polygon subsegments also exported to 'lines' file.
			bool export_slab_polygon_subsegments_to_lines;

			// Plate polygon subsegments placeholder strings.
			QString placeholder_ridge_transforms;
			bool export_ridge_transforms;
			QString placeholder_subductions;
			bool export_subductions;
			QString placeholder_left_subductions;
			bool export_left_subductions;
			QString placeholder_right_subductions;
			bool export_right_subductions;

			// Slab polygon subsegments placeholder strings.
			QString placeholder_slab_edge_leading;
			bool export_slab_edge_leading;
			QString placeholder_slab_edge_leading_left;
			bool export_slab_edge_leading_left;
			QString placeholder_slab_edge_leading_right;
			bool export_slab_edge_leading_right;
			QString placeholder_slab_edge_trench;
			bool export_slab_edge_trench;
			QString placeholder_slab_edge_side;
			bool export_slab_edge_side;
		};


		/**
		 * Exports @a ResolvedTopologicalBoundary objects and associated subsegments as specified
		 * by the options in @a output_options.
		 *
		 * @param export_format specifies which format to write.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_resolved_topological_boundaries(
				const QDir &target_dir,
				const QString &file_basename,
				const QString &placeholder_format_string,
				const OutputOptions &output_options,
				Format export_format,
				const std::vector<const GPlatesAppLogic::ResolvedTopologicalBoundary *> &resolved_topological_boundary_seq,
				const std::vector<const File::Reference *> &loaded_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);
	}
}

#endif // GPLATES_FILE_IO_RESOLVEDTOPOLOGICALBOUNDARYEXPORT_H
