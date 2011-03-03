/* $Id: ShapefileFormatReconstructedFeatureGeometryExport.h 6843 2009-10-15 14:54:14Z rwatson $ */

/**
 * \file Exports motion_paths to shapefile format.
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

#ifndef GPLATES_FILEIO_SHAPEFILEFORMATMOTIONPATHEXPORT_H
#define GPLATES_FILEIO_SHAPEFILEFORMATMOTIONPATHEXPORT_H

#include "ReconstructionGeometryExportImpl.h"


namespace GPlatesAppLogic
{
	class ReconstructedMotionPath;
}

namespace GPlatesFileIO
{

	namespace ShapefileFormatMotionPathExport
	{
		/**
		 * Typedef for a feature geometry group of @a ReconstructedMotionPath objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::ReconstructedMotionPath>
				feature_geometry_group_type;

		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type
				referenced_files_collection_type;


		/**
		* Exports @a ReconstructedMotionPath objects to ESRI Shapefile format.
		*/
		void
		export_motion_paths(
				const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool should_export_referenced_files = true);


	}
	

}

#endif // GPLATES_FILEIO_SHAPEFILEFORMATMOTIONPATHEXPORTER_H
