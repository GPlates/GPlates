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
 * Vertex shader source code to render light direction into cube texture for a 2D map view.
 */

uniform vec3 view_space_light_direction;

varying vec3 world_space_light_direction;

void main (void)
{
	gl_Position = gl_Vertex;

	// If the light direction is attached to view space then reverse rotate to world space.
	// We're hijacking the GL_MODELVIEW matrix to store the view transform so we can get
	// OpenGL to calculate the matrix inverse for us.
	world_space_light_direction = mat3(gl_ModelViewMatrixInverse) * view_space_light_direction;
}
