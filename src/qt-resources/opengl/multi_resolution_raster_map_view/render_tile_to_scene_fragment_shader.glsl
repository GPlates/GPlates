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
 * Fragment shader source code to render a tile to the scene.
 */

uniform sampler2D tile_texture_sampler;

#ifdef ENABLE_CLIPPING
uniform sampler2D clip_texture_sampler;
#endif // ENABLE_CLIPPING

void main (void)
{
	// Projective texturing to handle cube map projection.
	gl_FragColor = texture2DProj(tile_texture_sampler, gl_TexCoord[0]);

#ifdef ENABLE_CLIPPING
	gl_FragColor *= texture2DProj(clip_texture_sampler, gl_TexCoord[1]);
#endif // ENABLE_CLIPPING
}
