/* $Id: ShapefileFormatReconstructedFeatureGeometryExport.h 6843 2009-10-15 14:54:14Z rwatson $ */

/**
 * \file Exports motion_tracks to shapefile format.
 * $Revision: 6843 $
 * $Date: 2009-10-15 16:54:14 +0200 (to, 15 okt 2009) $
 * 
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

#ifndef GPLATES_FILEIO_SHAPEFILEFORMATMOTIONTRACKEXPORT_H
#define GPLATES_FILEIO_SHAPEFILEFORMATMOTIONTRACKEXPORT_H


#include "ReconstructedMotionTrackExportImpl.h"



namespace GPlatesFileIO
{

	namespace ShapefileFormatMotionTrackExport
	{


		/**
		 * Typedef for a sequence of @a FeatureGeometryGroup objects.
		 */
		typedef ReconstructedMotionTrackExportImpl::motion_track_group_seq_type
			motion_track_group_seq_type;

		/**
		 * Typedef for a sequence of files that reference the geometries.
		 */
		typedef ReconstructedMotionTrackExportImpl::referenced_files_collection_type
			referenced_files_collection_type;


		/**
		* Exports @a ReconstructedMotionTrack objects to ESRI Shapefile format.
		*/
		void
		export_motion_tracks(
				const motion_track_group_seq_type &motion_track_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool should_export_referenced_files = true);


	}
	

}

#endif // GPLATES_FILEIO_SHAPEFILEFORMATMOTIONTRACKEXPORTER_H
