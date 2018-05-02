/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_GMTFORMATDEFORMATIONEXPORT_H
#define GPLATES_FILE_IO_GMTFORMATDEFORMATIONEXPORT_H

#include <QFileInfo>

#include "DeformationExport.h"
#include "ReconstructionGeometryExportImpl.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class TopologyReconstructedFeatureGeometry;
}

namespace GPlatesFileIO
{
	namespace GMTFormatDeformationExport
	{
		/**
		 * Typedef for a feature geometry group of @a TopologyReconstructedFeatureGeometry objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::TopologyReconstructedFeatureGeometry>
				deformed_feature_geometry_group_type;

		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type referenced_files_collection_type;


		/**
		 * Exports @a TopologyReconstructedFeatureGeometry objects.
		 *
		 * Each line in the GMT file contains:
		 * 
		 *    domain_point [dilatation_strain_rate] [second_invariant_strain_rate]
		 * 
		 * ...where 'domain_point' is position associated with the dilatation strain rate.
		 * If @a include_dilatation_strain_rate is true then dilatation strain rate is output (in units of 1/second).
		 * If @a include_second_invariant_strain_rate is true then second invariant strain rate is output (in units of 1/second).
		 *
		 * If @a domain_point_lon_lat_format is true then the domain points are output as the
		 * GMT default of (longitude latitude), otherwise they're output as (latitude longitude).
		 */
		void
		export_deformation_strain_rate(
				const std::list<deformed_feature_geometry_group_type> &velocity_vector_field_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool domain_point_lon_lat_format,
				bool include_dilatation_strain_rate,
				bool include_second_invariant_strain_rate);
	}
}

#endif // GPLATES_FILE_IO_GMTFORMATDEFORMATIONEXPORT_H
