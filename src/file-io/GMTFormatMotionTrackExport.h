/* $Id: GMTFormatFlowlinesExporter.h 8209 2010-04-27 14:24:11Z rwatson $ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date: 2010-04-27 16:24:11 +0200 (ti, 27 apr 2010) $
* 
* Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_GMTFORMATMOTIONTRACKSEXPORT_H
#define GPLATES_FILEIO_GMTFORMATMOTIONTRACKSEXPORT_H

#include <QFileInfo>

#include "file-io/File.h"
#include "file-io/ReconstructedMotionTrackExportImpl.h"
#include "model/types.h"

namespace GPlatesFileIO
{

	namespace GMTFormatMotionTracksExport
	{
		/**
		 * Typedef for a sequence of files that reference the geometries.
		 */
		typedef std::vector<const GPlatesFileIO::File::Reference *> referenced_files_collection_type;	


		/**
		 * Typedef for a sequence of @a MotionTrackGroup objects.
		 */
		typedef ReconstructedMotionTrackExportImpl::motion_track_group_seq_type
			motion_track_group_seq_type;


		void
		export_motion_tracks(
				const motion_track_group_seq_type &motion_track_group_seq,
				const QFileInfo &qfile_info, 
				const referenced_files_collection_type referenced_files, 
				const GPlatesModel::integer_plate_id_type &anchor_plate_id, 
				const double &reconstruction_time);


	}
}


#endif //GPLATES_FILEIO_GMTFORMATMOTIONTRACKSEXPORT_H
