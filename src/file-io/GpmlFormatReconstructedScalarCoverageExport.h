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

#ifndef GPLATES_FILE_IO_GPMLFORMATRECONSTRUCTEDSCALARCOVERAGEEXPORT_H
#define GPLATES_FILE_IO_GPMLFORMATRECONSTRUCTEDSCALARCOVERAGEEXPORT_H

#include <QFileInfo>

#include "ReconstructionGeometryExportImpl.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedScalarCoverage;
}

namespace GPlatesModel
{
	class ModelInterface;
}

namespace GPlatesFileIO
{
	namespace GpmlFormatReconstructedScalarCoverageExport
	{
		/**
		 * Typedef for a feature geometry group of @a ReconstructedScalarCoverage objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::ReconstructedScalarCoverage>
				reconstructed_scalar_coverage_group_type;


		/**
		 * Exports @a ReconstructedScalarCoverage objects.
		 *
		 * If @a include_dilatation_strain_rate is true then an extra set of per-point scalars,
		 * under 'gpml:DilatationStrainRate', is exported as per-point dilatation strain rates (in units of 1/second).
		 *
		 * If @a include_second_invariant_strain_rate is true then an extra set of per-point scalars,
		 * under 'gpml:TotalStrainRate', is exported as per-point second invariant strain rates (in units of 1/second).
		 */
		void
		export_reconstructed_scalar_coverages(
				const std::list<reconstructed_scalar_coverage_group_type> &reconstructed_scalar_coverage_group_seq,
				const QFileInfo& file_info,
				GPlatesModel::ModelInterface &model,
				bool include_dilatation_strain_rate,
				bool include_second_invariant_strain_rate);
	}
}

#endif // GPLATES_FILE_IO_GPMLFORMATRECONSTRUCTEDSCALARCOVERAGEEXPORT_H
