/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_OPENGL_GLPROJECTIONUTILS_H
#define GPLATES_OPENGL_GLPROJECTIONUTILS_H

#include <opengl/OpenGL.h>

#include "GLMatrix.h"
#include "GLViewport.h"


namespace GPlatesOpenGL
{
	/**
	 * Utilities involving projection of 3D geometry to screen-space using OpenGL.
	 *
	 * This typically involves the OpenGL model-view and projection transforms and the viewport.
	 */
	namespace GLProjectionUtils
	{
		/**
		 * Convenience function performs same as similarly named OpenGL function.
		 */
		int
		glu_project(
				const GLViewport &viewport,
				const GLMatrix &model_view_transform,
				const GLMatrix &projection_transform,
				double objx,
				double objy,
				double objz,
				GLdouble *winx,
				GLdouble *winy,
				GLdouble *winz);


		/**
		 * Convenience function performs same as similarly named OpenGL function.
		 */
		int
		glu_un_project(
				const GLViewport &viewport,
				const GLMatrix &model_view_transform,
				const GLMatrix &projection_transform,
				double winx,
				double winy,
				double winz,
				GLdouble *objx,
				GLdouble *objy,
				GLdouble *objz);


		/**
		 * Returns an estimate of the minimum size of a viewport pixel when projected onto
		 * the unit sphere using the specified model-view and projection transforms.
		 *
		 * This assumes the globe is a sphere of radius one centred at the origin in model space.
		 *
		 * Currently this is done by sampling the corners of the view frustum and the middle
		 * of each of the four sides of the view frustum and the centre.
		 *
		 * This method is reasonably expensive but should be fine since it's only
		 * called once per raster per render scene.
		 *
		 * Returned result is in the range (0, Pi] where Pi is the distance between north and
		 * south poles on the unit sphere.
		 */
		double
		get_min_pixel_size_on_unit_sphere(
				const GLViewport &viewport,
				const GLMatrix &model_view_transform,
				const GLMatrix &projection_transform);
	};
}

#endif // GPLATES_OPENGL_GLPROJECTIONUTILS_H
