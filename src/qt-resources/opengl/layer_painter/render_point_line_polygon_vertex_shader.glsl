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
// Vertex shader source code to render points, lines and polygons with lighting.
//

// The 3D globe view needs to calculate lighting across the geometry but the map view does not
// because the surface normal is constant across the map (ie, it's flat unlike the 3D globe).
uniform bool map_view_enabled;

uniform bool lighting_enabled;

// The world-space coordinates are interpolated across the geometry.
varying vec3 globe_view_world_space_position;

void main (void)
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// Output the vertex colour.
	// We render both sides (front and back) of triangles (ie, there's no back-face culling).
	gl_FrontColor = gl_Color;
	gl_BackColor = gl_Color;

    if (lighting_enabled)
    {
        if (!map_view_enabled)
        {
            // This assumes the geometry does not need a model transform (eg, reconstruction rotation).
            globe_view_world_space_position = gl_Vertex.xyz / gl_Vertex.w;
        }
    }
}
