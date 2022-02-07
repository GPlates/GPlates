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
 * Vertex shader source to generate normals from a height field.
 */

// (x,y,z.w) is (texture u scale, texture v scale, texture translate, height field scale).
uniform vec4 height_field_parameters;

void main (void)
{
	// Scale and translate the texture coordinates from normal map to height map
	// which has extra texels around the border. This also takes into account partial
	// normal map tiles (such as near bottom or right edges of source raster).
	gl_TexCoord[0].x = dot(height_field_parameters.xz, gl_MultiTexCoord0.xw);
	gl_TexCoord[0].y = dot(height_field_parameters.yz, gl_MultiTexCoord0.yw);
	gl_TexCoord[0].zw = gl_MultiTexCoord0.zw;

	gl_Position = gl_Vertex;
}
