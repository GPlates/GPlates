/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
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

#include "ReconstructionGeometry.h"

#include "ReconstructionGeometryCollection.h"

#include "global/PreconditionViolationError.h"


GPlatesAppLogic::Reconstruction *
GPlatesAppLogic::ReconstructionGeometry::reconstruction() const
{
	Reconstruction *reconstruction_ptr = NULL;

	if (d_reconstruction_geometry_collection_ptr)
	{
		reconstruction_ptr = d_reconstruction_geometry_collection_ptr->reconstruction();
	}

	return reconstruction_ptr;
}


void
GPlatesAppLogic::ReconstructionGeometry::set_collection_ptr(
				   GPlatesAppLogic::ReconstructionGeometryCollection *collection_ptr) const
{
	// A ReconstructionGeometry can only belong to one ReconstructionGeometryCollection.
	// If we're setting it to a non-NULL value then it should currently be NULL.
	if (d_reconstruction_geometry_collection_ptr != NULL && collection_ptr != NULL)
	{
		throw GPlatesGlobal::PreconditionViolationError(GPLATES_EXCEPTION_SOURCE);
	}

	d_reconstruction_geometry_collection_ptr = collection_ptr;
}
