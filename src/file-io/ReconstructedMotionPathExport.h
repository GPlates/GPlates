/* $Id$ */

/**
 * \file Exports reconstructed feature geometries to a file.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_RECONSTRUCTEDMOTIONPATHEXPORT_H
#define GPLATES_FILEIO_RECONSTRUCTEDMOTIONPATHEXPORT_H

#include <vector>
#include <QFileInfo>
#include <QString>

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedMotionPath;
}

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class Registry;
	}

	namespace ReconstructedMotionPathExport
	{
		//! Formats of files that can export reconstructed motion paths.
		enum Format
		{
			UNKNOWN,           //!< Format, or file extension, is unknown.

			GMT,               //!< '.xy' extension.
			SHAPEFILE,         //!< '.shp' extension.
			OGRGMT,            //!< '.gmt' extension.
			GEOJSON            //!< '.geojson' or '.json' extension.
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
		 * Exports @a ReconstructedMotionPath objects.
		 *
		 * @param export_format specifies which format to write.
		 * @param export_single_output_file specifies whether to write all reconstruction geometries
		 *        to a single file.
		 * @param export_per_input_file specifies whether to group
		 *        reconstruction geometries according to the input files their features came from
		 *        and write to corresponding output files.
		 * @param export_separate_output_directory_per_input_file
		 *        Save each exported file to a different directory based on the file basename.
		 *        Only applies if @a export_per_input_file is 'true'.
		 * @param wrap_to_dateline if true then exported geometries are wrapped/clipped to the dateline.
		 *
		 * Note that both @a export_single_output_file and @a export_per_input_file can be true
		 * in which case both a single output file is exported as well as grouped output files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_reconstructed_motion_paths(const QString &filename,
				Format export_format,
				const std::vector<const GPlatesAppLogic::ReconstructedMotionPath *> &reconstructed_motion_path_seq,
				const std::vector<const File::Reference *> &active_files,
				const std::vector<const File::Reference *> &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file,
				bool wrap_to_dateline);
	}
}

#endif // GPLATES_FILEIO_RECONSTRUCTEDMOTIONPATHEXPORT_H

