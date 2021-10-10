/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "Reconstruction.h"


GPlatesModel::Reconstruction::~Reconstruction()
{
	// Tell all RFGs, which currently point to this Reconstruction instance, to set those
	// pointers to NULL, lest they become dangling pointers.

	geometry_collection_type::iterator
			geometry_iter = d_geometries.begin(),
			geometry_end = d_geometries.end();
	for ( ; geometry_iter != geometry_end; ++geometry_iter) {
		(*geometry_iter)->set_reconstruction_ptr(NULL);
	}
}
