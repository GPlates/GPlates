/* $Id: ReconstructedFeatureGeometryExport.h -1   $ */

/**
 * \file Exports reconstructed feature geometries to a file.
 * $Revision: -1 $
 * $Date: $
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
#include <QString>

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedMotionPath;
}

namespace GPlatesFileIO
{
	namespace ReconstructedMotionPathExport
	{
		//! Formats of files that can export reconstructed feature geometries.
		enum Format
		{
			UNKNOWN,           //!< Format, or file extension, is unknown.

			GMT,               //!< '.xy' extension.
			
			SHAPEFILE          //!< '.shp' extension.
		};


		//! Typedef for sequence of feature collection files.
		typedef std::vector<const GPlatesFileIO::File::Reference *> files_collection_type;

		//! Typedef for sequence of reconstructed motion_paths.
		typedef std::vector<const GPlatesAppLogic::ReconstructedMotionPath*>
			reconstructed_motion_path_seq_type;


		/**
		 * Determine type of export file format based on filename extension.
		 *
		 * @param file_info file whose extension used to determine file format.
		 */
		ReconstructedMotionPathExport::Format
		get_export_file_format(
				const QFileInfo& file_info);



		/**
		 * Exports @a ReconstructedMotionPath objects.
		 *
		 * @param export_format specifies which format to write.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_reconstructed_motion_paths(
				const QString &filename,
				ReconstructedMotionPathExport::Format export_format,
				const reconstructed_motion_path_seq_type &reconstructed_motion_path_seq,
				const files_collection_type &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);


		/**
		 * Exports @a ReconstructedMotionPath objects.
		 *
		 * @param file_info file whose extension is used to determine which format to write.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		inline
		void
		export_reconstructed_motion_paths(
				const QString &filename,
				const reconstructed_motion_path_seq_type &reconstructed_motion_path_seq,
				const files_collection_type &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time)
		{
			export_reconstructed_motion_paths(
					filename,
					get_export_file_format(filename),
					reconstructed_motion_path_seq,
					active_files,
					reconstruction_anchor_plate_id,
					reconstruction_time);
		}
	}

}

#endif // GPLATES_FILEIO_RECONSTRUCTEDMOTIONPATHEXPORT_H

