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

//
// Vertex shader source code to render volume fill boundary.
//
// Used for both depth range and wall normals.
// Also used for both walls and spherical caps (for depth range).
//

attribute vec4 surface_point;
attribute vec4 centroid_point;

varying vec4 polygon_centroid_point;

void main (void)
{
	// Currently there is no model transform (Euler rotation).
	// Just pass the vertex position on to the geometry shader.
	// Later, if there's a model transform, then we'll do it here to take the load
	// off the geometry shader (only vertex shader has a post-transform hardware cache).
	gl_Position = surface_point;

	// This is not used for wall normals (only used for spherical caps).
	polygon_centroid_point = centroid_point;
}
