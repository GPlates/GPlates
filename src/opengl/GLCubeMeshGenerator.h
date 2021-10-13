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

#ifndef GPLATES_OPENGL_GLCUBEMESHGENERATOR_H
#define GPLATES_OPENGL_GLCUBEMESHGENERATOR_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsFwd.h"
#include "maths/CubeCoordinateFrame.h"
#include "maths/UnitVector3D.h"


namespace GPlatesOpenGL
{
	/**
	 * Generates points for a cube subdivision mesh (on the sphere) that is gridded along the cube subdivision tiles.
	 */
	class GLCubeMeshGenerator :
			private boost::noncopyable
	{
	public:
		/**
		 * Constructor.
		 *
		 * @a cube_face_dimension specifies the density of mesh points along the side of a cube face.
		 * NOTE: It *must* be a power-of-two.
		 */
		explicit
		GLCubeMeshGenerator(
				unsigned int cube_face_dimension);


		/**
		 * Returns the power-of-two dimension of the side of a cube face in terms of mesh vertex spacing.
		 */
		unsigned int
		get_cube_face_dimension_in_vertex_spacing() const
		{
			return d_cube_face_dimension;
		}

		/**
		 * Returns the number of mesh vertices along the side of a cube face.
		 */
		unsigned int
		get_cube_face_dimension_in_vertex_samples() const
		{
			return d_cube_face_dimension + 1;
		}


		/**
		 * Create all mesh vertices for the specified cube face.
		 *
		 * The vertices in the array can be indexed using:
		 *
		 *    x_offset + y_offset * get_cube_face_dimension_in_vertex_samples()
		 *
		 * where 'x_offset' and 'y_offset' can be obtained from @a CubeQuadTreeLocation.
		 */
		void
		create_cube_face_mesh_vertices(
				std::vector<GPlatesMaths::UnitVector3D> &cube_face_mesh_vertices,
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) const;


		/**
		 * Create a subset of the mesh vertices for the specified cube face.
		 *
		 * The vertices in the array can be indexed using:
		 *
		 *    (x_offset - rect_x_offset) + (y_offset - rect_y_offset) * rect_width_in_samples
		 *
		 * where 'x_offset' and 'y_offset' can be obtained from @a CubeQuadTreeLocation.
		 */
		void
		create_mesh_vertices(
				std::vector<GPlatesMaths::UnitVector3D> &mesh_vertices,
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int rect_x_offset,
				unsigned int rect_y_offset,
				unsigned int rect_width_in_samples,
				unsigned int rect_height_in_samples) const;

	private:
		unsigned int d_cube_face_dimension;

		/**
		 * The vertices along the twelve edges of the cube.
		 */
		std::vector<GPlatesMaths::UnitVector3D> d_cube_edge_vertices_array[GPlatesMaths::CubeCoordinateFrame::NUM_CUBE_EDGES];


		void
		create_cube_edge_vertices(
				const GPlatesMaths::UnitVector3D cube_corner_vertices[]);
	};
}

#endif // GPLATES_OPENGL_GLCUBEMESHGENERATOR_H
