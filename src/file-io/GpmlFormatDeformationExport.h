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

#ifndef GPLATES_FILE_IO_GPMLFORMATDEFORMATIONEXPORT_H
#define GPLATES_FILE_IO_GPMLFORMATDEFORMATIONEXPORT_H

#include <QFileInfo>

#include "DeformationExport.h"
#include "ReconstructionGeometryExportImpl.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class TopologyReconstructedFeatureGeometry;
}

namespace GPlatesModel
{
	class ModelInterface;
}

namespace GPlatesFileIO
{
	namespace GpmlFormatDeformationExport
	{
		/**
		 * Typedef for a feature geometry group of @a TopologyReconstructedFeatureGeometry objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::TopologyReconstructedFeatureGeometry>
				deformed_feature_geometry_group_type;


		/**
		 * Exports @a TopologyReconstructedFeatureGeometry objects along with their deformation information.
		 *
		 * If @a include_principal_strain is specified then 3 extra sets of per-point scalars are exported:
		 * - 'gpml:PrincipalStrainMajorAngle/Azimuth' or 'PrincipalStretchMajorAngle/Azimuth' is the angle or azimuth (in degrees) of major principal axis.
		 * - 'gpml:PrincipalStrainMajorAxis' or 'PrincipalStretchMajorAxis' is largest principal strain or stretch (1+strain), both unitless.
		 * - 'gpml:PrincipalStrainMinorAxis' or 'PrincipalStretchMinorAxis' is smallest principal strain or stretch (1+strain), both unitless.
		 *
		 * If @a include_dilatation_strain is true then an extra set of per-point scalars,
		 * under 'gpml:DilatationStrainRate', is exported as per-point dilatation strain (unitless).
		 *
		 * If @a include_dilatation_strain_rate is true then an extra set of per-point scalars,
		 * under 'gpml:DilatationStrainRate', is exported as per-point dilatation strain rates (in units of 1/second).
		 *
		 * If @a include_second_invariant_strain_rate  is true then an extra set of per-point scalars,
		 * under 'gpml:TotalStrainRate', is exported as per-point second invariant strain rates (in units of 1/second).
		 *
		 * If @a include_strain_rate_style is true then an extra set of per-point scalars,
		 * under 'gpml:StrainRateStyle', is exported as per-point strain rate styles (unitless).
		 */
		void
		export_deformation(
				const std::list<deformed_feature_geometry_group_type> &deformed_feature_geometry_group_seq,
				const QFileInfo& file_info,
				GPlatesModel::ModelInterface &model,
				boost::optional<DeformationExport::PrincipalStrainOptions> include_principal_strain,
				bool include_dilatation_strain,
				bool include_dilatation_strain_rate,
				bool include_second_invariant_strain_rate,
				bool include_strain_rate_style);
	}
}

#endif // GPLATES_FILE_IO_GPMLFORMATDEFORMATIONEXPORT_H
