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
			OutputOptions() :
					export_individual_plate_polygon_files(false),
					export_individual_slab_polygon_files(false),
					export_plate_polygon_subsegments_to_lines(false),
					export_slab_polygon_subsegments_to_lines(false)
			{  }

			// Plate polygons.
			boost::optional<QString> placeholder_platepolygons;

			// Slab polygons.
			boost::optional<QString> placeholder_slab_polygons;

			// All subsegments go here.
			//
			// Depending on @a export_plate_polygon_subsegments_to_lines and
			// @a export_slab_polygon_subsegments_to_lines.
			boost::optional<QString> placeholder_lines;

			// Subsegments for plate polygons.
			boost::optional<QString> placeholder_ridge_transforms;
			boost::optional<QString> placeholder_subductions;
			boost::optional<QString> placeholder_left_subductions;
			boost::optional<QString> placeholder_right_subductions;

			// Subsegments for slab polygons.
			boost::optional<QString> placeholder_slab_edge_leading;
			boost::optional<QString> placeholder_slab_edge_leading_left;
			boost::optional<QString> placeholder_slab_edge_leading_right;
			boost::optional<QString> placeholder_slab_edge_trench;
			boost::optional<QString> placeholder_slab_edge_side;

			// If true then also export each plate polygon to its own file.
			bool export_individual_plate_polygon_files;

			// If true then also export each slab polygon to its own file.
			bool export_individual_slab_polygon_files;

			// If true then plate polygon subsegments also exported to 'lines' file.
			bool export_plate_polygon_subsegments_to_lines;

			// If true then slab polygon subsegments also exported to 'lines' file.
			bool export_slab_polygon_subsegments_to_lines;
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
