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
	// pointers to NULL, lest they become dangling pointers.  (Note that currently, since RFGs
	// are contained directly within a Reconstruction instance, it's not possible for contained
	// RFGs to outlive their Reconstruction.  However, this could change in the future (for
	// example, if we start holding RFGs by intrusive-ptr), so we should put this "correct"
	// code in place before we forget.)

	// FIXME: Merge these two for-loops into one, when the containers in class Reconstruction
	// have been merged into one.
	std::vector<ReconstructedFeatureGeometry>::iterator
			point_iter = d_point_geometries.begin(),
			point_end = d_point_geometries.end();
	for ( ; point_iter != point_end; ++point_iter) {
		point_iter->set_reconstruction_ptr(NULL);
	}

	std::vector<ReconstructedFeatureGeometry>::iterator
			polyline_iter = d_polyline_geometries.begin(),
			polyline_end = d_polyline_geometries.end();
	for ( ; polyline_iter != polyline_end; ++polyline_iter) {
		polyline_iter->set_reconstruction_ptr(NULL);
	}
}
