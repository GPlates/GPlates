/* $Id$ */

/**
 * \file Exports reconstructed feature geometries to a file.
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

#ifndef GPLATES_FILEIO_RECONSTRUCTEDFEATUREGEOMETRYEXPORT_H
#define GPLATES_FILEIO_RECONSTRUCTEDFEATUREGEOMETRYEXPORT_H

#include <vector>
#include <list>
#include <QFileInfo>

#include "model/types.h"
#include "qt-widgets/ApplicationState.h"


namespace GPlatesModel
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

			SHAPEFILE		   //!< '.shp' extension.

		};


		/**
		 * Typedef for a sequence of @a ReconstructedFeatureGeometry pointers.
		 */
		typedef std::vector<const GPlatesModel::ReconstructedFeatureGeometry *>
			reconstructed_feature_geometry_seq_type;

		/**
		 * Groups @a ReconstructedFeatureGeometry objects with their feature.
		 */
		struct FeatureGeometryGroup
		{
			FeatureGeometryGroup(
					const GPlatesModel::FeatureHandle::const_weak_ref &_feature_ref) :
				feature_ref(_feature_ref)
			{  }

			GPlatesModel::FeatureHandle::const_weak_ref feature_ref;
			reconstructed_feature_geometry_seq_type recon_feature_geoms;
		};

		/**
		 * Typedef for a sequence of @a FeatureGeometryGroup objects.
		 */
		typedef std::list<FeatureGeometryGroup> feature_geometry_group_seq_type;

		//! Typedef for iterator into global list of loaded feature collection files.
		typedef GPlatesAppState::ApplicationState::file_info_const_iterator file_info_const_iterator;

		//! Typedef for sequence of file iterators that reference a collection of geometries.
		typedef std::vector<file_info_const_iterator> referenced_files_collection_type;


		/**
		 * Determine type of export file format based on filename extension.
		 *
		 * @param file_info file whose extension used to determine file format.
		 */
		ReconstructedFeatureGeometryExport::Format
		get_export_file_format(
				const QFileInfo& file_info);


		/**
		* Exports @a ReconstructedFeatureGeometry objects that are already grouped
		* with their feature.
		*
		* @param export_format specifies which format to write.
		*
		* @throws ErrorOpeningFileForWritingException if file is not writable.
		* @throws FileFormatNotSupportedException if file format not supported.
		*/
		void
		export_geometries(
				const feature_geometry_group_seq_type &feature_geometry_group_seq,
				ReconstructedFeatureGeometryExport::Format export_format,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);


		/**
		* Exports @a ReconstructedFeatureGeometry objects that are already grouped
		* with their feature.
		*
		* @param file_info file whose extension is used to determine which format to write.
		*
		* @throws ErrorOpeningFileForWritingException if file is not writable.
		* @throws FileFormatNotSupportedException if file format not supported.
		*/
		inline
		void
		export_geometries(
				const feature_geometry_group_seq_type &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time)
		{
			export_geometries(
					feature_geometry_group_seq,
					get_export_file_format(file_info),
					file_info,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time);
		}
	}
}

#endif // GPLATES_FILEIO_RECONSTRUCTEDFEATUREGEOMETRYEXPORT_H
