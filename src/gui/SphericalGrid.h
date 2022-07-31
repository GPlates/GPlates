/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_SPHERICALGRID_H
#define GPLATES_GUI_SPHERICALGRID_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "GraticuleSettings.h"

#include "opengl/GLBuffer.h"
#include "opengl/GLProgram.h"
#include "opengl/GLVertexArray.h"
#include "opengl/OpenGL.h"  // For Class GL and the OpenGL constants/typedefs


namespace GPlatesOpenGL
{
	class GLViewProjection;

	namespace GLIntersect
	{
		class Plane;
	}
}

namespace GPlatesMaths
{
	class UnitVector3D;
}

namespace GPlatesGui
{
	/**
	 * Renders latitude and longitude grid lines in the 3D globe view.
	 */
	class SphericalGrid :
			private boost::noncopyable
	{
	public:

		SphericalGrid(
				GPlatesOpenGL::GL &gl,
				const GraticuleSettings &graticule_settings);

		/**
		 * Paints lines of latitude and longitude on the surface of the sphere.
		 *
		 * @param globe_horizon_plane Plane that separates visible front half of globe from rear
		 *        (from the camera's point of view). This plane determines whether front or rear
		 *        of globe is rendered. Only the part of globe in positive half space is rendered.
		 *
		 * Note: We don't increase the grid line width to compensate for high-DPI displays.
		 *       This means the grid lines are not as wide as on regular displays.
		 *       But this is desired - keeps the grid lines minimally intrusive wrt regular line geometries.
		 */
		void
		paint(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				const GPlatesOpenGL::GLIntersect::Plane &globe_horizon_plane);

	private:
		const GraticuleSettings &d_graticule_settings;
		boost::optional<GraticuleSettings> d_last_seen_graticule_settings;

		//! Shader program to render grid lines.
		GPlatesOpenGL::GLProgram::shared_ptr_type d_program;

		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_vertex_array;
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_vertex_buffer;
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_vertex_element_buffer;

		unsigned int d_num_vertex_indices;
	};
}

#endif  // GPLATES_GUI_SPHERICALGRID_H
