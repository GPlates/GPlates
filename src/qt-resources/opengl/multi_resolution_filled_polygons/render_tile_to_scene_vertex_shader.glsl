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

/*
 * Vertex shader source code to render a tile to the scene.
 */

#ifdef SURFACE_LIGHTING
// The world-space coordinates are interpolated across the geometry.
varying vec3 world_space_position;
#endif

void main (void)
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// Transform present-day position by cube map projection and
	// any texture coordinate adjustments before accessing textures.
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_Vertex;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_Vertex;

#ifdef SURFACE_LIGHTING
	// This assumes the geometry does not need a model transform (eg, reconstruction rotation).
	world_space_position = gl_Vertex.xyz / gl_Vertex.w;
#endif
}
