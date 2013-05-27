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
 * Fragment shader source code to render light direction into cube texture for a 2D map view.
 */

varying vec3 world_space_light_direction;

void main (void)
{
	// Just return the light direction - it doesn't vary across the cube texture face_target.
	// But first convert from range [-1,1] to range [0,1] to store in unsigned 8-bit render face_target.
   gl_FragColor = vec4(0.5 * world_space_light_direction + 0.5, 1);
}
