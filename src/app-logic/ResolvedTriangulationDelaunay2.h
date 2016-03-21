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

#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"
#include "VelocityDeltaTime.h"

#include "model/types.h"

#include "maths/CalculateVelocity.h"
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/Centroid.h"

#include "utils/Profile.h"


#ifdef _MSC_VER
#ifndef copysign
#define copysign _copysign
#endif
#endif


namespace GPlatesAppLogic
{
	namespace ResolvedTriangulation
	{
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
				return d_vertex_info;
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
					const GPlatesMaths::PointOnSphere &point_on_sphere,
					GPlatesModel::integer_plate_id_type plate_id,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const double &reconstruction_time)
			{
				// NOTE: Can get initialised twice if an inserted vertex happens to be at the
				// same position as an existing vertex - so we don't enforce only one initialisation.

				d_vertex_info = boost::in_place(
						point_on_sphere,
						plate_id,
						reconstruction_tree_creator,
						reconstruction_time);
			}

			//! Returns the vertex position un-projected from 2D coords back onto the 3D sphere.
			const GPlatesMaths::PointOnSphere &
			get_point_on_sphere() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->point_on_sphere;
			}

			//! Returns plate id associated with this vertex.
			GPlatesModel::integer_plate_id_type
			get_plate_id() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->plate_id;
			}

			/**
			 * Returns the reconstruction time of this vertex's triangulation.
			 */
			const double &
			get_reconstruction_time() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->reconstruction_time;
			}

			/**
			 * Returns the ReconstructionTreeCreator associated with this vertex.
			 */
			ReconstructionTreeCreator
			get_reconstruction_tree_creator() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->reconstruction_tree_creator;
			}

			//! Calculates the velocity vector of this vertex.
			GPlatesMaths::Vector3D
			calc_velocity_vector(
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T) const;

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


			const double &
			get_dilitation() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->dilitation;
			}

			void
			set_dilitation(
					const double &dilitation)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				d_vertex_info->dilitation = dilitation;
			}


			const double &
			get_second_invariant() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->second_invariant;
			}

			void
			set_second_invariant(
					const double &second_invariant)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				d_vertex_info->second_invariant = second_invariant;
			}


			const double &
			get_sph_dilitation() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->sph_dilitation;
			}

			void
			set_sph_dilitation(
					const double &sph_dilitation)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				d_vertex_info->sph_dilitation = sph_dilitation;
			}


			const double &
			get_sph_second_invariant() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				return d_vertex_info->sph_second_invariant;
			}

			void
			set_sph_second_invariant(
					const double &sph_second_invariant)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_vertex_info,
						GPLATES_ASSERTION_SOURCE);
				d_vertex_info->sph_second_invariant = sph_second_invariant;
			}

		private:

			/**
			 * All information passed into @a initialise goes here.
			 */
			struct VertexInfo
			{
				VertexInfo(
						const GPlatesMaths::PointOnSphere &point_on_sphere_,
						GPlatesModel::integer_plate_id_type plate_id_,
						const ReconstructionTreeCreator &reconstruction_tree_creator_,
						const double &reconstruction_time_,
						const double &dilitation_ = 0.0,
						const double &second_invariant_ = 0.0,
						const double &sph_dilitation_ = 0.0,
						const double &sph_second_invariant_ = 0.0) :
					point_on_sphere(point_on_sphere_),
					plate_id(plate_id_),
					reconstruction_tree_creator(reconstruction_tree_creator_),
					reconstruction_time(reconstruction_time_),
					dilitation(dilitation_),
					second_invariant(second_invariant_),
					sph_dilitation(sph_dilitation_),
					sph_second_invariant(sph_second_invariant_)
				{  }

				GPlatesMaths::PointOnSphere point_on_sphere;
				GPlatesModel::integer_plate_id_type plate_id;
				ReconstructionTreeCreator reconstruction_tree_creator;

				double reconstruction_time;

				double dilitation;
				double second_invariant;
				double sph_dilitation;
				double sph_second_invariant;
			};

			boost::optional<VertexInfo> d_vertex_info;
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
					int face_index)
			{
				// Make sure only gets initialised once.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						!d_face_info,
						GPLATES_ASSERTION_SOURCE);

				d_face_info = boost::in_place(face_index);
			}

			//! Returns face index of this face.
			int
			get_face_index() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_face_info,
						GPLATES_ASSERTION_SOURCE);
				return d_face_info->face_index;
			}

			//! Returns the centroid as a POS of this face (calculates if first time called).
			const GPlatesMaths::PointOnSphere &
			get_centroid() const
			{
				if (!d_centroid)
				{
					calculate_centroid();
				}
				return d_centroid.get();
			}


			/**
			 * Deformation information for this face of the triangulation.
			 */
			struct DeformationInfo
			{
				DeformationInfo(
						const double &SR22_,
						const double &SR33_,
						const double &SR23_,
						const double &SR_DIR_,
						const double &SR1_,
						const double &SR2_,
						const double &dilitation_,
						const double &second_invariant_,
						const double &smooth_dilitation_,
						const double &smooth_second_invariant_,
						const double &sph_dilitation_,
						const double &sph_second_invariant_,
						const double &sph_smooth_dilitation_,
						const double &sph_smooth_second_invariant_
				) :
					SR22(SR22_),
					SR33(SR33_),
					SR23(SR23_),
					SR_DIR(SR_DIR_),
					SR1(SR1_),
					SR2(SR2_),
					dilitation(dilitation_),
					second_invariant(second_invariant_),
					smooth_dilitation(smooth_dilitation_),
					smooth_second_invariant(smooth_second_invariant_),
					sph_dilitation(sph_dilitation_),
					sph_second_invariant(sph_second_invariant_),
					sph_smooth_dilitation(sph_smooth_dilitation_),
					sph_smooth_second_invariant(sph_smooth_second_invariant_)
				{  }

				// instantaneous strain rates
                //double SR_sph; // spherical strain rate
				double SR22; // 2 = x
				double SR33; // 3 = y
				double SR23;

				// instantaneous rotations
				// Not yet needed 
				//double ROT23;
				//double ROT32;

				// other strain rates
				double SR_DIR; // Strain Rate direction
				double SR1; 	// principle strain 1 = compression ? 
				double SR2; 	// principle strain 2 = tension ? FIXME: verify this ?
				double dilitation;
				double second_invariant;
				double smooth_dilitation;
				double smooth_second_invariant;
				double sph_dilitation;
				double sph_second_invariant;
				double sph_smooth_dilitation;
				double sph_smooth_second_invariant;
			};

			//! Returns the deformation information of this face (calculates if first time called).
			const DeformationInfo &
			get_deformation_info() const
			{
				if (!d_deformation_info)
				{
					calculate_deformation_info();
				}

				return d_deformation_info.get();
			}

			//! Returns the non const deformation information of this face 
			DeformationInfo &
			get_deformation_info_non_const()
			{
				return d_deformation_info.get();
			}

			//! Set the deformation information of this face 
			void
			set_deformation_info( DeformationInfo &d )
			{
				d_deformation_info = d;
			}

		private:
			
			/**
			 * All information passed into @a initialise goes here.
			 */
			struct FaceInfo
			{
				explicit
				FaceInfo(
						int face_index_) :
					face_index(face_index_)
				{  }

				int face_index;
			};

			// The centroid of the face
			mutable boost::optional<GPlatesMaths::PointOnSphere> d_centroid;

			// The extra info for the face
			boost::optional<FaceInfo> d_face_info;

			// Derived values - these are mutable since they are calculated on first call.
			mutable boost::optional<DeformationInfo> d_deformation_info;

			// Compute the deformation info for this face
			void
			calculate_deformation_info() const;

			// Compute the centroid for this face
			void 
			calculate_centroid() const;
		};


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

			/**
			 * Returns the natural neighbor coordinates of @a point in the triangulation
			 * (which can then be used with different interpolation methods like linear interpolation).
			 *
			 * Returns false if @a point is outside the triangulation.
			 */
			bool
			calc_natural_neighbor_coordinates(
					delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
					const delaunay_point_2_type &point) const;

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
					const delaunay_point_2_type &point) const;

			//! Returns the gradient vector at the specified point.
			delaunay_vector_2_type
			gradient_2(
					const delaunay_point_2_type &point,
					const delaunay_map_point_to_value_2_type &function_values) const;

		};
	}


	//
	// Implementation
	//


	namespace ResolvedTriangulation
	{
		//
		// Vertx Implementation
		//

		template < typename GT, typename Vb >
		GPlatesMaths::Vector3D
		DelaunayVertex_2<GT, Vb>::calc_velocity_vector(
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type) const
		{
			//
			// Currently we assume the geometry was reconstructed by plate id which precludes,
			// for example, half-stage rotations (eg, MOR). This is currently OK since we only use
			// this for calculating velocities at topological *network* boundaries/interiors where
			// we don't expect a MOR (mid-ocean ridge) to form part of the boundary of a network.
			//

			const GPlatesModel::integer_plate_id_type vertex_plate_id = get_plate_id();
			const double &vertex_reconstruction_time = get_reconstruction_time();
			const std::pair<double, double> time_range = VelocityDeltaTime::get_time_range(
					velocity_delta_time_type, vertex_reconstruction_time, velocity_delta_time);
			const ReconstructionTreeCreator vertex_reconstruction_tree_creator = get_reconstruction_tree_creator();

			// Get the reconstruction trees to calculate velocity with.
			const ReconstructionTree::non_null_ptr_to_const_type vertex_recon_tree1 =
					vertex_reconstruction_tree_creator.get_reconstruction_tree(time_range.second/*young*/);
			const ReconstructionTree::non_null_ptr_to_const_type vertex_recon_tree2 =
					vertex_reconstruction_tree_creator.get_reconstruction_tree(time_range.first/*old*/);

			// Get the finite rotations for this plate id.
			const GPlatesMaths::FiniteRotation &vertex_finite_rotation1 =
					vertex_recon_tree1->get_composed_absolute_rotation(vertex_plate_id).first;
			const GPlatesMaths::FiniteRotation &vertex_finite_rotation2 =
					vertex_recon_tree2->get_composed_absolute_rotation(vertex_plate_id).first;

			return GPlatesMaths::calculate_velocity_vector(
					get_point_on_sphere(),
					vertex_finite_rotation1,
					vertex_finite_rotation2);
		}


		//
		// Face Implementation
		//

		template < typename GT, typename Fb >
		void
		DelaunayFace_2<GT, Fb>::calculate_centroid() const
		{
			// Calculate the centroid of this face 
	
			// Get the vertices as PointOnSphere 's and add them to a vector
			std::vector<GPlatesMaths::PointOnSphere> points;
			points.push_back( this->vertex(0)->get_point_on_sphere() );
			points.push_back( this->vertex(1)->get_point_on_sphere() );
			points.push_back( this->vertex(2)->get_point_on_sphere() );

			// compute the centroid 
			GPlatesMaths::UnitVector3D centroid_uv3d =
					GPlatesMaths::Centroid::calculate_points_centroid(points.begin(), points.end() );

			const GPlatesMaths::PointOnSphere centroid = GPlatesMaths::PointOnSphere(centroid_uv3d);
			d_centroid = centroid;
		}


		template < typename GT, typename Fb >
		void
		DelaunayFace_2<GT, Fb>::calculate_deformation_info() const
		{
			// NOTE: array indices 0,1,2 in CGAL code coorespond to triangle vertex numbers 1,2,3 

			// Using 'this->' to make 'vertex()' a dependent name and hence delay name lookup
			// until instantiation - otherwise we get a compile error.
			Vertex_handle v1 = this->vertex(0);
			Vertex_handle v2 = this->vertex(1);
			Vertex_handle v3 = this->vertex(2);

			// Get vertex coordinates data as projected x,y coords and
			// get velocity data, u, for each vertex.
   
            // NOTE: y velocities are colat, down from the pole, 
            //       and have to have sign change for North South uses 

			// vertex 1
			const double x1 = v1->point().x();
			const double y1 = v1->point().y();
			double ux1 = v1->calc_velocity_colat_lon().get_vector_longitude().dval();
			double uy1 = -( v1->calc_velocity_colat_lon().get_vector_colatitude().dval() );

			// vertex 2
			const double x2 = v2->point().x();
			const double y2 = v2->point().y();
			double ux2 = v2->calc_velocity_colat_lon().get_vector_longitude().dval();
			double uy2 = -( v2->calc_velocity_colat_lon().get_vector_colatitude().dval() );

			// vertex 3
			const double x3 = v3->point().x();
			const double y3 = v3->point().y();
			double ux3 = v3->calc_velocity_colat_lon().get_vector_longitude().dval();
			double uy3 = -( v3->calc_velocity_colat_lon().get_vector_colatitude().dval() );

			// Establish coeficients for linear interpolation over the element
			// x,y data is in meters from projection;
			//
			//a1 = x2*y3 - x3*y2;
			//a2 = x3*y1 - x1*y3;
			//a3 = x1*y2 - x2*y1;
			const double b1 = y2-y3;
			const double b2 = y3-y1;
			const double b3 = y1-y2;
			const double c1 = x3-x2;
			const double c2 = x1-x3;
			const double c3 = x2-x1;

			// det is 2*(area of triangular element)
			const double det = x2*y3 + x1*y2 + y1*x3 - y1*x2 - x1*y3 - y2*x3;
			const double inv_det = 1.0 / det;

			// Scale velocity values from cm/yr to m/s.
			const double inv_velocity_scale = 1.0 / 3.1536e09;
			ux1 = ux1 * inv_velocity_scale;
			uy1 = uy1 * inv_velocity_scale;
			ux2 = ux2 * inv_velocity_scale;
			uy2 = uy2 * inv_velocity_scale;
			ux3 = ux3 * inv_velocity_scale;
			uy3 = uy3 * inv_velocity_scale;

#if 0
			// report 
			qDebug() << "DelaunayFace_2::calculate_deformation_info: face_index =" << get_face_index();
			qDebug() << "DelaunayFace_2::calculate_deformation_info: v1: x1 = " << x1 << "; y1 =" << y1;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: ux1    = " << ux1 << " in longitude in m/s ";
			qDebug() << "DelaunayFace_2::calculate_deformation_info: uy1    = " << uy1 << " in colatitude in m/s ";
			qDebug() << "DelaunayFace_2::calculate_deformation_info: v2: x2 = " << x2 << "; y2 =" << y2;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: ux2    = " << ux2 << " in longitude in m/s ";
			qDebug() << "DelaunayFace_2::calculate_deformation_info: uy2    = " << uy2 << " in colatitude in m/s ";
			qDebug() << "DelaunayFace_2::calculate_deformation_info: v3: x3 = " << x3 << "; y3 =" << y3;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: ux3    = " << ux3 << " in longitude in m/s ";
			qDebug() << "DelaunayFace_2::calculate_deformation_info: uy3    = " << uy3 << " in colatitude in m/s ";
#endif

			// compute derivatives ; NOTE: duydy = duy/dy ; units = 1/s
			const double duxdx = (b1*ux1 + b2*ux2 + b3*ux3) * inv_det;
			const double duydy = (c1*uy1 + c2*uy2 + c3*uy3) * inv_det;

			const double duxdy = (c1*ux1 + c2*ux2 + c3*ux3) * inv_det;
			const double duydx = (b1*uy1 + b2*uy2 + b3*uy3) * inv_det;

#if 0
			qDebug() << "DelaunayFace_2::calculate_deformation_info: duxdx = " << duxdx;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: duydy = " << duydy;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: duxdy = " << duxdy;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: duydx = " << duydx;
#endif

			// Compute Strain Rates
			const double SR22 = duxdx;
			const double SR33 = duydy;
			const double SR23 = 0.5 * (duxdy + duydx);

#if 0
// keep for now, but not yet needed
			// Compute Rotations
			const double ROT23 = 0.5 * (duydx - duxdy);
			const double ROT32 = 0.5 * (duxdy - duydx);
#endif

			// Principle strain rates.
			const double SR_DIR = 0.5 * std::atan( 2.0 * SR23/(SR33-SR22) );
			const double SR_variation = std::sqrt( SR23*SR23 + 0.25 * ((SR33-SR22)*(SR33-SR22)) );
			const double SR1 = 0.5*(SR22+SR33) + SR_variation;
			const double SR2 = 0.5*(SR22+SR33) - SR_variation;

			// Dilitational strain rate
			const double dilitation = SR22 + SR33;

			// These will be updated by function calls from ResolvedTriangulationNetwork.cc
			const double smooth_dilitation = 0.0;
			const double smooth_second_invariant = 0.0;
			const double sph_dilitation = 0.0;
			const double sph_second_invariant = 0.0;
			const double sph_smooth_dilitation = 0.0;
			const double sph_smooth_second_invariant = 0.0;

			// Second invariant of the strain rate

            // incompressable
			// const double second_invariant = std::sqrt(SR22 * SR22 + SR33 * SR33 + 2.0 * SR23 * SR23); 

            // compressable
			// const double second_invariant = std::sqrt( (SR22 * SR33) - (SR23 * SR23) );
            const double tmp1=( SR22 * SR33) - (SR23 * SR23);
			const double second_invariant = copysign( std::sqrt( std::abs(tmp1) ),tmp1) ;

#if 0
			qDebug() << "DelaunayFace_2::calculate_deformation_info: SR22 = " << SR22;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: SR33 = " << SR33;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: SR23 = " << SR23;
#if 0
// keep for now, but not yet needed
			qDebug() << "DelaunayFace_2::calculate_deformation_info: ROT23 = " << ROT23;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: ROT32 = " << ROT32;
#endif
			qDebug() << "DelaunayFace_2::calculate_deformation_info: SR_DIR = " << SR_DIR;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: SR1 = " << SR1;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: SR2 = " << SR2;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: dilitation = " << dilitation;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: second_invariant = " << second_invariant;
			qDebug() << "DelaunayFace_2::calculate_deformation_info: smooth_dilitation = " << smooth_dilitation;
#endif

			d_deformation_info = DeformationInfo(
					SR22,
					SR33,
					SR23,
					SR_DIR,
					SR1,
					SR2,
					dilitation,
					second_invariant,
					smooth_dilitation,
					smooth_second_invariant,
					sph_dilitation,
					sph_second_invariant,
					sph_smooth_dilitation,
					sph_smooth_second_invariant
			);
		}
	}
}


#endif // GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONDELAUNAY2_H
