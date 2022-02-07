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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONCONSTRAINEDDELAUNAY2_H
#define GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONCONSTRAINEDDELAUNAY2_H

#include <map>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include <QDebug>

#include "global/AssertionFailureException.h"
#include "global/config.h"
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
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/interpolation_functions.h>
#include <CGAL/Interpolation_gradient_fitting_traits_2.h>
#include <CGAL/Interpolation_traits_2.h>
#include <CGAL/natural_neighbor_coordinates_2.h>
#include <CGAL/sibson_gradient_fitting.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Triangulation_face_base_2.h>
#include <CGAL/Triangulation_hierarchy_2.h>
#include <CGAL/Triangulation_hierarchy_vertex_base_2.h>
#include <CGAL/Triangulation_vertex_base_2.h> 

POP_MSVC_WARNINGS

//POP_GCC_WARNINGS

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace ResolvedTriangulation
	{
		//
		// Basic CGAL typedefs for 2D constrained delaunay triangulation.
		//

		typedef CGAL::Exact_predicates_inexact_constructions_kernel constrained_delaunay_kernel_2_type;
		typedef constrained_delaunay_kernel_2_type::FT constrained_delaunay_coord_2_type;

		typedef constrained_delaunay_kernel_2_type::Point_2 constrained_delaunay_point_2_type;
		typedef constrained_delaunay_kernel_2_type::Vector_2 constrained_delaunay_vector_2_type;


		typedef std::vector<
				std::pair<constrained_delaunay_point_2_type, constrained_delaunay_coord_2_type> >
						constrained_delaunay_point_coordinate_vector_2_type;

		//! Typedefs for interpolations in 2D
		typedef std::map<
				constrained_delaunay_point_2_type,
				constrained_delaunay_coord_2_type,
				constrained_delaunay_kernel_2_type::Less_xy_2 >
						constrained_delaunay_map_point_to_value_2_type;

		typedef std::map<
				constrained_delaunay_point_2_type,
				constrained_delaunay_vector_2_type,
				constrained_delaunay_kernel_2_type::Less_xy_2 >
						constrained_delaunay_map_point_to_vector_2_type;


		//! Typedef for result of a natural neighbours query on a 2D triangulation.
		typedef std::pair<
				constrained_delaunay_point_coordinate_vector_2_type,
				constrained_delaunay_coord_2_type>
						constrained_delaunay_natural_neighbor_coordinates_2_type;


		/**
		 * Vertex type for constrained delaunay triangulation.
		 *
		 * NOTE: We wrap it in a CGAL::Triangulation_hierarchy_vertex_base_2 because our
		 * constrained delaunay triangulation is wrapped in a CGAL::Triangulation_hierarchy_2.
		 *
		 * NOTE: Avoid compiler warning 4503 'decorated name length exceeded' in Visual Studio 2008.
		 * Seems we get the warning, which gets (correctly) treated as error due to /WX switch,
		 * even if we disable the 4503 warning. So prevent warning by reducing name length of
		 * identifier - which we do by inheritance instead of using a typedef.
		 */
		struct constrained_delaunay_triangulation_vertex_2_type :
				public CGAL::Triangulation_hierarchy_vertex_base_2<
						CGAL::Triangulation_vertex_base_2<constrained_delaunay_kernel_2_type> >
		{  };

		//! Face type for constrained delaunay triangulation that are meshed.
		typedef CGAL::Delaunay_mesh_face_base_2<constrained_delaunay_kernel_2_type>
				constrained_delaunay_triangulation_face_2_type;

		//! 2D + Constraints Triangle data structure.
		typedef CGAL::Triangulation_data_structure_2<
				constrained_delaunay_triangulation_vertex_2_type,
				constrained_delaunay_triangulation_face_2_type>
						constrained_delaunay_triangulation_data_structure_2_type;


		/**
		 * 2D constrained Delaunay triangulation.
		 */
		class ConstrainedDelaunay_2 :
				public CGAL::Triangulation_hierarchy_2<
						CGAL::Constrained_Delaunay_triangulation_2<
								constrained_delaunay_kernel_2_type,
								constrained_delaunay_triangulation_data_structure_2_type,
								CGAL::Exact_predicates_tag> >
		{
		public:

			/**
			 * Returns true if the specified 2D point is within the meshed domain of the triangulation.
			 */
			bool
			is_point_in_mesh(
					const constrained_delaunay_point_2_type &point) const;

			/**
			 * Returns the natural neighbor coordinates of @a point in the triangulation
			 * (which can then be used with different interpolation methods like linear interpolation).
			 *
			 * Returns false if @a point is outside the triangulation.
			 *
			 * NOTE: This currently requires a CGAL patch to work (otherwise always returns false).
			 */
			bool
			calc_natural_neighbor_coordinates(
					constrained_delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
					const constrained_delaunay_point_2_type &point) const;
		};


		/**
		 * Constrained Delaunay triangulation mesher.
		 *
		 * NOTE:
		 * flag info from :
		 * http://www.cgal.org/Manual/last/doc_html/cgal_manual/Mesh_2_ref/Class_Delaunay_mesher_2.html
		 * If true,  the mesh domain is the union of the bounded connected components including at least one seed. 
		 * If false, the mesh domain is the union of the bounded components including no seed. 
		 */
		class ConstrainedDelaunayMesher_2 :
				public CGAL::Delaunay_mesher_2<
						ConstrainedDelaunay_2,
						CGAL::Delaunay_mesh_size_criteria_2<ConstrainedDelaunay_2> >
		{
		private:

			typedef CGAL::Delaunay_mesher_2<
						ConstrainedDelaunay_2,
						CGAL::Delaunay_mesh_size_criteria_2<ConstrainedDelaunay_2> >
								base_type;

		public:

			ConstrainedDelaunayMesher_2(
					ConstrainedDelaunay_2 &constrained_delaunay_triangulation,
					const Criteria &criteria_ = Criteria()) :
				base_type(constrained_delaunay_triangulation, criteria_)
			{  }

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

/* 
NOTE: save this for reference.
From CGAL-3.5/examples/Mesh_2/mesh_with_seeds.cpp, and CGAL-3.5/examples/Mesh_2/mesh_class.cpp
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangulation_vertex_base_2<K> Vb;
typedef CGAL::Delaunay_mesh_face_base_2<K> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds> CDT;
typedef CGAL::Delaunay_mesh_size_criteria_2<CDT> Criteria;
typedef CGAL::Delaunay_mesher_2<CDT, Criteria> Mesher;
*/

#if 0
// NOTE: see this example:
http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Triangulation_2/Chapter_main.html
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/IO/Color.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangulation_vertex_base_2<K> Vb;
typedef CGAL::Triangulation_face_base_with_info_2<CGAL::Color,K> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb,Fb> Tds;
typedef CGAL::Triangulation_2<K,Tds> Triangulation;
typedef Triangulation::Face_handle Face_handle;
typedef Triangulation::Finite_faces_iterator Finite_faces_iterator;
typedef Triangulation::Point  Point;
#endif

#endif // GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONCONSTRAINEDDELAUNAY2_H
