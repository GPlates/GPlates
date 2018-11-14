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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONDELAUNAY2_H
#define GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONDELAUNAY2_H

#include <cfloat>
#include <cmath>
#include <map>
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>

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
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/interpolation_functions.h>
#include <CGAL/Interpolation_gradient_fitting_traits_2.h>
#include <CGAL/Interpolation_traits_2.h>
#include <CGAL/natural_neighbor_coordinates_2.h>
#include <CGAL/sibson_gradient_fitting.h>
#include <CGAL/Triangulation_face_base_2.h>
#include <CGAL/Triangulation_hierarchy_2.h>
#include <CGAL/Triangulation_hierarchy_vertex_base_2.h>
#include <CGAL/Triangulation_vertex_base_2.h>

POP_MSVC_WARNINGS

//POP_GCC_WARNINGS

#include "DeformationStrainRate.h"
#include "ResolvedVertexSourceInfo.h"
#include "VelocityDeltaTime.h"

#include "maths/AzimuthalEqualAreaProjection.h"
#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/Real.h"

#include "utils/Profile.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	namespace ResolvedTriangulation
	{
		/**
		 * Deformation information containing the strain rate of a triangle in triangulation
		 * or smoothed strain rate at a point over nearby triangles.
		 */
		class DeformationInfo
		{
		public:

			//! Zero strain rates (non-deforming).
			DeformationInfo()
			{  }

			explicit
			DeformationInfo(
					const DeformationStrainRate &strain_rate) :
				d_strain_rate(strain_rate)
			{  }

			/**
			 * Returns the instantaneous strain rate.
			 */
			const DeformationStrainRate &
			get_strain_rate() const
			{
				return d_strain_rate;
			}

			friend
			DeformationInfo
			operator+(
					const DeformationInfo &lhs,
					const DeformationInfo &rhs)
			{
				return DeformationInfo(lhs.d_strain_rate + rhs.d_strain_rate);
			}

			friend
			DeformationInfo
			operator*(
					double scale,
					const DeformationInfo &di)
			{
				return DeformationInfo(scale * di.d_strain_rate);
			}

			friend
			DeformationInfo
			operator*(
					const DeformationInfo &di,
					double scale)
			{
				return scale * di;
			}

		private:
			DeformationStrainRate d_strain_rate;
		};


		//
		// Basic CGAL typedefs for 2D delaunay triangulation.
		//

		typedef CGAL::Exact_predicates_inexact_constructions_kernel delaunay_kernel_2_type;
		typedef delaunay_kernel_2_type::FT delaunay_coord_2_type;

		typedef delaunay_kernel_2_type::Point_2 delaunay_point_2_type;
		typedef delaunay_kernel_2_type::Vector_2 delaunay_vector_2_type;


		typedef std::vector< std::pair<delaunay_point_2_type, delaunay_coord_2_type> >
				delaunay_point_coordinate_vector_2_type;

		//! Typedefs for interpolations in 2D
		typedef std::map<delaunay_point_2_type, delaunay_coord_2_type, delaunay_kernel_2_type::Less_xy_2 >
				delaunay_map_point_to_value_2_type;


		//! Typedef for result of a natural neighbours query on a 2D triangulation.
		typedef std::pair<delaunay_point_coordinate_vector_2_type, delaunay_coord_2_type>
				delaunay_natural_neighbor_coordinates_2_type;


		// Forward declaration.
		class Delaunay_2;


		/**
		 * This class holds the extra info for each delaunay triangulation vertex.
		 *
		 * This is based on the rebind mechanism described in the 'flexibility' section of
		 * 2D Triangulation Data Structure in the CGAL user manual.
		 *
		 * We could have instead used the simpler CGAL::Triangulation_vertex_base_with_info_2
		 * since we don't need any information based on the triangulation data structure type,
		 * but it does make dereferencing a little more direct and we do a similar thing for
		 * the triangulation *face* structure (but it's needed there) so might as well do it here too.
		 */
		template < typename GT, typename Vb = CGAL::Triangulation_vertex_base_2<GT> >
		class DelaunayVertex_2 :
				public Vb
		{
		public:
			typedef typename Vb::Face_handle                   Face_handle;
			typedef typename Vb::Vertex_handle                 Vertex_handle;
			typedef typename Vb::Point                         Point;

			template < typename TDS2 >
			struct Rebind_TDS
			{
				typedef typename Vb::template Rebind_TDS<TDS2>::Other Vb2;
				typedef DelaunayVertex_2<GT, Vb2>  Other;
			};

			DelaunayVertex_2() :
				Vb()
			{ }

			DelaunayVertex_2(
					const Point &p) :
				Vb(p)
			{ }

			DelaunayVertex_2(
					const Point &p,
					Face_handle c) :
				Vb(p, c)
			{ }

			DelaunayVertex_2(
					Face_handle c) :
				Vb(c)
			{ }


			/**
			 * Returns true if @a initialise has been called.
			 *
			 * This vertex must be initialised before any other methods can be called.
			 */
			bool
			is_initialised() const
			{
				return static_cast<bool>(d_vertex_info);
			}

			/**
			 * Set all essential vertex information in one go.
			 *
			 * You can initialise the same vertex multiple times - the last initialisation applies.
			 *
			 * Any information that is derived from this essential information is calculated as needed.
			 *
			 * With a delaunay triangulation we control the insertion of vertices so it's easy
			 * to initialise each one as we insert it. This is not easy with *constrained* delaunay
			 * triangulations because they can be meshed or made conforming which introduces new
			 * vertices that we are less aware of.
			 */
			void
			initialise(
					const Delaunay_2 &delaunay_2,
					unsigned int vertex_index,
					const GPlatesMaths::PointOnSphere &point_on_sphere,
					const GPlatesMaths::LatLonPoint &lat_lon_point,
					const ResolvedVertexSourceInfo::non_null_ptr_to_const_type &shared_source_info)
			{
				// NOTE: Can get initialised twice if an inserted vertex happens to be at the
				// same position as an existing vertex - so we don't enforce only one initialisation.

				d_vertex_info = boost::in_place(
						delaunay_2,
						vertex_index,
						point_on_sphere,
						lat_lon_point,
						shared_source_info);
			}

			//! Returns index of this vertex within all vertices in the delaunay triangulation.
			unsigned int
			get_vertex_index() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->vertex_index;
			}

			//! Returns the x/y/z vertex position un-projected from 2D coords back onto the 3D sphere.
			const GPlatesMaths::PointOnSphere &
			get_point_on_sphere() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->point_on_sphere;
			}

			//! Returns the lat/lon vertex position un-projected from 2D coords back onto the 3D sphere.
			const GPlatesMaths::LatLonPoint &
			get_lat_lon_point() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->lat_lon_point;
			}

			/**
			 * Returns the reconstruction time of this vertex's triangulation.
			 */
			const double &
			get_reconstruction_time() const
			{
				return get_delaunay_2().get_reconstruction_time();
			}

			/**
			 * Returns the shared vertex source info.
			 */
			const ResolvedVertexSourceInfo &
			get_shared_source_info() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return *d_vertex_info->shared_source_info;
			}

			//! Calculates the stage rotation of this vertex.
			GPlatesMaths::FiniteRotation
			calc_stage_rotation(
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T) const
			{
				return get_shared_source_info().get_stage_rotation(
						get_reconstruction_time(),
						velocity_delta_time,
						velocity_delta_time_type);
			}

			//! Calculates the velocity vector of this vertex.
			GPlatesMaths::Vector3D
			calc_velocity_vector(
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T) const
			{
				return get_shared_source_info().get_velocity_vector(
						get_point_on_sphere(),
						get_reconstruction_time(),
						velocity_delta_time,
						velocity_delta_time_type);
			}

			//! Calculates the velocity colat/lon of this vertex.
			GPlatesMaths::VectorColatitudeLongitude
			calc_velocity_colat_lon(
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T) const
			{
				return GPlatesMaths::convert_vector_from_xyz_to_colat_lon(
						get_point_on_sphere(),
						calc_velocity_vector(velocity_delta_time, velocity_delta_time_type));
			}


			/**
			 * Returns the deformation information of this vertex (calculates if first time called).
			 *
			 * This is the area-averaged deformation strains of the faces incident to this vertex.
			 */
			const DeformationInfo &
			get_deformation_info() const
			{
				if (!d_deformation_info)
				{
					d_deformation_info = calculate_deformation_info();
				}

				return d_deformation_info.get();
			}

		private:

			/**
			 * All information passed into @a initialise goes here.
			 */
			struct VertexInfo
			{
				VertexInfo(
						const Delaunay_2 &delaunay_2_,
						unsigned int vertex_index_,
						const GPlatesMaths::PointOnSphere &point_on_sphere_,
						const GPlatesMaths::LatLonPoint &lat_lon_point_,
						const ResolvedVertexSourceInfo::non_null_ptr_to_const_type &shared_source_info_) :
					delaunay_2(delaunay_2_),
					vertex_index(vertex_index_),
					point_on_sphere(point_on_sphere_),
					lat_lon_point(lat_lon_point_),
					shared_source_info(shared_source_info_)
				{  }

				const Delaunay_2 &delaunay_2;
				unsigned int vertex_index;
				GPlatesMaths::PointOnSphere point_on_sphere;
				GPlatesMaths::LatLonPoint lat_lon_point;
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type shared_source_info;
			};

			boost::optional<VertexInfo> d_vertex_info;

			// Derived values - these are mutable since they are calculated on first call.
			mutable boost::optional<DeformationInfo> d_deformation_info;

			// Compute the deformation info for this vertex.
			DeformationInfo
			calculate_deformation_info() const;

			const Delaunay_2 &
			get_delaunay_2() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->delaunay_2;
			}

			Vertex_handle
			get_handle() const
			{
				// Using 'this->' to make 'vertex()' a dependent name and hence delay name lookup
				// until instantiation - otherwise we get a compile error.
				const Face_handle fh = this->face();
				for (int i = 0 ; i < 3 ; ++i)
				{
					Vertex_handle vh = fh->vertex(i);
					if (&*vh == this)
					{
						return vh;
					}
				}

				// Shouldn't be able to get here.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				return Vertex_handle();
			}
		};


		/**
		 * This class holds the extra info for each delaunay triangulation face.
		 *
		 * This is based on the rebind mechanism described in the 'flexibility' section of
		 * 2D Triangulation Data Structure in the CGAL user manual.
		 *
		 * We use this instead of the simpler CGAL::Triangulation_face_base_with_info_2
		 * because we want to keep vertex handles in our face structure in order to
		 * directly access the triangle vertices.
		 */
		template < typename GT, typename Fb = CGAL::Triangulation_face_base_2<GT> >
		class DelaunayFace_2 :
				public Fb
		{
		public:
			typedef typename Fb::Vertex_handle                   Vertex_handle;
			typedef typename Fb::Face_handle                     Face_handle;

			template < typename TDS2 >
			struct Rebind_TDS
			{
				typedef typename Fb::template Rebind_TDS<TDS2>::Other Fb2;
				typedef DelaunayFace_2<GT, Fb2>  Other;
			};

			DelaunayFace_2() :
				Fb()
			{ }

			DelaunayFace_2(
					Vertex_handle v0, 
					Vertex_handle v1,
					Vertex_handle v2) :
				Fb(v0, v1, v2)
			{ }

			DelaunayFace_2(
					Vertex_handle v0, 
					Vertex_handle v1,
					Vertex_handle v2, 
					Face_handle n0, 
					Face_handle n1,
					Face_handle n2) :
				Fb(v0, v1, v2, n0, n1, n2)
			{ }


			/**
			 * Set all essential face information in one go.
			 *
			 * Any derived information can be calculated as needed.
			 */
			void
			initialise(
					const Delaunay_2 &delaunay_2,
					unsigned int face_index,
					bool is_in_deforming_region_)
			{
				// Make sure only gets initialised once.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						!d_face_info,
						GPLATES_ASSERTION_SOURCE);

				d_face_info = boost::in_place(delaunay_2, face_index, is_in_deforming_region_);
			}

			//! Returns index of this face within all faces in the delaunay triangulation.
			unsigned int
			get_face_index() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_face_info,
						GPLATES_ASSERTION_SOURCE);
				return d_face_info->face_index;
			}

			/**
			 * Returns true if face is inside the deforming region.
			 *
			 * The delaunay triangulation is the convex hull around the network boundary,
			 * so it includes faces outside the network boundary (and also faces inside any
			 * non-deforming interior blocks).
			 *
			 * If the centroid of this face is inside the deforming region then true is returned.
			 *
			 * TODO: Note that the delaunay triangulation is *not* constrained which means some
			 * delaunay faces can cross over network boundary edges or interior block edges.
			 * This is something that perhaps needs to be dealt with, but currently doesn't appear
			 * to be too much of a problem with current topological network datasets.
			 */
			bool
			is_in_deforming_region() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_face_info,
						GPLATES_ASSERTION_SOURCE);
				return d_face_info->is_in_deforming_region;
			}

			/**
			 * Returns the deformation information of this face (calculates if first time called).
			 *
			 * This is the constant deformation strain across this face.
			 */
			const DeformationInfo &
			get_deformation_info() const
			{
				if (!d_deformation_info)
				{
					d_deformation_info = calculate_deformation_info();
				}

				return d_deformation_info.get();
			}

		private:
			
			/**
			 * All information passed into @a initialise goes here.
			 */
			struct FaceInfo
			{
				FaceInfo(
						const Delaunay_2 &delaunay_2_,
						unsigned int face_index_,
						bool is_in_deforming_region_) :
					delaunay_2(delaunay_2_),
					face_index(face_index_),
					is_in_deforming_region(is_in_deforming_region_)
				{  }

				const Delaunay_2 &delaunay_2;
				unsigned int face_index;
				bool is_in_deforming_region;
			};


			// The extra info for the face
			boost::optional<FaceInfo> d_face_info;

			// Derived values - these are mutable since they are calculated on first call.
			mutable boost::optional<DeformationInfo> d_deformation_info;

			// Compute the deformation info for this face.
			DeformationInfo
			calculate_deformation_info() const;

			const Delaunay_2 &
			get_delaunay_2() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_face_info,
						GPLATES_ASSERTION_SOURCE);
				return d_face_info->delaunay_2;
			}
		};


		/**
		 * Put part of DelaunayFace_2<GT, Fb>::calculate_deformation_info() in the '.cc' file
		 * to avoid lengthy recompile times each time it's modified.
		 */
		DeformationInfo
		calculate_face_deformation_info(
				const Delaunay_2 &delaunay_2,
				const double &theta1,
				const double &theta2,
				const double &theta3,
				const double &theta_centroid,
				const double &phi1,
				const double &phi2,
				const double &phi3,
				const double &phi_centroid,
				const double &utheta1,
				const double &utheta2,
				const double &utheta3,
				const double &utheta_centroid,
				const double &uphi1,
				const double &uphi2,
				const double &uphi3,
				const double &uphi_centroid);


		/**
		 * Vertex type with extra vertex info for delaunay triangulation.
		 *
		 * NOTE: We wrap it in a CGAL::Triangulation_hierarchy_vertex_base_2 because our
		 * delaunay triangulation is wrapped in a CGAL::Triangulation_hierarchy_2.
		 *
		 * NOTE: Avoid compiler warning 4503 'decorated name length exceeded' in Visual Studio 2008.
		 * Seems we get the warning, which gets (correctly) treated as error due to /WX switch,
		 * even if we disable the 4503 warning. So prevent warning by reducing name length of
		 * identifier - which we do by inheritance instead of using a typedef.
		 */
		struct delaunay_triangulation_vertex_2_type :
				public CGAL::Triangulation_hierarchy_vertex_base_2<
						DelaunayVertex_2<delaunay_kernel_2_type> >
		{  };

		//! Face type with extra face info for delaunay triangulation.
		typedef DelaunayFace_2<delaunay_kernel_2_type> delaunay_triangulation_face_2_type;

		//! 2D Triangle data structure, with extra info for each vertex and face
		typedef CGAL::Triangulation_data_structure_2<
				delaunay_triangulation_vertex_2_type,
				delaunay_triangulation_face_2_type>
						delaunay_triangulation_data_structure_2_type;


		/**
		 * 2D Delaunay triangulation.
		 */
		class Delaunay_2 :
				public CGAL::Triangulation_hierarchy_2<
						CGAL::Delaunay_triangulation_2<
								delaunay_kernel_2_type,
								delaunay_triangulation_data_structure_2_type> >
		{
		public:

			Delaunay_2(
					const GPlatesMaths::AzimuthalEqualAreaProjection &projection,
					const double &reconstruction_time,
					boost::optional<double> clamp_total_strain_rate = boost::none) :
				d_projection(projection),
				d_reconstruction_time(reconstruction_time),
				d_clamp_total_strain_rate(clamp_total_strain_rate)
			{  }

			/**
			 * Returns the natural neighbor coordinates of @a point in the triangulation
			 * (which can then be used with different interpolation methods like linear interpolation).
			 *
			 * Returns false if @a point is outside the triangulation.
			 *
			 * NOTE: It appears that CGAL can trigger assertions under certain situations (at certain query points).
			 * This is most likely due to us not using exact arithmetic in our delaunay triangulation
			 * (currently we use 'CGAL::Exact_predicates_inexact_constructions_kernel').
			 * The assertion seems to manifest as a normalisation factor of zero.
			 * We currently handle this by instead querying the barycentric coordinates and
			 * converting them to natural neighbour coordinates.
			 */
			bool
			calc_natural_neighbor_coordinates(
					delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
					const delaunay_point_2_type &point,
					Face_handle start_face_hint = Face_handle()) const;

			/**
			 * Returns the barycentric coordinates of @a point in the triangulation along with
			 * the face containing @a point.
			 *
			 * The coordinates sum to 1.0.
			 *
			 * Returns boost::none if @a point is outside the triangulation.
			 */
			boost::optional<Face_handle>
			calc_barycentric_coordinates(
					delaunay_coord_2_type &barycentric_coord_vertex_1,
					delaunay_coord_2_type &barycentric_coord_vertex_2,
					delaunay_coord_2_type &barycentric_coord_vertex_3,
					const delaunay_point_2_type &point,
					Face_handle start_face_hint = Face_handle()) const;

			/**
			 * Returns the face containing @a point.
			 *
			 * Returns boost::none if @a point is outside the triangulation.
			 */
			boost::optional<Face_handle>
			get_face_containing_point(
					const delaunay_point_2_type &point,
					Face_handle start_face_hint = Face_handle()) const;

			//! Returns the gradient vector at the specified point.
			delaunay_vector_2_type
			gradient_2(
					const delaunay_point_2_type &point,
					const delaunay_map_point_to_value_2_type &function_values) const;


			/**
			 * Returns the projection used by this triangulation to convert from 3D points to
			 * 2D points and vice versa.
			 */
			const GPlatesMaths::AzimuthalEqualAreaProjection &
			get_projection() const
			{
				return d_projection;
			}

			/**
			 * Returns the reconstruction time.
			 */
			const double &
			get_reconstruction_time() const
			{
				return d_reconstruction_time;
			}

			/**
			 * Returns the optional maximum total strain rate (2nd invariant).
			 */
			boost::optional<double>
			get_clamp_total_strain_rate() const
			{
				return d_clamp_total_strain_rate;
			}

		private:

			GPlatesMaths::AzimuthalEqualAreaProjection d_projection;
			double d_reconstruction_time;
			boost::optional<double> d_clamp_total_strain_rate;
		};
	}


	//
	// Implementation
	//


	namespace ResolvedTriangulation
	{
		//
		// Vertex Implementation
		//

		template < typename GT, typename Vb >
		DeformationInfo
		DelaunayVertex_2<GT, Vb>::calculate_deformation_info() const
		{
			const Delaunay_2 &delaunay_2 = get_delaunay_2();

			DeformationInfo vertex_deformation_info;
			double area_sum = 0.0;

			// Circulate over the faces incident to this vertex.
			const Delaunay_2::Face_circulator incident_face_circulator_start =
					delaunay_2.incident_faces(get_handle());
			Delaunay_2::Face_circulator incident_face_circulator = incident_face_circulator_start;
			do
			{
				// Ignore the infinite face - we're at the edge of the convex hull so one (or two?)
				// adjacent face(s) will be the infinite face.
				if (delaunay_2.is_infinite(incident_face_circulator) ||
					// Also ignore faces that are outside the deforming region
					// (outside network boundary or inside non-deforming interior rigid blocks).
					//
					// Previously we did *not* ignore these faces because it's possible for there to be
					// extremely tiny faces in the delaunay triangulation (eg, if a topological section
					// has adjacent vertices very close together) and the strain rate on these faces tends
					// to be much larger than normal (presumably due to the accuracy of calculations) and
					// including the larger faces outside the deforming region (which have zero strain rates)
					// causes the face-area-average of strain rate to significantly reduce the
					// contribution of the tiny face (with the much larger strain rate).
					//
					// However the user now has optional strain rate clamping to deal with these artifacts
					// so we return to ignoring faces outside deforming region as we should.
					!incident_face_circulator->is_in_deforming_region())
				{
					continue;
				}

				// Get the area of the face triangle.
				const double area = std::fabs(delaunay_2.triangle(incident_face_circulator).area());

				// Get the deformation data for the current face.
				const DeformationInfo &face_deformation_info = incident_face_circulator->get_deformation_info();

				vertex_deformation_info = vertex_deformation_info + area * face_deformation_info;
				area_sum += area;
			}
			while (++incident_face_circulator != incident_face_circulator_start);

			if (GPlatesMaths::are_almost_exactly_equal(area_sum, 0.0))
			{
				// The incident faces all had zero area for some reason.
				return DeformationInfo();
			}

			return (1.0 / area_sum) * vertex_deformation_info;
		}


		//
		// Face Implementation
		//

		template < typename GT, typename Fb >
		DeformationInfo
		DelaunayFace_2<GT, Fb>::calculate_deformation_info() const
		{
			if (!d_face_info->is_in_deforming_region)
			{
				// Not in the deforming region so return zero strain rates.
				return DeformationInfo();
			}
			const Delaunay_2 &delaunay_2 = get_delaunay_2();

			// NOTE: array indices 0,1,2 in CGAL code correspond to triangle vertex numbers 1,2,3 

			// Using 'this->' to make 'vertex()' a dependent name and hence delay name lookup
			// until instantiation - otherwise we get a compile error.
			Vertex_handle v1 = this->vertex(0);
			Vertex_handle v2 = this->vertex(1);
			Vertex_handle v3 = this->vertex(2);

			// Position and velocity at vertex 1.
			// NOTE: y velocities are colat, down from the pole, and have to have sign change for North South uses.
			const GPlatesMaths::LatLonPoint &v1_llp = v1->get_lat_lon_point();
			double phi1 = v1_llp.longitude();
			double theta1 = 90.0 - v1_llp.latitude();
			const GPlatesMaths::VectorColatitudeLongitude u1 = v1->calc_velocity_colat_lon();
			double uphi1 = u1.get_vector_longitude().dval();
			double utheta1 = u1.get_vector_colatitude().dval();

			// Position and velocity at vertex 2.
			// NOTE: y velocities are colat, down from the pole, and have to have sign change for North South uses.
			const GPlatesMaths::LatLonPoint &v2_llp = v2->get_lat_lon_point();
			double phi2 = v2_llp.longitude();
			double theta2 = 90.0 - v2_llp.latitude();
			const GPlatesMaths::VectorColatitudeLongitude u2 = v2->calc_velocity_colat_lon();
			double uphi2 = u2.get_vector_longitude().dval();
			double utheta2 = u2.get_vector_colatitude().dval();

			// Position and velocity at vertex 3.
			// NOTE: y velocities are colat, down from the pole, and have to have sign change for North South uses.
			const GPlatesMaths::LatLonPoint &v3_llp = v3->get_lat_lon_point();
			double phi3 = v3_llp.longitude();
			double theta3 = 90.0 - v3_llp.latitude();
			const GPlatesMaths::VectorColatitudeLongitude u3 = v3->calc_velocity_colat_lon();
			double uphi3 = u3.get_vector_longitude().dval();
			double utheta3 = u3.get_vector_colatitude().dval();

			// Face centroid.
			const double inv_3 = 1.0 / 3.0;
			const double x_centroid = inv_3 * (v1->point().x() + v2->point().x() + v3->point().x());
			const double y_centroid = inv_3 * (v1->point().y() + v2->point().y() + v3->point().y());

			const GPlatesMaths::AzimuthalEqualAreaProjection &projection = delaunay_2.get_projection();

			const GPlatesMaths::LatLonPoint centroid_llp =
					projection.unproject_to_lat_lon(QPointF(x_centroid, y_centroid));

			// The colatitude/longitude coordinates of the face centroid.
			double phi_centroid = centroid_llp.longitude();
			double theta_centroid = 90.0 - centroid_llp.latitude();

			// Velocity at face centroid.
			double uphi_centroid = inv_3 * (uphi1 + uphi2 + uphi3);
			double utheta_centroid = inv_3 * (utheta1 + utheta2 + utheta3);

			// Convert spherical coordinates from degrees to radians.
			phi_centroid = GPlatesMaths::convert_deg_to_rad( phi_centroid );
			phi1 = GPlatesMaths::convert_deg_to_rad( phi1 );
			phi2 = GPlatesMaths::convert_deg_to_rad( phi2 );
			phi3 = GPlatesMaths::convert_deg_to_rad( phi3 );

			theta_centroid = GPlatesMaths::convert_deg_to_rad( theta_centroid  );
			theta1 = GPlatesMaths::convert_deg_to_rad( theta1 );
			theta2 = GPlatesMaths::convert_deg_to_rad( theta2 );
			theta3 = GPlatesMaths::convert_deg_to_rad( theta3 );

			// Scale velocity values from cm/yr to m/s.
			const double inv_velocity_scale = 1.0 / 3.1536e09;

			uphi_centroid = uphi_centroid * inv_velocity_scale;
			uphi1 = uphi1 * inv_velocity_scale;
			uphi2 = uphi2 * inv_velocity_scale;
			uphi3 = uphi3 * inv_velocity_scale;

			utheta_centroid = utheta_centroid * inv_velocity_scale;
			utheta1 = utheta1 * inv_velocity_scale;
			utheta2 = utheta2 * inv_velocity_scale;
			utheta3 = utheta3 * inv_velocity_scale;

			return calculate_face_deformation_info(
					delaunay_2,
					theta1, theta2, theta3, theta_centroid,
					phi1, phi2, phi3, phi_centroid,
					utheta1, utheta2, utheta3, utheta_centroid,
					uphi1, uphi2, uphi3, uphi_centroid);
		}
	}
}


#endif // GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONDELAUNAY2_H
