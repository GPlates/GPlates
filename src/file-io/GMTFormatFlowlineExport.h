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

#ifndef GPLATES_FILEIO_GMTFORMATFLOWLINESEXPORT_H
#define GPLATES_FILEIO_GMTFORMATFLOWLINESEXPORT_H

#include <QFileInfo>

#include "file-io/File.h"
#include "file-io/ReconstructionGeometryExportImpl.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedFlowline;
}

namespace GPlatesFileIO
{
	namespace GMTFormatFlowlinesExport
	{
		/**
		 * Typedef for a feature geometry group of @a ReconstructedFlowline objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFlowline>
				feature_geometry_group_type;

		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type
				referenced_files_collection_type;


		void
		export_flowlines(
				const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
				const QFileInfo &qfile_info, 
				const referenced_files_collection_type referenced_files, 
				const referenced_files_collection_type active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &anchor_plate_id, 
				const double &reconstruction_time);
	}
}


#endif //GPLATES_FILEIO_GMTFORMATFLOWLINESEXPORT_H
