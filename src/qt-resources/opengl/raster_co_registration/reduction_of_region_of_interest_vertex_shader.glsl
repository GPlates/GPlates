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
 * Vertex shader source to reduce region-of-interest filter results.
 */

// (x,y,z) is (translate_x, translate_y, scale).
uniform vec3 target_quadrant_translate_scale;

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 texture_coordinate;

out VertexData
{
	vec4 texture_coordinate;
} vs_out;

void main (void)
{
	out.texture_coordinate = texture_coordinate;

	// Scale and translate the pixel coordinates to the appropriate quadrant
	// of the destination render target.
	gl_Position.x = dot(target_quadrant_translate_scale.zx, position.xw);
	gl_Position.y = dot(target_quadrant_translate_scale.zy, position.yw);
	gl_Position.zw = position.zw;
}
