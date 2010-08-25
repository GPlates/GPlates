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

#ifndef GPLATES_FILEIO_RECONSTRUCTEDFEATUREGEOMETRYEXPORT_H
#define GPLATES_FILEIO_RECONSTRUCTEDFEATUREGEOMETRYEXPORT_H

#include <vector>
#include <QString>

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesFileIO
{
	namespace ReconstructedFeatureGeometryExport
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

		//! Typedef for sequence of RFGs.
		typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *>
				reconstructed_feature_geom_seq_type;


		/**
		 * Determine type of export file format based on filename extension.
		 *
		 * @param file_info file whose extension used to determine file format.
		 */
		ReconstructedFeatureGeometryExport::Format
		get_export_file_format(
				const QFileInfo& file_info);


		/**
		 * Exports @a ReconstructedFeatureGeometry objects.
		 *
		 * @param export_format specifies which format to write.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_geometries_as_single_file(
				const QString &filename,
				ReconstructedFeatureGeometryExport::Format export_format,
				const reconstructed_feature_geom_seq_type &reconstructed_feature_geom_seq,
				const files_collection_type &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);

		/**
		 * Exports @a ReconstructedFeatureGeometry objects.
		 *
		 * @param export_format specifies which format to write.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_geometries_per_collection(
				const QString &filename,
				ReconstructedFeatureGeometryExport::Format export_format,
				const reconstructed_feature_geom_seq_type &reconstructed_feature_geom_seq,
				const files_collection_type &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);



		/**
		 * Exports @a ReconstructedFeatureGeometry objects.
		 *
		 * @param file_info file whose extension is used to determine which format to write.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		inline
		void
		export_geometries_as_single_file(
				const QString &filename,
				const reconstructed_feature_geom_seq_type &reconstructed_feature_geom_seq,
				const files_collection_type &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time)
		{
			export_geometries_as_single_file(
					filename,
					get_export_file_format(filename),
					reconstructed_feature_geom_seq,
					active_files,
					reconstruction_anchor_plate_id,
					reconstruction_time);
		}

		/**
		 * Exports @a ReconstructedFeatureGeometry objects.
		 *
		 * @param file_info file whose extension is used to determine which format to write.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		inline
		void
		export_geometries_per_collection(
				const QString &filename,
				const reconstructed_feature_geom_seq_type &reconstructed_feature_geom_seq,
				const files_collection_type &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time)
		{
			export_geometries_per_collection(
					filename,
					get_export_file_format(filename),
					reconstructed_feature_geom_seq,
					active_files,
					reconstruction_anchor_plate_id,
					reconstruction_time);
		}
	}

}

#endif // GPLATES_FILEIO_RECONSTRUCTEDFEATUREGEOMETRYEXPORT_H

