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
#include <QProgressBar>
#include <boost/shared_ptr.hpp>

#include "GenerateVelocityDomainCitcoms.h"

#include "maths/PolylineIntersections.h"
#include "maths/Rotation.h"

#include "utils/Profile.h"

namespace
{
	const double MY_PI = 3.14159265358979323846;
	const double OFFSET = 9.736/180.0*MY_PI;
	const int DIAMONDS_MUNBER = 12;

	void 
	even_divide_arc(
			int elx,
			double x1, double y1, double z1,
			double x2, double y2, double z2,
			std::vector<double> &theta,
			std::vector<double> &fi)
	{
		double dx,dy,dz,xx,yy,zz;
		int j, nox;

		nox = elx+1;

		dx = (x2 - x1)/elx;
		dy = (y2 - y1)/elx;
		dz = (z2 - z1)/elx;
		
		for (j=1;j<=nox;j++)   
		{
			xx = x1 + dx*(j-1) + 5.0e-32;
			yy = y1 + dy*(j-1);
			zz = z1 + dz*(j-1);
			theta.push_back(acos(zz/sqrt(xx*xx+yy*yy+zz*zz)));
			fi.push_back(atan2(yy,xx));
		}

		return;
	}

	inline
	void 
	convert_coord(
			const double &theta, 
			const double &fi, 
			double &x, double &y, double &z)
	{
		x = sin(theta)*cos(fi);
		y = sin(theta)*sin(fi);
		z = cos(theta);
	}

	inline
	GPlatesMaths::PointOnSphere 
	create_vertex(
			double theta, 
			double fi)
	{
		double x, y, z;
		convert_coord(theta,fi,x,y,z);
		return *GPlatesMaths::PointOnSphere::create_on_heap(GPlatesMaths::UnitVector3D(x,y,z));
	}

	class Mesh
	{
	public:
		Mesh(
				int node_x=1);
		
		/**
		 *   Given the index of the diamond, return all the points                                                                   
		 */
		void
		get_diamonds_points(
				unsigned index, 
				std::vector<GPlatesMaths::PointOnSphere> &points);
		
	private:
		class CapDiamond
		{
		public:
			CapDiamond(
					const GPlatesMaths::PointOnSphere &v1,
					const GPlatesMaths::PointOnSphere &v2,
					const GPlatesMaths::PointOnSphere &v3,
					const GPlatesMaths::PointOnSphere &v4)
				:d_vertex_1(v1), d_vertex_2(v2), d_vertex_3(v3), d_vertex_4(v4)
			{ }
			
			/**
			 *     given the resolution, return all the points in the mesh diamond                                                                  
			 */
			void 
			get_points(
					int resolution,
					std::vector<GPlatesMaths::PointOnSphere> &points);
			
			/**
			 *      given the dividend, divide the edges of the mesh diamond evenly
			 *      by inserting points on the edges
			 *      the points inserted will be kept in the member variables 
			 */
			void 
			divide_arc_evenly(
					unsigned dividend);

			void 
			divide_arc_evenly(
					unsigned dividend,
					const GPlatesMaths::PointOnSphere& vertex_begin,
					const GPlatesMaths::PointOnSphere& vertex_end,
					std::vector<GPlatesMaths::PointOnSphere>& points_on_edge);

			/**
			 *      find all the intersection points of the mesh diamond
			 *      The points are kept in d_intersections.
			 */
			void 
			find_intersections();

		private:
			GPlatesMaths::PointOnSphere d_vertex_1;
			GPlatesMaths::PointOnSphere d_vertex_2;
			GPlatesMaths::PointOnSphere d_vertex_3;
			GPlatesMaths::PointOnSphere d_vertex_4;
			std::vector<GPlatesMaths::PointOnSphere> d_points_on_edge_1_2;
			std::vector<GPlatesMaths::PointOnSphere> d_points_on_edge_2_3;
			std::vector<GPlatesMaths::PointOnSphere> d_points_on_edge_3_4;
			std::vector<GPlatesMaths::PointOnSphere> d_points_on_edge_4_1;
			std::vector<GPlatesMaths::PointOnSphere> d_intersections;
		};

		int d_node_x;
		std::vector<CapDiamond> d_cap_diamonds; 
	};

