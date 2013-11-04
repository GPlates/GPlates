/* $Id$ */

/**
 * \file Exports motion_paths to shapefile format.
 * $Revision$
 * $Date$
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

	namespace OgrFormatMotionPathExport
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
		*
		* @param wrap_to_dateline if true then exported geometries are wrapped/clipped to the dateline.
		*/
		void
		export_motion_paths(
				const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool should_export_referenced_files = true,
				bool wrap_to_dateline = false);


	}
	

}

#endif // GPLATES_FILEIO_SHAPEFILEFORMATMOTIONPATHEXPORTER_H
