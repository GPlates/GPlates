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

#ifndef GPLATES_FILE_IO_CITCOMSFORMATVELOCITYVECTORFIELDEXPORT_H
#define GPLATES_FILE_IO_CITCOMSFORMATVELOCITYVECTORFIELDEXPORT_H

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
	namespace CitcomsFormatVelocityVectorFieldExport
	{
		/**
		 * Typedef for a feature geometry group of @a MultiPointVectorField objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::MultiPointVectorField>
				velocity_vector_field_group_type;


		/**
		 * Exports @a MultiPointVectorField objects containing *velocities* to CitcomS global format.
		 *
		 * @a age is the reconstruction time rounded to an integer.
		 *
		 * If @a include_gmt_export is true then, for each CitcomS velocity file exported, a
		 * CitcomS-compatible GMT format velocity file is exported with the same filename but
		 * with the GMT ".xy" filename extension added.
		 * If @a include_gmt_export is true then, only for the GMT exported files, the
		 * velocity magnitudes are scaled by @a gmt_velocity_scale and only every
		 * 'gmt_velocity_stride'th velocity vector is output.
		 */
		void
		export_global_velocity_vector_fields(
				const std::list<velocity_vector_field_group_type> &velocity_vector_field_group_seq,
				const QFileInfo& file_info,
				int age,
				bool include_gmt_export,
				double gmt_velocity_scale,
				unsigned int gmt_velocity_stride);
	}
}

#endif // GPLATES_FILE_IO_CITCOMSFORMATVELOCITYVECTORFIELDEXPORT_H