	/*
	* The order of points sequence is important for the compatibility with Citcoms.
	*    1-------4
	*    | | | | |      
	*    | | | | |
	*	 2-------3
	* The order is: start with edge (1-2), save points on each vertical line until edge(4-3)
	*/   
	void 
	Mesh::CapDiamond::get_points(
			int resolution,
			std::vector<GPlatesMaths::PointOnSphere> &points)
	{
		//PROFILE_FUNC();
		divide_arc_evenly( resolution );
		find_intersections();

		points.push_back(d_vertex_1);
		points.insert(
				points.end(), 
				d_points_on_edge_1_2.begin(), 
				d_points_on_edge_1_2.end());
		points.push_back(d_vertex_2);
		
		for(int i = 0; i < resolution -1 ; i++)
		{
			points.push_back(
					d_points_on_edge_4_1[ resolution - i - 2 ] );
		
			for(int j = 0; j < resolution - 1 ; j++)
			{
				int p = i + j * ( resolution - 1 );
				points.push_back( d_intersections[ p ] );
			}
			points.push_back(
					d_points_on_edge_2_3[ i ] );
		}
		
		points.push_back( d_vertex_4 );
		points.insert(
				points.end(),
				d_points_on_edge_3_4.rbegin(),
				d_points_on_edge_3_4.rend() );
		points.push_back( d_vertex_3 );
	}

	void 
	Mesh::CapDiamond::divide_arc_evenly(
			unsigned dividend,
			const GPlatesMaths::PointOnSphere& vertex_begin,
			const GPlatesMaths::PointOnSphere& vertex_end,
			std::vector<GPlatesMaths::PointOnSphere>& points_on_edge)
	{
		std::vector<double> theta, fi; 

		even_divide_arc(
			dividend,
			vertex_begin.position_vector().x().dval(),
			vertex_begin.position_vector().y().dval(),
			vertex_begin.position_vector().z().dval(),
			vertex_end.position_vector().x().dval(),
			vertex_end.position_vector().y().dval(),
			vertex_end.position_vector().z().dval(),
			theta,
			fi);

		for(size_t i=1;i<dividend;i++)
		{
			points_on_edge.push_back(create_vertex(theta[i],fi[i]));
		}
	}

	//the order of corners is: 
	//           1 - 4
	//           |      | 
	//           2 - 3 
	void 
	Mesh::CapDiamond::divide_arc_evenly(
			unsigned dividend)
	{
		//PROFILE_FUNC();
		divide_arc_evenly(dividend, d_vertex_1, d_vertex_2, d_points_on_edge_1_2);
		divide_arc_evenly(dividend, d_vertex_2, d_vertex_3, d_points_on_edge_2_3);
		divide_arc_evenly(dividend, d_vertex_3, d_vertex_4, d_points_on_edge_3_4);
		divide_arc_evenly(dividend, d_vertex_4, d_vertex_1, d_points_on_edge_4_1);
	}

	void 
	Mesh::CapDiamond::find_intersections()
	{
		d_intersections.reserve(d_points_on_edge_1_2.size()*d_points_on_edge_2_3.size());	
		for(size_t i =0; i<d_points_on_edge_1_2.size(); i++)
		{
			for(size_t j =0; j<d_points_on_edge_2_3.size(); j++)
			{
					std::vector<GPlatesMaths::PointOnSphere> points_pair_1, points_pair_2;
					
					points_pair_1.push_back(d_points_on_edge_1_2[i]);
					points_pair_1.push_back(d_points_on_edge_3_4[d_points_on_edge_1_2.size()-i-1]);
					points_pair_2.push_back(d_points_on_edge_2_3[j]);
					points_pair_2.push_back(d_points_on_edge_4_1[d_points_on_edge_2_3.size()-j-1]);

					boost::shared_ptr<const GPlatesMaths::PolylineIntersections::Graph> intersection=
							GPlatesMaths::PolylineIntersections::partition_intersecting_geometries(
									*GPlatesMaths::PolylineOnSphere::create_on_heap(points_pair_1),
									*GPlatesMaths::PolylineOnSphere::create_on_heap(points_pair_2));
				
					if(intersection)
					{
						d_intersections.push_back(
								intersection->intersections[0]->intersection_point);
					}
			}
		}
	}

