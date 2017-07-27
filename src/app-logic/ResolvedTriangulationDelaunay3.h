/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
 * Copyright (C) 2012 2013 California Institute of Technology 
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONDELAUNAY3_H
#define GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONDELAUNAY3_H

#include <map>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include <QDebug>

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

// suppress bogus warning when compiling with gcc 4.3
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 3)
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif 

//PUSH_GCC_WARNINGS
//DISABLE_GCC_WARNING("-Wshadow")
//DISABLE_GCC_WARNING("-Wold-style-cast")
//DISABLE_GCC_WARNING("-Werror")

PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING( 4005 ) // For Boost 1.44 and Visual Studio 2010.
#include <CGAL/algorithm.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/interpolation_functions.h>
#include <CGAL/sibson_gradient_fitting.h>
#include <CGAL/surface_neighbor_coordinates_3.h>
#include <CGAL/Triangulation_hierarchy_3.h>

POP_MSVC_WARNINGS

//POP_GCC_WARNINGS

#include "maths/AzimuthalEqualAreaProjection.h"
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace ResolvedTriangulation
	{
		//
		// Basic CGAL typedefs for 3D delaunay triangulation.
		//

		typedef CGAL::Exact_predicates_inexact_constructions_kernel delaunay_kernel_3_type;
		typedef delaunay_kernel_3_type::FT delaunay_coord_3_type;

		typedef delaunay_kernel_3_type::Point_3 delaunay_point_3_type;
		typedef delaunay_kernel_3_type::Vector_3 delaunay_vector_3_type;


		typedef CGAL::Triangulation_hierarchy_vertex_base_3<
				CGAL::Triangulation_vertex_base_3<delaunay_kernel_3_type> >
						delaunay_triangulation_vertex_3_type;

		typedef CGAL::Triangulation_data_structure_3<delaunay_triangulation_vertex_3_type>
				delaunay_triangulation_data_structure_3_type;


		typedef std::vector< std::pair<delaunay_point_3_type, delaunay_coord_3_type> >
				delaunay_point_coordinate_vector_3_type;

		typedef std::map<delaunay_point_3_type, delaunay_coord_3_type, delaunay_kernel_3_type::Less_xy_3 >
				delaunay_map_point_to_value_3_type;

		typedef CGAL::Data_access<delaunay_map_point_to_value_3_type> delaunay_point_value_access_3_type;

		typedef std::map<delaunay_point_3_type, delaunay_vector_3_type, delaunay_kernel_3_type::Less_xy_3 >
				delaunay_map_point_to_vector_3_type;

		typedef CGAL::Data_access<delaunay_map_point_to_vector_3_type> delaunay_point_vector_access_3_type;


		//! Typedef for result of a natural neighbours query on a 3D triangulation.
		typedef std::pair<delaunay_point_coordinate_vector_3_type, delaunay_coord_3_type>
				delaunay_natural_neighbor_coordinates_3_type;


		/**
		 * 3D Delaunay triangulation.
		 */
		class Delaunay_3 :
				public CGAL::Triangulation_hierarchy_3<
						CGAL::Delaunay_triangulation_3<
								delaunay_kernel_3_type,
								delaunay_triangulation_data_structure_3_type> >
		{
		public:

			/**
			 * Returns the natural neighbor coordinates of @a point in the triangulation
			 * (which can then be used with different interpolation methods like linear interpolation).
			 *
			 * Returns false if @a point is outside the triangulation.
			 */
			bool
			calc_surface_neighbor_coordinates(
					delaunay_natural_neighbor_coordinates_3_type &natural_neighbor_coordinates,
					const delaunay_point_3_type &point) const;

			//! Returns the gradient vector at the specified point.
			void
			gradient_3(
					const delaunay_point_3_type &point,
					delaunay_map_point_to_value_3_type &function_values,
					delaunay_map_point_to_vector_3_type &function_gradients) const;
		};
	}
}

//
// Implementation
//

namespace GPlatesAppLogic
{
	namespace ResolvedTriangulation
	{
	}
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONDELAUNAY3_H
