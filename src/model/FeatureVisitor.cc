/* $Id$ */

/**
 * \file 
 * Contains template specialisations for the templated FeatureVisitor class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "FeatureVisitor.h"

#include "property-values/GpmlTotalReconstructionPole.h"


namespace GPlatesModel
{
	// Note: Only reason for specialising is to avoid needing to include "GpmlTotalReconstructionPole.h" in header.
	template<>
	void
	FeatureVisitorBase<FeatureHandle>::visit_gpml_total_reconstruction_pole(
			gpml_total_reconstruction_pole_type &gpml_total_reconstruction_pole)
	{
		// Default implementation delegates to base class 'gpml_finite_rotation_type'.
		visit_gpml_finite_rotation(gpml_total_reconstruction_pole);
	}


	// Note: Only reason for specialising is to avoid needing to include "GpmlTotalReconstructionPole.h" in header.
	template<>
	void
	FeatureVisitorBase<const FeatureHandle>::visit_gpml_total_reconstruction_pole(
			gpml_total_reconstruction_pole_type &gpml_total_reconstruction_pole)
	{
		// Default implementation delegates to base class 'gpml_finite_rotation_type'.
		visit_gpml_finite_rotation(gpml_total_reconstruction_pole);
	}
}