	Mesh::Mesh(
			int node_x)
		: d_node_x(node_x)
	{
		double vertice[DIAMONDS_MUNBER][4][2] =
		{
			{
				{0.0,0.0}, 
				{MY_PI/4.0+OFFSET, 0.0},
				{MY_PI/2.0, MY_PI/4.0}, 
				{MY_PI/4.0+OFFSET, MY_PI/2.0}
			},//#1
			{
				{MY_PI/4.0+OFFSET, MY_PI/2.0},
				{MY_PI/2.0, MY_PI/2.0 - MY_PI/4.0},
				{3*MY_PI/4.0-OFFSET, MY_PI/2.0},
				{MY_PI/2.0, MY_PI/2.0 + MY_PI/4.0}
			},//#2
			{
				{MY_PI/2.0, MY_PI/2.0 + MY_PI/4.0},
				{3*MY_PI/4.0-OFFSET, MY_PI/2.0},
				{MY_PI, 0.0},
				{3*MY_PI/4.0-OFFSET, 2*MY_PI/2.0}
			},//#3
			{
				{0.0, 0.0},
				{MY_PI/4.0+OFFSET, MY_PI/2.0},
				{MY_PI/2.0, MY_PI/2.0 + MY_PI/4.0},
				{MY_PI/4.0+OFFSET, 2*MY_PI/2.0}
			},//#4
			{
				{MY_PI/4.0+OFFSET, 2*MY_PI/2.0},
				{MY_PI/2.0, 2*MY_PI/2.0 - MY_PI/4.0},
				{3*MY_PI/4.0-OFFSET, 2*MY_PI/2.0},
				{MY_PI/2.0, 2*MY_PI/2.0 + MY_PI/4.0}
			},//#5
			{
				{MY_PI/2.0, 2*MY_PI/2.0 + MY_PI/4.0},
				{3*MY_PI/4.0-OFFSET, 2*MY_PI/2.0},
				{MY_PI, 0.0},
				{3*MY_PI/4.0-OFFSET, 3*MY_PI/2.0}
			},//#6
			{
				{0.0, 0.0},
				{MY_PI/4.0+OFFSET, 2*MY_PI/2.0},
				{MY_PI/2.0, 2*MY_PI/2.0 + MY_PI/4.0},
				{MY_PI/4.0+OFFSET,  3*MY_PI/2.0}
			},//#7
			{
				{MY_PI/4.0+OFFSET, 3*MY_PI/2.0},
				{MY_PI/2.0,  3*MY_PI/2.0 - MY_PI/4.0},
				{3*MY_PI/4.0-OFFSET, 3*MY_PI/2.0},
				{MY_PI/2.0, 3*MY_PI/2.0 + MY_PI/4.0}
			},//#8
			{
				{MY_PI/2.0, 3*MY_PI/2.0 + MY_PI/4.0},
				{3*MY_PI/4.0-OFFSET, 3*MY_PI/2.0},
				{MY_PI, 0.0},
				{3*MY_PI/4.0-OFFSET, 4*MY_PI/2.0}
			},//#9
			{
				{0.0,0.0},
				{MY_PI/4.0+OFFSET, 3*MY_PI/2.0},
				{MY_PI/2.0, 3*MY_PI/2.0 + MY_PI/4.0},
				{MY_PI/4.0+OFFSET,  4*MY_PI/2.0}
			},//#10
			{
				{MY_PI/4.0+OFFSET, 4*MY_PI/2.0},
				{MY_PI/2.0,  4*MY_PI/2.0 - MY_PI/4.0},
				{3*MY_PI/4.0-OFFSET, 4*MY_PI/2.0},
				{MY_PI/2.0, 4*MY_PI/2.0 + MY_PI/4.0}
			},//#11
			{
				{MY_PI/2.0, MY_PI/4.00},
				{3*MY_PI/4.0-OFFSET, 0.0},
				{MY_PI, 0.0},
				{3*MY_PI/4.0-OFFSET, MY_PI/2.0}
			},//#12
		};

		for(int i=0; i<DIAMONDS_MUNBER; i++)
		{
			d_cap_diamonds.push_back(
					CapDiamond(
							create_vertex(vertice[i][0][0],vertice[i][0][1]),
							create_vertex(vertice[i][1][0],vertice[i][1][1]),
							create_vertex(vertice[i][2][0],vertice[i][2][1]),
							create_vertex(vertice[i][3][0],vertice[i][3][1])
							));
		}
	}

	void
	Mesh::get_diamonds_points(
			unsigned index, 
			std::vector<GPlatesMaths::PointOnSphere> &points)
	{
		d_cap_diamonds[index].get_points(d_node_x, points);
	}
}//anonymous namespace


GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::GenerateVelocityDomainCitcoms::generate_velocity_domain(
		int node_x,
		unsigned index)
{
	Mesh mesh(node_x);
	std::vector<GPlatesMaths::PointOnSphere> points;
	mesh.get_diamonds_points(index, points);
	//Mesh mesh2(mesh);
	
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type geo_ptr = 
		GPlatesMaths::MultiPointOnSphere::create_on_heap(
				points.begin(), 
				points.end());

	const GPlatesMaths::Rotation rot = 
		GPlatesMaths::Rotation::create(GPlatesMaths::UnitVector3D(0,1,0),(0.5*(MY_PI/4.0)/node_x));

	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type new_geo_ptr = 
		rot*geo_ptr;

	const GPlatesMaths::MultiPointOnSphere* mul_point = 
		dynamic_cast<const GPlatesMaths::MultiPointOnSphere*> (new_geo_ptr.get());
	return mul_point->get_non_null_pointer();
}

void
GPlatesAppLogic::GenerateVelocityDomainCitcoms::generate_velocity_domains(
		int node_x, 
		std::vector<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> &geometries)
{
	for(int i= 0; i<DIAMONDS_MUNBER; i++)
	{
		geometries.push_back(
					generate_velocity_domain(node_x,i));
	}
}

