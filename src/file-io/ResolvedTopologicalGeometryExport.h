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

#ifndef GPLATES_FILE_IO_RESOLVEDTOPOLOGICALGEOMETRYEXPORT_H
#define GPLATES_FILE_IO_RESOLVEDTOPOLOGICALGEOMETRYEXPORT_H

#include <vector>
#include <boost/optional.hpp>
#include <QFileInfo>
#include <QString>

#include "file-io/File.h"

#include "maths/PolygonOrientation.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometry;
}

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class Registry;
	}

	namespace ResolvedTopologicalGeometryExport
	{
		//! Formats of files that can export resolved topological geometries.
		enum Format
		{
			UNKNOWN,           //!< Format, or file extension, is unknown.

			GMT,               //!< '.xy' extension.
			
			SHAPEFILE,         //!< '.shp' extension.

			OGRGMT			   //!< '.gmt' extension.
		};


		/**
		 * Determine type of export file format based on filename extension.
		 *
		 * @param file_info file whose extension used to determine file format.
		 */
		Format
		get_export_file_format(
				const QFileInfo& file_info,
				const FeatureCollectionFileFormat::Registry &file_format_registry);


		/**
		 * Exports resolved topology objects (includes @a ResolvedTopologicalLine,
		 * @a ResolvedTopologicalBoundary and @a ResolvedTopologicalNetwork).
		 *
		 * @param export_format specifies which format to write.
		 * @param export_single_output_file specifies whether to write all resolved topologies to a single file.
		 * @param export_per_input_file specifies whether to group
		 *        resolved topologies according to the input files their features came from
		 *        and write to corresponding output files.
		 * @param export_separate_output_directory_per_input_file
		 *        Save each exported file to a different directory based on the file basename.
		 *        Only applies if @a export_per_input_file is 'true'.
		 * @param force_polygon_orientation
		 *        Optionally force polygon orientation (clockwise or counter-clockwise).
		 *        Only applies to resolved topological boundaries and networks (their polygon boundaries).
		 * @param wrap_to_dateline if true then exported geometries are wrapped/clipped to
		 *        the dateline (currently only applies to @a SHAPEFILE format).
		 *
		 * Note that both @a export_single_output_file and @a export_per_input_file can be true
		 * in which case both a single output file is exported as well as grouped output files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_resolved_topological_geometries(
				const QString &filename,
				Format export_format,
				const std::vector<const GPlatesAppLogic::ReconstructionGeometry *> &resolved_topologies,
				const std::vector<const File::Reference *> &active_files,
				const std::vector<const File::Reference *> &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file,
				boost::optional<GPlatesMaths::PolygonOrientation::Orientation>
						force_polygon_orientation = boost::none,
				bool wrap_to_dateline = true);
	}
}

#endif // GPLATES_FILE_IO_RESOLVEDTOPOLOGICALGEOMETRYEXPORT_H
