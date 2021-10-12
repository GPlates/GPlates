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

#include "ReconstructionGeometryCollection.h"

#include "global/PreconditionViolationError.h"


GPlatesAppLogic::ReconstructionGeometryCollection::~ReconstructionGeometryCollection()
{
	// Tell all ReconstructionGeometry's, which currently point to this
	// ReconstructionGeometryCollection instance, to set those
	// pointers to NULL, lest they become dangling pointers.

	reconstruction_geometry_seq_type::iterator reconstruction_geometry_iter =
			d_reconstruction_geometry_seq.begin();
	reconstruction_geometry_seq_type::iterator reconstruction_geometry_end =
			d_reconstruction_geometry_seq.end();
	for ( ; reconstruction_geometry_iter != reconstruction_geometry_end; ++reconstruction_geometry_iter)
	{
		(*reconstruction_geometry_iter)->set_collection_ptr(NULL);
	}
}


void
GPlatesAppLogic::ReconstructionGeometryCollection::add_reconstruction_geometry(
		const ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry)
{
	d_reconstruction_geometry_seq.push_back(reconstruction_geometry);
	reconstruction_geometry->set_collection_ptr(this);
}


void
GPlatesAppLogic::ReconstructionGeometryCollection::set_reconstruction_ptr(
		Reconstruction *reconstruction_ptr) const
{
	// A ReconstructionGeometryCollection can only belong to one Reconstruction.
	// If we're setting it to a non-NULL value then it should currently be NULL.
	if (d_reconstruction_ptr != NULL && reconstruction_ptr != NULL)
	{
		throw GPlatesGlobal::PreconditionViolationError(GPLATES_EXCEPTION_SOURCE);
	}

	d_reconstruction_ptr = reconstruction_ptr;
}
