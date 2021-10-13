/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLMAPCUBEMESHGENERATOR_H
#define GPLATES_OPENGL_GLMAPCUBEMESHGENERATOR_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLCubeMeshGenerator.h"

#include "gui/MapProjection.h"

#include "maths/MathsFwd.h"
#include "maths/CubeCoordinateFrame.h"
#include "maths/UnitVector3D.h"


namespace GPlatesOpenGL
{
	/**
	 * Generates points for a cube subdivision mesh that are projected onto a 2D map.
	 *
	 * The points on the sphere that are projected are gridded along the cube subdivision tiles.
	 */
	class GLMapCubeMeshGenerator :
			private boost::noncopyable
	{
	public:
		/**
		 * A 2D map-projected point.
		 */
		struct Point2D
		{
			double x;
			double y;
		};

		/**
		 * The 2D map-projected point and its associated position on the sphere.
		 */
		struct Point
		{
			GPlatesMaths::UnitVector3D point_3D;
			Point2D point_2D;
		};


		/**
		 * Uses the specified map projection to project cube mesh points (on the sphere) onto the 2D map.
		 *
		 * @a cube_face_dimension specifies the density of mesh points along the side of a cube face.
		 * NOTE: It *must* be a power-of-two.
		 *
		 * NOTE: @a map_projection must exist as long as 'this' object exists.
		 */
		GLMapCubeMeshGenerator(
				const GPlatesGui::MapProjection &map_projection,
				unsigned int cube_face_dimension);


		/**
		 * Returns the power-of-two dimension of the side of a *quadrant* of a cube face in terms of mesh vertex spacing.
		 */
		unsigned int
		get_cube_face_quadrant_dimension_in_vertex_spacing() const
		{
			return d_cube_mesh_generator.get_cube_face_dimension_in_vertex_spacing() >> 1;
		}

		/**
		 * Returns the number of mesh vertices along the side of a *quadrant* of a cube face.
		 */
		unsigned int
		get_cube_face_quadrant_dimension_in_vertex_samples() const
		{
			return (d_cube_mesh_generator.get_cube_face_dimension_in_vertex_spacing() >> 1) + 1;
		}


		/**
		 * Create all map-projected mesh vertices for the specified *quadrant* of the specified cube face.
		 *
		 * The cube faces are divided into quadrants because the dateline then only touches the
		 * edges of quadrants and does not cut through them.
		 *
		 * The map-projected points (ie, the cube) are also aligned with the map projection's
		 * central meridian longitude.
		 *
		 * The vertices in the array can be indexed using:
		 *
		 *    (x_offset - quadrant_x_offset * D) + (y_offset - quadrant_y_offset * D) * N
		 *
		 * ...where...
		 * 'D' is 'get_cube_face_quadrant_dimension_in_vertex_spacing()',
		 * 'N' is 'get_cube_face_quadrant_dimension_in_vertex_samples()' and
		 * 'x_offset' and 'y_offset' can be obtained from @a CubeQuadTreeLocation.
		 *
		 * @a quadrant_x_offset and @a quadrant_y_offset follow the same offset direction and
		 * must be either 0 or 1.
		 *
		 * NOTE: The pre-map-projected longitude values (after lat/lon conversion, before map projection)
		 * at the north/south poles are (after adjusting for central meridian longitude):
		 *    +180.0 for quadrants in same hemisphere as dateline and in [0,+180] longitude range,
		 *    -180.0 for quadrants in same hemisphere as dateline and in [-180,0] longitude range,
		 *       0.0 for quadrants in opposite hemisphere to dateline.
		 */
		void
		create_cube_face_quadrant_mesh_vertices(
				std::vector<Point> &cube_face_quadrant_mesh_vertices,
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int quadrant_x_offset,
				unsigned int quadrant_y_offset) const;

	private:
		/**
		 * Used to generate the cube mesh positions on the sphere.
		 */
		GLCubeMeshGenerator d_cube_mesh_generator;

		/**
		 * Used to project points on the sphere onto a 2D map.
		 */
		const GPlatesGui::MapProjection &d_map_projection;
	};
}

#endif // GPLATES_OPENGL_GLMAPCUBEMESHGENERATOR_H
