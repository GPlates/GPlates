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

#ifndef GPLATES_FILE_IO_GMTFORMATRECONSTRUCTEDSCALARCOVERAGEEXPORT_H
#define GPLATES_FILE_IO_GMTFORMATRECONSTRUCTEDSCALARCOVERAGEEXPORT_H

#include <QFileInfo>

#include "ReconstructedScalarCoverageExport.h"
#include "ReconstructionGeometryExportImpl.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedScalarCoverage;
}

namespace GPlatesFileIO
{
	namespace GMTFormatReconstructedScalarCoverageExport
	{
		/**
		 * Typedef for a feature geometry group of @a ReconstructedScalarCoverage objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::ReconstructedScalarCoverage>
				reconstructed_scalar_coverage_group_type;

		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type referenced_files_collection_type;


		/**
		 * Exports @a ReconstructedScalarCoverage objects.
		 *
		 * Each line in the GMT file contains:
		 * 
		 *    [domain_point] [dilatation_rate] scalar
		 * 
		 * ...where 'domain_point' is position associated with the scalar value.
		 * The domain point is only included if @a include_domain_point is true.
		 * And the dilatation rate (in units of 1/second) is only included if @a include_dilatation_rate is true.
		 *
		 * If @a domain_point_lon_lat_format is true then the domain points are output as the
		 * GMT default of (longitude latitude), otherwise they're output as (latitude longitude).
		 */
		void
		export_reconstructed_scalar_coverages(
				const std::list<reconstructed_scalar_coverage_group_type> &velocity_vector_field_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool domain_point_lon_lat_format,
				bool include_domain_point,
				bool include_dilatation_rate,
				bool include_domain_meta_data);
	}
}

#endif // GPLATES_FILE_IO_GMTFORMATRECONSTRUCTEDSCALARCOVERAGEEXPORT_H
