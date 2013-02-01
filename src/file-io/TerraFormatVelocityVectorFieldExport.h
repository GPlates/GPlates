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

#ifndef GPLATES_FILE_IO_TERRAFORMATVELOCITYVECTORFIELDEXPORT_H
#define GPLATES_FILE_IO_TERRAFORMATVELOCITYVECTORFIELDEXPORT_H

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
	namespace TerraFormatVelocityVectorFieldExport
	{
		/**
		 * Typedef for a feature geometry group of @a MultiPointVectorField objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::MultiPointVectorField>
				velocity_vector_field_group_type;


		/**
		 * Exports @a MultiPointVectorField objects containing *velocities* to Terra text format.
		 *
		 * @a age is the reconstruction time rounded to an integer.
		 */
		void
		export_velocity_vector_fields(
				const std::list<velocity_vector_field_group_type> &velocity_vector_field_group_seq,
				const QFileInfo& file_info,
				int terra_mt,
				int terra_nt,
				int terra_nd,
				int local_processor_number,
				int age);
	}
}

#endif // GPLATES_FILE_IO_TERRAFORMATVELOCITYVECTORFIELDEXPORT_H
