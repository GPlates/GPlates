/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_GMTFORMATMULTIPOINTVECTORFIELDEXPORT_H
#define GPLATES_FILE_IO_GMTFORMATMULTIPOINTVECTORFIELDEXPORT_H

#include <QFileInfo>

#include "MultiPointVectorFieldExport.h"
#include "ReconstructionGeometryExportImpl.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class MultiPointVectorField;
}

namespace GPlatesFileIO
{
	namespace GMTFormatMultiPointVectorFieldExport
	{
		/**
		 * Typedef for a feature geometry group of @a MultiPointVectorField objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::MultiPointVectorField>
				multi_point_vector_field_group_type;

		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type
				referenced_files_collection_type;


		/**
		 * Exports @a MultiPointVectorField objects containing *velocities*.
		 *
		 * Each line in the GMT file contains:
		 *
		 *    [domain_point] velocity [plate_id]
		 *
		 * ...where 'domain_point' is position at which the velocity was calculated and 'plate_id'
		 * is the plate id used to calculate the velocity (for topological networks the plate id
		 * only identifies the network used to calculate the velocity).
		 *
		 * The plate ID is only included if @a include_plate_id is true.
		 * The domain point is only included if @a include_domain_point is true.
		 * If @a domain_point_lon_lat_format is true then the domain points are output as the
		 * GMT default of (longitude latitude), otherwise they're output as (latitude longitude).
		 *
		 * Velocity magnitudes are scaled by @a velocity_scale.
		 * Only every 'velocity_stride'th velocity vector is output.
		 *
		 * The format of 'velocity' is determined by @a velocity_vector_format.
		 */
		void
		export_velocity_vector_fields(
				const std::list<multi_point_vector_field_group_type> &velocity_vector_field_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				MultiPointVectorFieldExport::GMTVelocityVectorFormatType velocity_vector_format,
				double velocity_scale,
				unsigned int velocity_stride,
				bool domain_point_lon_lat_format,
				bool include_plate_id,
				bool include_domain_point,
				bool include_domain_meta_data);
	}
}

#endif // GPLATES_FILE_IO_GMTFORMATMULTIPOINTVECTORFIELDEXPORT_H
