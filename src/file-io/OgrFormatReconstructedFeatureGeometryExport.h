/* $Id$ */

/**
 * \file Exports reconstructed feature geometries to a GMT format file.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010, 2014 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_SHAPEFILEFORMATRECONSTRUCTEDFEATUREGEOMETRYEXPORT_H
#define GPLATES_FILEIO_SHAPEFILEFORMATRECONSTRUCTEDFEATUREGEOMETRYEXPORT_H

#include <QFileInfo>

#include "ReconstructionGeometryExportImpl.h"

#include "model/types.h"
#include "property-values/GpmlKeyValueDictionary.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesFileIO
{
	namespace OgrFormatReconstructedFeatureGeometryExport
	{
		/**
		 * Typedef for a feature geometry group of @a ReconstructedFeatureGeometry objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFeatureGeometry>
				feature_geometry_group_type;

		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type
				referenced_files_collection_type;


		/**
		* Exports @a ReconstructedFeatureGeometry objects to ESRI Shapefile format.
		*
		* If @a wrap_to_dateline is true then exported polyline/polygon geometries are wrapped/clipped to the dateline.
		*/
		void
		export_geometries(
				const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const referenced_files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool wrap_to_dateline = true);

		/**
		* Exports @a ReconstructedFeatureGeometry objects to ESRI Shapefile format.
		*
		* If @a wrap_to_dateline is true then exported polyline/polygon geometries are wrapped/clipped to the dateline.
		*/
		void
		export_geometries_per_collection(
				const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const referenced_files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool wrap_to_dateline = true);
	}
}

#endif // GPLATES_FILEIO_SHAPEFILEFORMATRECONSTRUCTEDFEATUREGEOMETRYEXPORT_H
