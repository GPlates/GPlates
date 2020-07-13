/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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
// Vertex shader source code to render stars (points) in the 3D globe views (perspective and orthographic).
//
// The only purpose of this shader is to project any stars behind the far clip plane back onto the far plane so that
// they don't get clipped. This means we don't have to worry about how far away (distance-wise) we created the stars.
//

void main (void)
{
	vec4 clip_space_position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // The near/far planes clip such that:
    //
    //   -clip_space_position.w <= clip_space_position.z <= clip_space_position.w
    //
    // ...so clamp anything beyond the far plane to the far plane.
    //
    // Note that this won't affect the projected xy position in the viewport since we're
    // only clamping z (depth).
    //
    // This means the perspective divide (automatically done after vertex shader)
    //   clip_space_position.xyz / clip_space_position.w
    // will result in a normalised device z coordinate of 1.0 (corresponding to the far plane).
    // This trick works fine for individual points, but we might get distorted textures if we had
    // used this for textured triangles (due to messing with the perspective division).
    //
    if (clip_space_position.z > clip_space_position.w)
    {
        clip_space_position.z = clip_space_position.w;
    }

    gl_Position = clip_space_position;

	// Output the point colour.
	// Specify both front and back colours (since there's no back-face culling).
	gl_FrontColor = gl_Color;
	gl_BackColor = gl_Color;
}
