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

#include "GeometryDistance.h"

#include "ConstGeometryOnSphereVisitor.h"
#include "PolyGreatCircleArcBoundingTree.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Find the minimum distance between two derived @a GeometryOnSphere objects.
		 */
		class MinimumDistanceBetweenGeometryOnSpheres :
				public ConstGeometryOnSphereVisitor
		{
		public:

			explicit
			MinimumDistanceBetweenGeometryOnSpheres(
					const GeometryOnSphere &second_geometry,
					bool geometry1_interior_is_solid,
					bool geometry2_interior_is_solid,
					AngularDistance &minimum_distance,
					const boost::optional<const AngularExtent &> &minimum_distance_threshold,
					const boost::optional<
							boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
									> &closest_positions,
					const boost::optional<
							boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
									> &closest_indices) :
				d_second_geometry(second_geometry),
				d_geometry1_interior_is_solid(geometry1_interior_is_solid),
				d_geometry2_interior_is_solid(geometry2_interior_is_solid),
				d_minimum_distance(minimum_distance),
				d_minimum_distance_threshold(minimum_distance_threshold),
				d_closest_positions(closest_positions),
				d_closest_indices(closest_indices)
			{  }

			virtual
			void
			visit_point_on_sphere(
					PointOnSphere::non_null_ptr_to_const_type point_on_sphere1)
			{
				/**
				 * Minimum distance between a PointOnSphere and a GeometryOnSphere.
				 */
				class PointOnSphereVisitor :
						public ConstGeometryOnSphereVisitor
				{
				public:

					explicit
					PointOnSphereVisitor(
							const PointOnSphere &point_on_sphere1_,
							bool geometry2_interior_is_solid,
							AngularDistance &minimum_distance,
							const boost::optional<const AngularExtent &> &minimum_distance_threshold,
							const boost::optional<
									boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
											> &closest_positions,
							const boost::optional<
									boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
											> &closest_indices) :
						d_point_on_sphere1(point_on_sphere1_),
						d_geometry2_interior_is_solid(geometry2_interior_is_solid),
						d_minimum_distance(minimum_distance),
						d_minimum_distance_threshold(minimum_distance_threshold),
						d_closest_positions(closest_positions),
						d_closest_indices(closest_indices)
					{  }

					virtual
					void
					visit_point_on_sphere(
							PointOnSphere::non_null_ptr_to_const_type point_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_point_on_sphere1,
								*point_on_sphere2,
								d_minimum_distance_threshold);

						if (d_closest_positions)
						{
							boost::get<0>(d_closest_positions.get()) = d_point_on_sphere1.position_vector();
							boost::get<1>(d_closest_positions.get()) = point_on_sphere2->position_vector();
						}

						if (d_closest_indices)
						{
							boost::get<0>(d_closest_indices.get()) = 0;
							boost::get<1>(d_closest_indices.get()) = 0;
						}
					}

					virtual
					void
					visit_multi_point_on_sphere(
							MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere2)
					{
						boost::optional<UnitVector3D &> closest_position_in_multipoint2;
						if (d_closest_positions)
						{
							closest_position_in_multipoint2 = boost::get<1>(d_closest_positions.get());
						}
						boost::optional<unsigned int &> closest_position_index_in_multipoint2;
						if (d_closest_indices)
						{
							closest_position_index_in_multipoint2 = boost::get<1>(d_closest_indices.get());
						}

						d_minimum_distance = minimum_distance(
								d_point_on_sphere1,
								*multi_point_on_sphere2,
								d_minimum_distance_threshold,
								closest_position_in_multipoint2,
								closest_position_index_in_multipoint2);

						if (d_closest_positions)
						{
							boost::get<0>(d_closest_positions.get()) = d_point_on_sphere1.position_vector();
						}
						if (d_closest_indices)
						{
							boost::get<0>(d_closest_indices.get()) = 0;
						}
					}

					virtual
					void
					visit_polygon_on_sphere(
							PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere2)
					{
						boost::optional<UnitVector3D &> closest_position_on_polygon2;
						if (d_closest_positions)
						{
							closest_position_on_polygon2 = boost::get<1>(d_closest_positions.get());
						}
						boost::optional<unsigned int &> closest_segment_index_in_polygon2;
						if (d_closest_indices)
						{
							closest_segment_index_in_polygon2 = boost::get<1>(d_closest_indices.get());
						}

						d_minimum_distance = minimum_distance(
								d_point_on_sphere1,
								*polygon_on_sphere2,
								d_geometry2_interior_is_solid,
								d_minimum_distance_threshold,
								closest_position_on_polygon2,
								closest_segment_index_in_polygon2);

						if (d_closest_positions)
						{
							boost::get<0>(d_closest_positions.get()) = d_point_on_sphere1.position_vector();
						}
						if (d_closest_indices)
						{
							boost::get<0>(d_closest_indices.get()) = 0;
						}
					}

					virtual
					void
					visit_polyline_on_sphere(
							PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere2)
					{
						boost::optional<UnitVector3D &> closest_position_on_polyline2;
						if (d_closest_positions)
						{
							closest_position_on_polyline2 = boost::get<1>(d_closest_positions.get());
						}
						boost::optional<unsigned int &> closest_segment_index_in_polyline2;
						if (d_closest_indices)
						{
							closest_segment_index_in_polyline2 = boost::get<1>(d_closest_indices.get());
						}

						d_minimum_distance = minimum_distance(
								d_point_on_sphere1,
								*polyline_on_sphere2,
								d_minimum_distance_threshold,
								closest_position_on_polyline2,
								closest_segment_index_in_polyline2);

						if (d_closest_positions)
						{
							boost::get<0>(d_closest_positions.get()) = d_point_on_sphere1.position_vector();
						}
						if (d_closest_indices)
						{
							boost::get<0>(d_closest_indices.get()) = 0;
						}
					}

				private:

					const PointOnSphere &d_point_on_sphere1;
					bool d_geometry2_interior_is_solid;
					AngularDistance &d_minimum_distance;
					const boost::optional<const AngularExtent &> &d_minimum_distance_threshold;
					const boost::optional<
							boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
									> &d_closest_positions;
					const boost::optional<
							boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
									> &d_closest_indices;
				};

				PointOnSphereVisitor visitor(
						*point_on_sphere1,
						d_geometry2_interior_is_solid,
						d_minimum_distance,
						d_minimum_distance_threshold,
						d_closest_positions,
						d_closest_indices);

				d_second_geometry.accept_visitor(visitor);
			}

			virtual
			void
			visit_multi_point_on_sphere(
					MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere1)
			{
				/**
				 * Minimum distance between a MultiPointOnSphere and a GeometryOnSphere.
				 */
				class MultiPointOnSphereVisitor :
						public ConstGeometryOnSphereVisitor
				{
				public:

					explicit
					MultiPointOnSphereVisitor(
							const MultiPointOnSphere &multi_point_on_sphere1_,
							bool geometry2_interior_is_solid,
							AngularDistance &minimum_distance,
							const boost::optional<const AngularExtent &> &minimum_distance_threshold,
							const boost::optional<
									boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
											> &closest_positions,
							const boost::optional<
									boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
											> &closest_indices) :
						d_multi_point_on_sphere1(multi_point_on_sphere1_),
						d_geometry2_interior_is_solid(geometry2_interior_is_solid),
						d_minimum_distance(minimum_distance),
						d_minimum_distance_threshold(minimum_distance_threshold),
						d_closest_positions(closest_positions),
						d_closest_indices(closest_indices)
					{  }

					virtual
					void
					visit_point_on_sphere(
							PointOnSphere::non_null_ptr_to_const_type point_on_sphere2)
					{
						boost::optional<UnitVector3D &> closest_position_in_multipoint1;
						if (d_closest_positions)
						{
							closest_position_in_multipoint1 = boost::get<0>(d_closest_positions.get());
						}
						boost::optional<unsigned int &> closest_position_index_in_multipoint1;
						if (d_closest_indices)
						{
							closest_position_index_in_multipoint1 = boost::get<0>(d_closest_indices.get());
						}

						d_minimum_distance = minimum_distance(
								d_multi_point_on_sphere1,
								*point_on_sphere2,
								d_minimum_distance_threshold,
								closest_position_in_multipoint1,
								closest_position_index_in_multipoint1);

						if (d_closest_positions)
						{
							boost::get<1>(d_closest_positions.get()) = point_on_sphere2->position_vector();
						}
						if (d_closest_indices)
						{
							boost::get<1>(d_closest_indices.get()) = 0;
						}
					}

					virtual
					void
					visit_multi_point_on_sphere(
							MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_multi_point_on_sphere1,
								*multi_point_on_sphere2,
								d_minimum_distance_threshold,
								d_closest_positions,
								d_closest_indices);
					}

					virtual
					void
					visit_polygon_on_sphere(
							PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_multi_point_on_sphere1,
								*polygon_on_sphere2,
								d_geometry2_interior_is_solid,
								d_minimum_distance_threshold,
								d_closest_positions,
								d_closest_indices);
					}

					virtual
					void
					visit_polyline_on_sphere(
							PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_multi_point_on_sphere1,
								*polyline_on_sphere2,
								d_minimum_distance_threshold,
								d_closest_positions,
								d_closest_indices);
					}

				private:

					const MultiPointOnSphere &d_multi_point_on_sphere1;
					bool d_geometry2_interior_is_solid;
					AngularDistance &d_minimum_distance;
					const boost::optional<const AngularExtent &> &d_minimum_distance_threshold;
					const boost::optional<
							boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
									> &d_closest_positions;
					const boost::optional<
							boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
									> &d_closest_indices;
				};

				MultiPointOnSphereVisitor visitor(
						*multi_point_on_sphere1,
						d_geometry2_interior_is_solid,
						d_minimum_distance,
						d_minimum_distance_threshold,
						d_closest_positions,
						d_closest_indices);

				d_second_geometry.accept_visitor(visitor);
			}

			virtual
			void
			visit_polygon_on_sphere(
					PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere1)
			{
				/**
				 * Minimum distance between a PolygonOnSphere and a GeometryOnSphere.
				 */
				class PolygonOnSphereVisitor :
						public ConstGeometryOnSphereVisitor
				{
				public:

					explicit
					PolygonOnSphereVisitor(
							const PolygonOnSphere &polygon_on_sphere1_,
							bool geometry1_interior_is_solid,
							bool geometry2_interior_is_solid,
							AngularDistance &minimum_distance,
							const boost::optional<const AngularExtent &> &minimum_distance_threshold,
							const boost::optional<
									boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
											> &closest_positions,
							const boost::optional<
									boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
											> &closest_indices) :
						d_polygon_on_sphere1(polygon_on_sphere1_),
						d_geometry1_interior_is_solid(geometry1_interior_is_solid),
						d_geometry2_interior_is_solid(geometry2_interior_is_solid),
						d_minimum_distance(minimum_distance),
						d_minimum_distance_threshold(minimum_distance_threshold),
						d_closest_positions(closest_positions),
						d_closest_indices(closest_indices)
					{  }

					virtual
					void
					visit_point_on_sphere(
							PointOnSphere::non_null_ptr_to_const_type point_on_sphere2)
					{
						boost::optional<UnitVector3D &> closest_position_on_polygon1;
						if (d_closest_positions)
						{
							closest_position_on_polygon1 = boost::get<0>(d_closest_positions.get());
						}
						boost::optional<unsigned int &> closest_segment_index_in_polygon1;
						if (d_closest_indices)
						{
							closest_segment_index_in_polygon1 = boost::get<0>(d_closest_indices.get());
						}

						d_minimum_distance = minimum_distance(
								d_polygon_on_sphere1,
								*point_on_sphere2,
								d_geometry1_interior_is_solid,
								d_minimum_distance_threshold,
								closest_position_on_polygon1,
								closest_segment_index_in_polygon1);

						if (d_closest_positions)
						{
							boost::get<1>(d_closest_positions.get()) = point_on_sphere2->position_vector();
						}
						if (d_closest_indices)
						{
							boost::get<1>(d_closest_indices.get()) = 0;
						}
					}

					virtual
					void
					visit_multi_point_on_sphere(
							MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_polygon_on_sphere1,
								*multi_point_on_sphere2,
								d_geometry1_interior_is_solid,
								d_minimum_distance_threshold,
								d_closest_positions,
								d_closest_indices);
					}

					virtual
					void
					visit_polygon_on_sphere(
							PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_polygon_on_sphere1,
								*polygon_on_sphere2,
								d_geometry1_interior_is_solid,
								d_geometry2_interior_is_solid,
								d_minimum_distance_threshold,
								d_closest_positions,
								d_closest_indices);
					}

					virtual
					void
					visit_polyline_on_sphere(
							PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_polygon_on_sphere1,
								*polyline_on_sphere2,
								d_geometry1_interior_is_solid,
								d_minimum_distance_threshold,
								d_closest_positions,
								d_closest_indices);
					}

				private:

					const PolygonOnSphere &d_polygon_on_sphere1;
					bool d_geometry1_interior_is_solid;
					bool d_geometry2_interior_is_solid;
					AngularDistance &d_minimum_distance;
					const boost::optional<const AngularExtent &> &d_minimum_distance_threshold;
					const boost::optional<
							boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
									> &d_closest_positions;
					const boost::optional<
							boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
									> &d_closest_indices;
				};

				PolygonOnSphereVisitor visitor(
						*polygon_on_sphere1,
						d_geometry1_interior_is_solid,
						d_geometry2_interior_is_solid,
						d_minimum_distance,
						d_minimum_distance_threshold,
						d_closest_positions,
						d_closest_indices);

				d_second_geometry.accept_visitor(visitor);
			}

			virtual
			void
			visit_polyline_on_sphere(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere1)
			{
				/**
				 * Minimum distance between a PolylineOnSphere and a GeometryOnSphere.
				 */
				class PolylineOnSphereVisitor :
						public ConstGeometryOnSphereVisitor
				{
				public:

					explicit
					PolylineOnSphereVisitor(
							const PolylineOnSphere &polyline_on_sphere1_,
							bool geometry2_interior_is_solid,
							AngularDistance &minimum_distance,
							const boost::optional<const AngularExtent &> &minimum_distance_threshold,
							const boost::optional<
									boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
											> &closest_positions,
							const boost::optional<
									boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
											> &closest_indices) :
						d_polyline_on_sphere1(polyline_on_sphere1_),
						d_geometry2_interior_is_solid(geometry2_interior_is_solid),
						d_minimum_distance(minimum_distance),
						d_minimum_distance_threshold(minimum_distance_threshold),
						d_closest_positions(closest_positions),
						d_closest_indices(closest_indices)
					{  }

					virtual
					void
					visit_point_on_sphere(
							PointOnSphere::non_null_ptr_to_const_type point_on_sphere2)
					{
						boost::optional<UnitVector3D &> closest_position_on_polyline1;
						if (d_closest_positions)
						{
							closest_position_on_polyline1 = boost::get<0>(d_closest_positions.get());
						}
						boost::optional<unsigned int &> closest_segment_index_in_polyline1;
						if (d_closest_indices)
						{
							closest_segment_index_in_polyline1 = boost::get<0>(d_closest_indices.get());
						}

						d_minimum_distance = minimum_distance(
								d_polyline_on_sphere1,
								*point_on_sphere2,
								d_minimum_distance_threshold,
								closest_position_on_polyline1,
								closest_segment_index_in_polyline1);

						if (d_closest_positions)
						{
							boost::get<1>(d_closest_positions.get()) = point_on_sphere2->position_vector();
						}
						if (d_closest_indices)
						{
							boost::get<1>(d_closest_indices.get()) = 0;
						}
					}

					virtual
					void
					visit_multi_point_on_sphere(
							MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_polyline_on_sphere1,
								*multi_point_on_sphere2,
								d_minimum_distance_threshold,
								d_closest_positions,
								d_closest_indices);
					}

					virtual
					void
					visit_polygon_on_sphere(
							PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_polyline_on_sphere1,
								*polygon_on_sphere2,
								d_geometry2_interior_is_solid,
								d_minimum_distance_threshold,
								d_closest_positions,
								d_closest_indices);
					}

					virtual
					void
					visit_polyline_on_sphere(
							PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere2)
					{
						d_minimum_distance = minimum_distance(
								d_polyline_on_sphere1,
								*polyline_on_sphere2,
								d_minimum_distance_threshold,
								d_closest_positions,
								d_closest_indices);
					}

				private:

					const PolylineOnSphere &d_polyline_on_sphere1;
					bool d_geometry2_interior_is_solid;
					AngularDistance &d_minimum_distance;
					const boost::optional<const AngularExtent &> &d_minimum_distance_threshold;
					const boost::optional<
							boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
									> &d_closest_positions;
					const boost::optional<
							boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
									> &d_closest_indices;
				};

				PolylineOnSphereVisitor visitor(
						*polyline_on_sphere1,
						d_geometry2_interior_is_solid,
						d_minimum_distance,
						d_minimum_distance_threshold,
						d_closest_positions,
						d_closest_indices);

				d_second_geometry.accept_visitor(visitor);
			}

		private:

			const GeometryOnSphere &d_second_geometry;
			bool d_geometry1_interior_is_solid;
			bool d_geometry2_interior_is_solid;
			AngularDistance &d_minimum_distance;
			const boost::optional<const AngularExtent &> &d_minimum_distance_threshold;
			const boost::optional<
					boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
							> &d_closest_positions;
			const boost::optional<
					boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
							> &d_closest_indices;
		};


		/**
		 * Calculate (and update) the minimum distance between a point and a polyline or polygon.
		 */
		template <typename GreatCircleArcConstIteratorType>
		void
		minimum_distance_between_point_and_poly_geometry_bounding_tree_node(
				const PointOnSphere &point,
				const PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType> &polygeom_bounding_tree,
				const typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::node_type &polygeom_sub_tree_node,
				AngularDistance &min_distance,
				AngularExtent &min_distance_threshold,
				const boost::optional<UnitVector3D &> &closest_position_on_polygeom,
				boost::optional<unsigned int &> closest_segment_index_in_polygeom)
		{
			typedef PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType> bounding_tree_type;

			if (polygeom_sub_tree_node.is_leaf_node())
			{
				// Iterate over the great circle arcs of the leaf node.
				typename bounding_tree_type::great_circle_arc_const_iterator_type
						gca_iter = polygeom_sub_tree_node.get_bounded_great_circle_arcs_begin();
				typename bounding_tree_type::great_circle_arc_const_iterator_type
						gca_end = polygeom_sub_tree_node.get_bounded_great_circle_arcs_end();
				unsigned int gca_index = polygeom_sub_tree_node.get_bounded_great_circle_arcs_begin_index();
				for ( ; gca_iter != gca_end; ++gca_iter, ++gca_index)
				{
					const GreatCircleArc &gca = *gca_iter;

					// Calculate minimum distance from the point to the current great circle arc.
					const AngularDistance min_distance_point_to_gca =
							minimum_distance(
									point,
									gca,
									min_distance_threshold,
									closest_position_on_polygeom);

					// If shortest distance so far (within threshold)...
					if (min_distance_point_to_gca.is_precisely_less_than(min_distance))
					{
						min_distance = min_distance_point_to_gca;
						min_distance_threshold = AngularExtent(min_distance);

						// If index of closest segment in polygeom is requested...
						if (closest_segment_index_in_polygeom)
						{
							closest_segment_index_in_polygeom.get() = gca_index;
						}
					}
				}

				return;
			}
			// else is an internal node...

			const typename bounding_tree_type::node_type child_nodes[2] =
			{
				polygeom_bounding_tree.get_child_node(polygeom_sub_tree_node, 0),
				polygeom_bounding_tree.get_child_node(polygeom_sub_tree_node, 1)
			};

			const AngularDistance child_node_min_bounding_small_circle_distances[2] =
			{
				minimum_distance(point, child_nodes[0].get_bounding_small_circle()),
				minimum_distance(point, child_nodes[1].get_bounding_small_circle())
			};

			// Visit the closest child node first since it can avoid unnecessary calculations
			// when visiting the furthest child node (because more likely to exceed the threshold).
			unsigned int child_node_visit_indices[2];
			if (child_node_min_bounding_small_circle_distances[0].is_precisely_less_than(
				child_node_min_bounding_small_circle_distances[1]))
			{
				child_node_visit_indices[0] = 0;
				child_node_visit_indices[1] = 1;
			}
			else
			{
				child_node_visit_indices[0] = 1;
				child_node_visit_indices[1] = 0;
			}

			// Iterate over the child nodes.
			for (unsigned int n = 0; n < 2; ++n)
			{
				const unsigned int child_offset = child_node_visit_indices[n];

				// If the point is further away (from the child node's bounding small circle)
				// than the current threshold then skip the current child node.
				if (child_node_min_bounding_small_circle_distances[child_offset]
					.is_precisely_greater_than(min_distance_threshold))
				{
					continue;
				}

				minimum_distance_between_point_and_poly_geometry_bounding_tree_node(
						point,
						polygeom_bounding_tree,
						child_nodes[child_offset],
						min_distance,
						min_distance_threshold,
						closest_position_on_polygeom,
						closest_segment_index_in_polygeom);
			}
		}


		// Forward declaration.
		template <typename GreatCircleArcConstIterator1Type, typename GreatCircleArcConstIterator2Type>
		void
		minimum_distance_between_bounding_tree_node_of_geometry1_and_two_child_nodes_of_geometry2(
				const PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type> &geometry1_bounding_tree,
				const typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type>::node_type &geometry1_sub_tree_node,
				const PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type> &geometry2_bounding_tree,
				const typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type>::node_type &geometry2_sub_tree_node,
				AngularDistance &min_distance,
				AngularExtent &min_distance_threshold,
				const boost::optional< boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/> > &closest_positions,
				const boost::optional< boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/> > &closest_segment_indices);


		/**
		 * Calculate (and update) the minimum distance between a bounding tree node of one polyline or polygon,
		 * and the bounding tree node of another polyline or polygon.
		 */
		template <typename GreatCircleArcConstIterator1Type, typename GreatCircleArcConstIterator2Type>
		void
		minimum_distance_between_bounding_tree_nodes_of_two_geometries(
				const PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type> &geometry1_bounding_tree,
				const typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type>::node_type &geometry1_sub_tree_node,
				const PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type> &geometry2_bounding_tree,
				const typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type>::node_type &geometry2_sub_tree_node,
				AngularDistance &min_distance,
				AngularExtent &min_distance_threshold,
				const boost::optional< boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/> > &closest_positions,
				const boost::optional< boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/> > &closest_segment_indices)
		{
			typedef PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type> geometry1_bounding_tree_type;
			typedef PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type> geometry2_bounding_tree_type;

			// If both geometries are at one of their leaf nodes then calculate N*M distances
			// between the N great circle arcs in first geometry's leaf node and the M great
			// circle arcs in the second geometry's leaf node.
			if (geometry1_sub_tree_node.is_leaf_node() &&
				geometry2_sub_tree_node.is_leaf_node())
			{
				// Iterate over the great circle arcs of the leaf node of the first geometry.
				typename geometry1_bounding_tree_type::great_circle_arc_const_iterator_type
						gca1_iter = geometry1_sub_tree_node.get_bounded_great_circle_arcs_begin();
				typename geometry1_bounding_tree_type::great_circle_arc_const_iterator_type
						gca1_end = geometry1_sub_tree_node.get_bounded_great_circle_arcs_end();
				unsigned int gca1_index = geometry1_sub_tree_node.get_bounded_great_circle_arcs_begin_index();
				for ( ; gca1_iter != gca1_end; ++gca1_iter, ++gca1_index)
				{
					const GreatCircleArc &gca1 = *gca1_iter;

					// Iterate over the great circle arcs of the leaf node of the second geometry.
					typename geometry2_bounding_tree_type::great_circle_arc_const_iterator_type
							gca2_iter = geometry2_sub_tree_node.get_bounded_great_circle_arcs_begin();
					typename geometry2_bounding_tree_type::great_circle_arc_const_iterator_type
							gca2_end = geometry2_sub_tree_node.get_bounded_great_circle_arcs_end();
					unsigned int gca2_index = geometry2_sub_tree_node.get_bounded_great_circle_arcs_begin_index();
					for ( ; gca2_iter != gca2_end; ++gca2_iter, ++gca2_index)
					{
						const GreatCircleArc &gca2 = *gca2_iter;

						// Calculate minimum distance between the current great circle arcs of the
						// two geometries.
						const AngularDistance min_distance_between_gcas =
								minimum_distance(
										gca1,
										gca2,
										min_distance_threshold,
										closest_positions);

						// If shortest distance so far (within threshold)...
						if (min_distance_between_gcas.is_precisely_less_than(min_distance))
						{
							min_distance = min_distance_between_gcas;
							min_distance_threshold = AngularExtent(min_distance);

							// If indices of closest segments in polygeoms is requested...
							if (closest_segment_indices)
							{
								boost::get<0>(closest_segment_indices.get()) = gca1_index;
								boost::get<1>(closest_segment_indices.get()) = gca2_index;
							}
						}
					}
				}

				return;
			}

			if (geometry1_sub_tree_node.is_internal_node() &&
				geometry2_sub_tree_node.is_internal_node())
			{
				// Recurse into the largest internal node first. This can result in fewer
				// minimum distance tests between bounding small circles of sub-tree nodes.
				if (geometry1_sub_tree_node.get_bounding_small_circle().get_angular_extent().is_precisely_greater_than(
					geometry2_sub_tree_node.get_bounding_small_circle().get_angular_extent()))
				{
					// Since we're swapping the order of the geometries we also need to swap the closest position references.
					boost::optional< boost::tuple<UnitVector3D &/*geometry2*/, UnitVector3D &/*geometry1*/> >
							closest_positions_reversed;
					if (closest_positions)
					{
						closest_positions_reversed = boost::make_tuple(
								boost::ref(boost::get<1>(closest_positions.get())),
								boost::ref(boost::get<0>(closest_positions.get())));
					}

					// Since we're swapping the order of the geometries we also need to swap the closest segment references.
					boost::optional< boost::tuple<unsigned int &/*geometry2*/, unsigned int &/*geometry1*/> >
							closest_segment_indices_reversed;
					if (closest_segment_indices)
					{
						closest_segment_indices_reversed = boost::make_tuple(
								boost::ref(boost::get<1>(closest_segment_indices.get())),
								boost::ref(boost::get<0>(closest_segment_indices.get())));
					}

					// Recurse into the child nodes of the first geometry.
					minimum_distance_between_bounding_tree_node_of_geometry1_and_two_child_nodes_of_geometry2(
							geometry2_bounding_tree,
							geometry2_sub_tree_node,
							geometry1_bounding_tree,
							geometry1_sub_tree_node,
							min_distance,
							min_distance_threshold,
							closest_positions_reversed,
							closest_segment_indices_reversed);
				}
				else // second geometry's internal node is larger...
				{
					// Recurse into the child nodes of the second geometry.
					minimum_distance_between_bounding_tree_node_of_geometry1_and_two_child_nodes_of_geometry2(
							geometry1_bounding_tree,
							geometry1_sub_tree_node,
							geometry2_bounding_tree,
							geometry2_sub_tree_node,
							min_distance,
							min_distance_threshold,
							closest_positions,
							closest_segment_indices);
				}

				return;
			}
			// else one geometry is at a leaf node and the other is at an internal node...

			if (geometry1_sub_tree_node.is_internal_node())
			{
				// The second geometry is at a leaf node.

				// Since we're swapping the order of the geometries we also need to swap the closest position references.
				boost::optional< boost::tuple<UnitVector3D &/*geometry2*/, UnitVector3D &/*geometry1*/> >
						closest_positions_reversed;
				if (closest_positions)
				{
					closest_positions_reversed = boost::make_tuple(
							boost::ref(boost::get<1>(closest_positions.get())),
							boost::ref(boost::get<0>(closest_positions.get())));
				}

				// Since we're swapping the order of the geometries we also need to swap the closest segment references.
				boost::optional< boost::tuple<unsigned int &/*geometry2*/, unsigned int &/*geometry1*/> >
						closest_segment_indices_reversed;
				if (closest_segment_indices)
				{
					closest_segment_indices_reversed = boost::make_tuple(
							boost::ref(boost::get<1>(closest_segment_indices.get())),
							boost::ref(boost::get<0>(closest_segment_indices.get())));
				}

				// Recurse into the child nodes of the first geometry.
				minimum_distance_between_bounding_tree_node_of_geometry1_and_two_child_nodes_of_geometry2(
						geometry2_bounding_tree,
						geometry2_sub_tree_node,
						geometry1_bounding_tree,
						geometry1_sub_tree_node,
						min_distance,
						min_distance_threshold,
						closest_positions_reversed,
						closest_segment_indices_reversed);

				return;
			}
			// else the first geometry is at a leaf node and the second is at an internal node...

			// Recurse into the child nodes of the second geometry.
			minimum_distance_between_bounding_tree_node_of_geometry1_and_two_child_nodes_of_geometry2(
					geometry1_bounding_tree,
					geometry1_sub_tree_node,
					geometry2_bounding_tree,
					geometry2_sub_tree_node,
					min_distance,
					min_distance_threshold,
					closest_positions,
					closest_segment_indices);
		}


		/**
		 * Calculate (and update) the minimum distance between a bounding tree node of the first polyline or polygon,
		 * and two child bounding tree nodes of the second polyline or polygon.
		 *
		 * This is essentially a recursion into the bounding tree of the second geometry.
		 */
		template <typename GreatCircleArcConstIterator1Type, typename GreatCircleArcConstIterator2Type>
		void
		minimum_distance_between_bounding_tree_node_of_geometry1_and_two_child_nodes_of_geometry2(
				const PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type> &geometry1_bounding_tree,
				const typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type>::node_type &geometry1_sub_tree_node,
				const PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type> &geometry2_bounding_tree,
				const typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type>::node_type &geometry2_sub_tree_node,
				AngularDistance &min_distance,
				AngularExtent &min_distance_threshold,
				const boost::optional< boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/> > &closest_positions,
				const boost::optional< boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/> > &closest_segment_indices)
		{
			typedef PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type> geometry2_bounding_tree_type;

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					geometry2_sub_tree_node.is_internal_node(),
					GPLATES_ASSERTION_SOURCE);

			// The child nodes of the second geometry.
			const typename geometry2_bounding_tree_type::node_type geometry2_child_nodes[2] =
			{
				geometry2_bounding_tree.get_child_node(geometry2_sub_tree_node, 0),
				geometry2_bounding_tree.get_child_node(geometry2_sub_tree_node, 1)
			};

			// The minimum distance between the (bounding small circle) centre of node of the first geometry
			// and the bounding small circles of the child nodes of the second geometry.
			//
			// We could have found the minimum distance to the 'bounding small circle' of the node
			// of the first geometry instead of its 'centre' but it doesn't change the relative difference
			// between the minimum distances to each child node (of the second geometry) - also less
			// likely to clamp minimum distances to zero which makes it harder to determine which
			// child node is closer.
			const AngularDistance min_distance_geometry1_node_centre_to_geometry2_child_node_bounding_small_circles[2] =
			{
				minimum_distance(
						geometry1_sub_tree_node.get_bounding_small_circle().get_centre(),
						geometry2_child_nodes[0].get_bounding_small_circle()),
				minimum_distance(
						geometry1_sub_tree_node.get_bounding_small_circle().get_centre(),
						geometry2_child_nodes[1].get_bounding_small_circle())
			};

			// Visit the closest child node (of the second geometry) first since it can avoid unnecessary
			// calculations when visiting the furthest child node (because more likely to exceed the threshold).
			unsigned int geometry2_child_node_visit_indices[2];
			if (min_distance_geometry1_node_centre_to_geometry2_child_node_bounding_small_circles[0] <
				min_distance_geometry1_node_centre_to_geometry2_child_node_bounding_small_circles[1])
			{
				geometry2_child_node_visit_indices[0] = 0;
				geometry2_child_node_visit_indices[1] = 1;
			}
			else if (min_distance_geometry1_node_centre_to_geometry2_child_node_bounding_small_circles[0] >
				min_distance_geometry1_node_centre_to_geometry2_child_node_bounding_small_circles[1])
			{
				geometry2_child_node_visit_indices[0] = 1;
				geometry2_child_node_visit_indices[1] = 0;
			}
			else
			{
				// Both child node bounding small circles are the same distance (within epsilon)
				// from the centre of the first geometry's node. Most likely the centre of first
				// geometry's node is inside the bounding small circles of both nodes (ie, both
				// angular distances got clamped to AngularDistance::ZERO).
				// In this case we'll visit the largest child node first since this can result in
				// fewer minimum distance tests between bounding small circles of sub-tree nodes.
				if (geometry2_child_nodes[0].get_bounding_small_circle().get_angular_extent().is_precisely_greater_than(
					geometry2_child_nodes[1].get_bounding_small_circle().get_angular_extent()))
				{
					geometry2_child_node_visit_indices[0] = 0;
					geometry2_child_node_visit_indices[1] = 1;
				}
				else
				{
					geometry2_child_node_visit_indices[0] = 1;
					geometry2_child_node_visit_indices[1] = 0;
				}
			}

			// Iterate over the child nodes.
			for (unsigned int n = 0; n < 2; ++n)
			{
				const unsigned int geometry2_child_offset = geometry2_child_node_visit_indices[n];

				// If the minimum distance between the node of the first geometry and the current child node
				// of the second geometry exceeds the current threshold then skip the current child node.
				const AngularExtent min_distance_geometry1_node_to_geometry2_child_node =
						min_distance_geometry1_node_centre_to_geometry2_child_node_bounding_small_circles[geometry2_child_offset] -
							geometry1_sub_tree_node.get_bounding_small_circle().get_angular_extent();
				if (min_distance_geometry1_node_to_geometry2_child_node.is_precisely_greater_than(min_distance_threshold))
				{
					continue;
				}

				minimum_distance_between_bounding_tree_nodes_of_two_geometries(
						geometry1_bounding_tree,
						geometry1_sub_tree_node,
						geometry2_bounding_tree,
						geometry2_child_nodes[geometry2_child_offset],
						min_distance,
						min_distance_threshold,
						closest_positions,
						closest_segment_indices);
			}
		}
	}
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const GeometryOnSphere &geometry1,
		const GeometryOnSphere &geometry2,
		bool geometry1_interior_is_solid,
		bool geometry2_interior_is_solid,
		boost::optional<const AngularExtent &> minimum_distance_threshold,
		boost::optional< boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/> > closest_indices)
{
	AngularDistance minimum_distance(AngularDistance::PI);

	MinimumDistanceBetweenGeometryOnSpheres visitor(
			geometry2,
			geometry1_interior_is_solid,
			geometry2_interior_is_solid,
			minimum_distance,
			minimum_distance_threshold,
			closest_positions,
			closest_indices);

	geometry1.accept_visitor(visitor);

	return minimum_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PointOnSphere &point1,
		const PointOnSphere &point2,
		boost::optional<const AngularExtent &> minimum_distance_threshold)
{
	const AngularDistance min_distance =
			AngularDistance::create_from_cosine(
					dot(point1.position_vector(), point2.position_vector()));

	// If there's a threshold and the minimum distance is greater than the threshold then
	// return the maximum possible distance (PI) to signal this.
	if (minimum_distance_threshold &&
		min_distance.is_precisely_greater_than(minimum_distance_threshold.get()))
	{
		return AngularDistance::PI;
	}

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PointOnSphere &point,
		const MultiPointOnSphere &multipoint,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional<UnitVector3D &> closest_position_in_multipoint,
		boost::optional<unsigned int &> closest_position_index_in_multipoint)
{
	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the point is further away, from the multi-point's bounding small circle, than the
		// threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(point, multipoint.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	// Iterate over the points in the multi-point.
	MultiPointOnSphere::const_iterator multipoint_iter = multipoint.begin();
	MultiPointOnSphere::const_iterator multipoint_end = multipoint.end();
	unsigned int multipoint_point_index = 0;
	for ( ; multipoint_iter != multipoint_end; ++multipoint_iter, ++multipoint_point_index)
	{
		const PointOnSphere &multipoint_point = *multipoint_iter;

		const AngularDistance min_distance_point_to_multipoint_point =
				AngularDistance::create_from_cosine(
						dot(point.position_vector(), multipoint_point.position_vector()));

		// If shortest distance so far (within threshold)...
		if (min_distance_point_to_multipoint_point.is_precisely_less_than(min_distance) &&
			min_distance_point_to_multipoint_point.is_precisely_less_than(min_distance_threshold))
		{
			min_distance = min_distance_point_to_multipoint_point;
			if (closest_position_in_multipoint)
			{
				closest_position_in_multipoint.get() = multipoint_point.position_vector();
			}
			if (closest_position_index_in_multipoint)
			{
				closest_position_index_in_multipoint.get() = multipoint_point_index;
			}
		}
	}

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PointOnSphere &point,
		const PolylineOnSphere &polyline,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional<UnitVector3D &> closest_position_on_polyline,
		boost::optional<unsigned int &> closest_segment_index_in_polyline)
{
	const PolylineOnSphere::bounding_tree_type &polyline_bounding_tree = polyline.get_bounding_tree();

	const PolylineOnSphere::bounding_tree_type::node_type polyline_bounding_tree_root_node =
			polyline_bounding_tree.get_root_node();

	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	// Note that after each minimum distance component calculation we update the threshold
	// with the updated minimum distance.
	//
	// This avoids overwriting the closest point (so far) with a point that is further away.
	//
	// This is also an optimisation that can avoid calculating the closest point in some situations
	// where the next component minimum distance is greater than the current minimum distance.
	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the point is further away (from the root node's bounding small circle) than the
		// threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(point, polyline_bounding_tree_root_node.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	minimum_distance_between_point_and_poly_geometry_bounding_tree_node(
			point,
			polyline_bounding_tree,
			polyline_bounding_tree_root_node,
			min_distance,
			min_distance_threshold,
			closest_position_on_polyline,
			closest_segment_index_in_polyline);

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PointOnSphere &point,
		const PolygonOnSphere &polygon,
		bool polygon_interior_is_solid,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional<UnitVector3D &> closest_position_on_polygon_outline,
		boost::optional<unsigned int &> closest_segment_index_in_polygon)
{
	const PolygonOnSphere::bounding_tree_type &polygon_bounding_tree = polygon.get_bounding_tree();

	const PolygonOnSphere::bounding_tree_type::node_type polygon_bounding_tree_root_node =
			polygon_bounding_tree.get_root_node();

	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	// Note that after each minimum distance component calculation we update the threshold
	// with the updated minimum distance.
	//
	// This avoids overwriting the closest point (so far) with a point that is further away.
	//
	// This is also an optimisation that can avoid calculating the closest point in some situations
	// where the next component minimum distance is greater than the current minimum distance.
	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the point is further away (from the root node's bounding small circle) than the
		// threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(point, polygon_bounding_tree_root_node.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	// Test point-in-polygon-interior after testing against root node bounding small circle since
	// the latter is a lot cheaper to test.
	if (polygon_interior_is_solid)
	{
		if (polygon.is_point_in_polygon(point))
		{
			// Find the closest point and/or segment (on the polygon's outline).
			if (closest_position_on_polygon_outline ||
				closest_segment_index_in_polygon)
			{
				// Don't use a threshold since we now need to find the closest segment regardless because
				// the polygon interior is solid (and hence threshold is zero and never exceeded).
				min_distance = AngularDistance::PI;
				min_distance_threshold = AngularExtent::PI;

				minimum_distance_between_point_and_poly_geometry_bounding_tree_node(
						point,
						polygon_bounding_tree,
						polygon_bounding_tree_root_node,
						min_distance,
						min_distance_threshold,
						closest_position_on_polygon_outline,
						closest_segment_index_in_polygon);
			}

			// Anything intersecting the polygon interior is considered zero distance
			// which is also below any possible minimum distance threshold.
			return AngularDistance::ZERO;
		}
	}

	minimum_distance_between_point_and_poly_geometry_bounding_tree_node(
			point,
			polygon_bounding_tree,
			polygon_bounding_tree_root_node,
			min_distance,
			min_distance_threshold,
			closest_position_on_polygon_outline,
			closest_segment_index_in_polygon);

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const MultiPointOnSphere &multipoint1,
		const MultiPointOnSphere &multipoint2,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional< boost::tuple<UnitVector3D &/*multipoint1*/, UnitVector3D &/*multipoint2*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*multipoint1*/, unsigned int &/*multipoint2*/> > closest_position_indices)
{
	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the bounding small circles of the two multi-points are further away than the
		// threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(
				multipoint1.get_bounding_small_circle(),
				multipoint2.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	boost::optional<UnitVector3D &> closest_position_in_smaller_multipoint;
	boost::optional<UnitVector3D &> closest_position_in_larger_multipoint;

	boost::optional<unsigned int &> closest_position_index_in_smaller_multipoint;
	boost::optional<unsigned int &> closest_position_index_in_larger_multipoint;

	const MultiPointOnSphere *larger_multipoint = NULL;
	const MultiPointOnSphere *smaller_multipoint = NULL;

	// Recurse into the larger multi-point first.
	// This gives the greatest chance of early rejection of point-to-multi-point minimum distances.
	if (multipoint1.get_bounding_small_circle().get_angular_extent().is_precisely_greater_than(
		multipoint2.get_bounding_small_circle().get_angular_extent()))
	{
		larger_multipoint = &multipoint1;
		smaller_multipoint = &multipoint2;

		if (closest_positions)
		{
			closest_position_in_larger_multipoint = boost::get<0>(closest_positions.get());
			closest_position_in_smaller_multipoint = boost::get<1>(closest_positions.get());
		}
		if (closest_position_indices)
		{
			closest_position_index_in_larger_multipoint = boost::get<0>(closest_position_indices.get());
			closest_position_index_in_smaller_multipoint = boost::get<1>(closest_position_indices.get());
		}
	}
	else // second multi-point is larger...
	{
		larger_multipoint = &multipoint2;
		smaller_multipoint = &multipoint1;

		if (closest_positions)
		{
			closest_position_in_larger_multipoint = boost::get<1>(closest_positions.get());
			closest_position_in_smaller_multipoint = boost::get<0>(closest_positions.get());
		}
		if (closest_position_indices)
		{
			closest_position_index_in_larger_multipoint = boost::get<1>(closest_position_indices.get());
			closest_position_index_in_smaller_multipoint = boost::get<0>(closest_position_indices.get());
		}
	}

	// Iterate over the points in the larger multi-point.
	MultiPointOnSphere::const_iterator larger_multipoint_iter = larger_multipoint->begin();
	MultiPointOnSphere::const_iterator larger_multipoint_end = larger_multipoint->end();
	unsigned int larger_multipoint_point_index = 0;
	for ( ;
		larger_multipoint_iter != larger_multipoint_end;
		++larger_multipoint_iter, ++larger_multipoint_point_index)
	{
		const PointOnSphere &larger_multipoint_point = *larger_multipoint_iter;

		const AngularDistance min_distance_larger_multipoint_point_to_smaller_multipoint =
				minimum_distance(
						larger_multipoint_point,
						*smaller_multipoint,
						min_distance_threshold,
						closest_position_in_smaller_multipoint,
						closest_position_index_in_smaller_multipoint);

		// If shortest distance so far (within threshold)...
		if (min_distance_larger_multipoint_point_to_smaller_multipoint.is_precisely_less_than(min_distance))
		{
			min_distance = min_distance_larger_multipoint_point_to_smaller_multipoint;
			min_distance_threshold = AngularExtent(min_distance);
			if (closest_position_in_larger_multipoint)
			{
				closest_position_in_larger_multipoint.get() = larger_multipoint_point.position_vector();
			}
			if (closest_position_index_in_larger_multipoint)
			{
				closest_position_index_in_larger_multipoint.get() = larger_multipoint_point_index;
			}
		}
	}

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const MultiPointOnSphere &multipoint,
		const PolylineOnSphere &polyline,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional< boost::tuple<UnitVector3D &/*multipoint*/, UnitVector3D &/*polyline*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*multipoint*/, unsigned int &/*polyline*/> > closest_indices)
{
	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the bounding small circles of the multi-point and the polyline are further away than
		// the threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(
				multipoint.get_bounding_small_circle(),
				polyline.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	boost::optional<UnitVector3D &> closest_position_in_multipoint;
	boost::optional<UnitVector3D &> closest_position_on_polyline;
	if (closest_positions)
	{
		closest_position_in_multipoint = boost::get<0>(closest_positions.get());
		closest_position_on_polyline = boost::get<1>(closest_positions.get());
	}

	boost::optional<unsigned int &> closest_position_index_in_multipoint;
	boost::optional<unsigned int &> closest_segment_index_in_polyline;
	if (closest_indices)
	{
		closest_position_index_in_multipoint = boost::get<0>(closest_indices.get());
		closest_segment_index_in_polyline = boost::get<1>(closest_indices.get());
	}

	// Iterate over the points in the multi-point.
	MultiPointOnSphere::const_iterator multipoint_iter = multipoint.begin();
	MultiPointOnSphere::const_iterator multipoint_end = multipoint.end();
	unsigned int multipoint_point_index = 0;
	for ( ; multipoint_iter != multipoint_end; ++multipoint_iter, ++multipoint_point_index)
	{
		const PointOnSphere &multipoint_point = *multipoint_iter;

		const AngularDistance min_distance_multipoint_point_to_polyline =
				minimum_distance(
						multipoint_point,
						polyline,
						min_distance_threshold,
						closest_position_on_polyline,
						closest_segment_index_in_polyline);

		// If shortest distance so far (within threshold)...
		if (min_distance_multipoint_point_to_polyline.is_precisely_less_than(min_distance))
		{
			min_distance = min_distance_multipoint_point_to_polyline;
			min_distance_threshold = AngularExtent(min_distance);
			if (closest_position_in_multipoint)
			{
				closest_position_in_multipoint.get() = multipoint_point.position_vector();
			}
			if (closest_position_index_in_multipoint)
			{
				closest_position_index_in_multipoint.get() = multipoint_point_index;
			}
		}
	}

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const MultiPointOnSphere &multipoint,
		const PolygonOnSphere &polygon,
		bool polygon_interior_is_solid,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional< boost::tuple<UnitVector3D &/*multipoint*/, UnitVector3D &/*polygon*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*multipoint*/, unsigned int &/*polygon*/> > closest_indices)
{
	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the bounding small circles of the multi-point and the polygon are further away than
		// the threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(
				multipoint.get_bounding_small_circle(),
				polygon.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	boost::optional<UnitVector3D &> closest_position_in_multipoint;
	boost::optional<UnitVector3D &> closest_position_on_polygon;
	if (closest_positions)
	{
		closest_position_in_multipoint = boost::get<0>(closest_positions.get());
		closest_position_on_polygon = boost::get<1>(closest_positions.get());
	}

	boost::optional<unsigned int &> closest_position_index_in_multipoint;
	boost::optional<unsigned int &> closest_segment_index_in_polygon;
	if (closest_indices)
	{
		closest_position_index_in_multipoint = boost::get<0>(closest_indices.get());
		closest_segment_index_in_polygon = boost::get<1>(closest_indices.get());
	}

	// Iterate over the points in the multi-point.
	MultiPointOnSphere::const_iterator multipoint_iter = multipoint.begin();
	MultiPointOnSphere::const_iterator multipoint_end = multipoint.end();
	unsigned int multipoint_point_index = 0;
	for ( ; multipoint_iter != multipoint_end; ++multipoint_iter, ++multipoint_point_index)
	{
		const PointOnSphere &multipoint_point = *multipoint_iter;

		const AngularDistance min_distance_multipoint_point_to_polygon =
				minimum_distance(
						multipoint_point,
						polygon,
						polygon_interior_is_solid,
						min_distance_threshold,
						closest_position_on_polygon,
						closest_segment_index_in_polygon);

		// If shortest distance so far (within threshold)...
		if (min_distance_multipoint_point_to_polygon.is_precisely_less_than(min_distance))
		{
			min_distance = min_distance_multipoint_point_to_polygon;
			min_distance_threshold = AngularExtent(min_distance);
			if (closest_position_in_multipoint)
			{
				closest_position_in_multipoint.get() = multipoint_point.position_vector();
			}
			if (closest_position_index_in_multipoint)
			{
				closest_position_index_in_multipoint.get() = multipoint_point_index;
			}
		}
	}

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PolylineOnSphere &polyline,
		const MultiPointOnSphere &multipoint,
		boost::optional<const AngularExtent &> minimum_distance_threshold,
		boost::optional< boost::tuple<UnitVector3D &/*polyline*/, UnitVector3D &/*multipoint*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*polyline*/, unsigned int &/*multipoint*/> > closest_indices)
{
	// Since we're swapping the order of the geometries we also need to swap the closest position
	// references and the closest index references.
	boost::optional< boost::tuple<UnitVector3D &/*multipoint*/, UnitVector3D &/*polyline*/> >
			closest_positions_reversed;
	if (closest_positions)
	{
		closest_positions_reversed = boost::make_tuple(
				boost::ref(boost::get<1>(closest_positions.get())),
				boost::ref(boost::get<0>(closest_positions.get())));
	}
	boost::optional< boost::tuple<unsigned int &/*multipoint*/, unsigned int &/*polyline*/> >
			closest_indices_reversed;
	if (closest_indices)
	{
		closest_indices_reversed = boost::make_tuple(
				boost::ref(boost::get<1>(closest_indices.get())),
				boost::ref(boost::get<0>(closest_indices.get())));
	}

	return minimum_distance(
			multipoint,
			polyline,
			minimum_distance_threshold,
			closest_positions_reversed,
			closest_indices_reversed);
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PolylineOnSphere &polyline1,
		const PolylineOnSphere &polyline2,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional< boost::tuple<UnitVector3D &/*polyline1*/, UnitVector3D &/*polyline2*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*polyline1*/, unsigned int &/*polyline2*/> > closest_segment_indices)
{
	const PolylineOnSphere::bounding_tree_type &polyline1_bounding_tree = polyline1.get_bounding_tree();
	const PolylineOnSphere::bounding_tree_type &polyline2_bounding_tree = polyline2.get_bounding_tree();

	const PolylineOnSphere::bounding_tree_type::node_type polyline1_bounding_tree_root_node =
			polyline1_bounding_tree.get_root_node();
	const PolylineOnSphere::bounding_tree_type::node_type polyline2_bounding_tree_root_node =
			polyline2_bounding_tree.get_root_node();

	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	// Note that after each minimum distance component calculation we update the threshold
	// with the updated minimum distance.
	//
	// This avoids overwriting the closest point (so far) with a point that is further away.
	//
	// This is also an optimisation that can avoid calculating the closest point in some situations
	// where the next component minimum distance is greater than the current minimum distance.
	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the root node bounding small circles of the two geometries are further away than the
		// threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(
				polyline1_bounding_tree_root_node.get_bounding_small_circle(),
				polyline2_bounding_tree_root_node.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	minimum_distance_between_bounding_tree_nodes_of_two_geometries(
			polyline1_bounding_tree,
			polyline1_bounding_tree_root_node,
			polyline2_bounding_tree,
			polyline2_bounding_tree_root_node,
			min_distance,
			min_distance_threshold,
			closest_positions,
			closest_segment_indices);

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PolylineOnSphere &polyline,
		const PolygonOnSphere &polygon,
		bool polygon_interior_is_solid,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional< boost::tuple<UnitVector3D &/*polyline*/, UnitVector3D &/*polygon*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*polyline*/, unsigned int &/*polygon*/> > closest_segment_indices)
{
	const PolylineOnSphere::bounding_tree_type &polyline_bounding_tree = polyline.get_bounding_tree();
	const PolygonOnSphere::bounding_tree_type &polygon_bounding_tree = polygon.get_bounding_tree();

	const PolylineOnSphere::bounding_tree_type::node_type polyline_bounding_tree_root_node =
			polyline_bounding_tree.get_root_node();
	const PolygonOnSphere::bounding_tree_type::node_type polygon_bounding_tree_root_node =
			polygon_bounding_tree.get_root_node();

	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	// Note that after each minimum distance component calculation we update the threshold
	// with the updated minimum distance.
	//
	// This avoids overwriting the closest point (so far) with a point that is further away.
	//
	// This is also an optimisation that can avoid calculating the closest point in some situations
	// where the next component minimum distance is greater than the current minimum distance.
	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the root node bounding small circles of the two geometries are further away than the
		// threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(
				polyline_bounding_tree_root_node.get_bounding_small_circle(),
				polygon_bounding_tree_root_node.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	minimum_distance_between_bounding_tree_nodes_of_two_geometries(
			polyline_bounding_tree,
			polyline_bounding_tree_root_node,
			polygon_bounding_tree,
			polygon_bounding_tree_root_node,
			min_distance,
			min_distance_threshold,
			closest_positions,
			closest_segment_indices);

	// If the polygon interior is solid and the polyline has not intersected the polygon boundary then
	// it's possible the polyline is completely inside the polygon which also counts as an intersection.
	if (polygon_interior_is_solid &&
		min_distance != AngularDistance::ZERO /*epsilon comparison*/)
	{
		// If the polyline is completely inside the polygon then we only need to test if one of
		// the polyline's points (any arbitrary point) is inside the polygon (because we know the
		// polyline did not intersect the polygon boundary).
		if (polygon.is_point_in_polygon(polyline.start_point()/*arbitrary*/))
		{
			if (closest_positions ||
				closest_segment_indices)
			{
				// Find the closest position and segment in polyline and the closest position and
				// segment in polygon's *outline* (if haven't already found).
				if (min_distance == AngularDistance::PI /*epsilon comparison*/)
				{
					// Don't use a threshold since we now need to find the closest points and
					// segments regardless.
					min_distance = AngularDistance::PI;
					min_distance_threshold = AngularExtent::PI;

					// Note that we have to call this a second time because the first time (above)
					// determined if the polyline intersected the *outline* of the polygon (and this
					// can happen even if none of the polyline's points are inside the polygon).
					// But the first time used a threshold and did not find the closest points
					// and/or segments since they were separated by a distance greater than the threshold.
					// Note that because the polygon is solid and the polyline is inside the polygon
					// we can never exceed the threshold.
					minimum_distance_between_bounding_tree_nodes_of_two_geometries(
							polyline_bounding_tree,
							polyline_bounding_tree_root_node,
							polygon_bounding_tree,
							polygon_bounding_tree_root_node,
							min_distance,
							min_distance_threshold,
							closest_positions,
							closest_segment_indices);
				}
			}

			// Anything intersecting the polygon interior is considered zero distance
			// which is also below any possible minimum distance threshold.
			return AngularDistance::ZERO;
		}
	}

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PolygonOnSphere &polygon,
		const MultiPointOnSphere &multipoint,
		bool polygon_interior_is_solid,
		boost::optional<const AngularExtent &> minimum_distance_threshold,
		boost::optional< boost::tuple<UnitVector3D &/*polygon*/, UnitVector3D &/*multipoint*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*polygon*/, unsigned int &/*multipoint*/> > closest_indices)
{
	// Since we're swapping the order of the geometries we also need to swap the closest position references.
	boost::optional< boost::tuple<UnitVector3D &/*multipoint*/, UnitVector3D &/*polygon*/> >
			closest_positions_reversed;
	if (closest_positions)
	{
		closest_positions_reversed = boost::make_tuple(
				boost::ref(boost::get<1>(closest_positions.get())),
				boost::ref(boost::get<0>(closest_positions.get())));
	}
	boost::optional< boost::tuple<unsigned int &/*multipoint*/, unsigned int &/*polygon*/> >
			closest_indices_reversed;
	if (closest_indices)
	{
		closest_indices_reversed = boost::make_tuple(
				boost::ref(boost::get<1>(closest_indices.get())),
				boost::ref(boost::get<0>(closest_indices.get())));
	}

	return minimum_distance(
			multipoint,
			polygon,
			polygon_interior_is_solid,
			minimum_distance_threshold,
			closest_positions_reversed,
			closest_indices_reversed);
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PolygonOnSphere &polygon,
		const PolylineOnSphere &polyline,
		bool polygon_interior_is_solid,
		boost::optional<const AngularExtent &> minimum_distance_threshold,
		boost::optional< boost::tuple<UnitVector3D &/*polygon*/, UnitVector3D &/*polyline*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*polygon*/, unsigned int &/*polyline*/> > closest_segment_indices)
{
	// Since we're swapping the order of the geometries we also need to swap the closest position references.
	boost::optional< boost::tuple<UnitVector3D &/*polyline*/, UnitVector3D &/*polygon*/> >
			closest_positions_reversed;
	if (closest_positions)
	{
		closest_positions_reversed = boost::make_tuple(
				boost::ref(boost::get<1>(closest_positions.get())),
				boost::ref(boost::get<0>(closest_positions.get())));
	}

	// Since we're swapping the order of the geometries we also need to swap the closest segment references.
	boost::optional< boost::tuple<unsigned int &/*geometry2*/, unsigned int &/*geometry1*/> >
			closest_segment_indices_reversed;
	if (closest_segment_indices)
	{
		closest_segment_indices_reversed = boost::make_tuple(
				boost::ref(boost::get<1>(closest_segment_indices.get())),
				boost::ref(boost::get<0>(closest_segment_indices.get())));
	}

	return minimum_distance(
			polyline,
			polygon,
			polygon_interior_is_solid,
			minimum_distance_threshold,
			closest_positions_reversed,
			closest_segment_indices_reversed);
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PolygonOnSphere &polygon1,
		const PolygonOnSphere &polygon2,
		bool polygon1_interior_is_solid,
		bool polygon2_interior_is_solid,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional< boost::tuple<UnitVector3D &/*polygon1*/, UnitVector3D &/*polygon2*/> > closest_positions,
		boost::optional< boost::tuple<unsigned int &/*polygon1*/, unsigned int &/*polygon2*/> > closest_segment_indices)
{
	const PolygonOnSphere::bounding_tree_type &polygon1_bounding_tree = polygon1.get_bounding_tree();
	const PolygonOnSphere::bounding_tree_type &polygon2_bounding_tree = polygon2.get_bounding_tree();

	const PolygonOnSphere::bounding_tree_type::node_type polygon1_bounding_tree_root_node =
			polygon1_bounding_tree.get_root_node();
	const PolygonOnSphere::bounding_tree_type::node_type polygon2_bounding_tree_root_node =
			polygon2_bounding_tree.get_root_node();

	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	// Note that after each minimum distance component calculation we update the threshold
	// with the updated minimum distance.
	//
	// This avoids overwriting the closest point (so far) with a point that is further away.
	//
	// This is also an optimisation that can avoid calculating the closest point in some situations
	// where the next component minimum distance is greater than the current minimum distance.
	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the root node bounding small circles of the two geometries are further away than the
		// threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(
				polygon1_bounding_tree_root_node.get_bounding_small_circle(),
				polygon2_bounding_tree_root_node.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	minimum_distance_between_bounding_tree_nodes_of_two_geometries(
			polygon1_bounding_tree,
			polygon1_bounding_tree_root_node,
			polygon2_bounding_tree,
			polygon2_bounding_tree_root_node,
			min_distance,
			min_distance_threshold,
			closest_positions,
			closest_segment_indices);

	// If polygon1's interior is solid and polygon2 has not intersected polygon1's boundary then
	// it's possible polygon2 is completely inside polygon1 which also counts as an intersection.
	if (polygon1_interior_is_solid &&
		min_distance != AngularDistance::ZERO /*epsilon comparison*/)
	{
		// If polygon2 is completely inside polygon1 then we only need to test if one of
		// the polygon2's points (any arbitrary point) is inside polygon1 (because we know that
		// polygon2 did not intersect polygon1's boundary).
		if (polygon1.is_point_in_polygon(polygon2.first_exterior_ring_vertex()/*arbitrary*/))
		{
			if (closest_positions ||
				closest_segment_indices)
			{
				// Find the closest positions/segments in the polygon boundaries (if haven't already found).
				if (min_distance == AngularDistance::PI /*epsilon comparison*/)
				{
					// Don't use a threshold since we now need to find the closest points and
					// segments regardless.
					min_distance = AngularDistance::PI;
					min_distance_threshold = AngularExtent::PI;

					// Note that we have to call this a second time because the first time (above)
					// determined if the polygon outlines intersected (and this can happen even if
					// none of polyline2's points are inside polygon1).
					// But the first time used a threshold and did not find the closest points
					// and/or segments since they were separated by a distance greater than the threshold.
					// Note that because polygon1 is solid and polygon2 is inside polygon1
					// we can never exceed the threshold.
					minimum_distance_between_bounding_tree_nodes_of_two_geometries(
							polygon1_bounding_tree,
							polygon1_bounding_tree_root_node,
							polygon2_bounding_tree,
							polygon2_bounding_tree_root_node,
							min_distance,
							min_distance_threshold,
							closest_positions,
							closest_segment_indices);
				}
			}

			// Anything intersecting polygon1's interior is considered zero distance
			// which is also below any possible minimum distance threshold.
			return AngularDistance::ZERO;
		}
	}

	// If polygon2's interior is solid and polygon1 has not intersected polygon2's boundary then
	// it's possible polygon1 is completely inside polygon2 which also counts as an intersection.
	if (polygon2_interior_is_solid &&
		min_distance != AngularDistance::ZERO /*epsilon comparison*/)
	{
		// If polygon1 is completely inside polygon2 then we only need to test if one of
		// the polygon1's points (any arbitrary point) is inside polygon2 (because we know that
		// polygon1 did not intersect polygon2's boundary).
		if (polygon2.is_point_in_polygon(polygon1.first_exterior_ring_vertex()/*arbitrary*/))
		{
			if (closest_positions ||
				closest_segment_indices)
			{
				// Find the closest positions/segments in the polygon boundaries (if haven't already found).
				if (min_distance == AngularDistance::PI /*epsilon comparison*/)
				{
					// Don't use a threshold since we now need to find the closest points and
					// segments regardless.
					min_distance = AngularDistance::PI;
					min_distance_threshold = AngularExtent::PI;

					// Note that we have to call this a second time because the first time (above)
					// determined if the polygon outlines intersected (and this can happen even if
					// none of polyline1's points are inside polygon2).
					// But the first time used a threshold and did not find the closest points
					// and/or segments since they were separated by a distance greater than the threshold.
					// Note that because polygon2 is solid and polygon1 is inside polygon2
					// we can never exceed the threshold.
					minimum_distance_between_bounding_tree_nodes_of_two_geometries(
							polygon1_bounding_tree,
							polygon1_bounding_tree_root_node,
							polygon2_bounding_tree,
							polygon2_bounding_tree_root_node,
							min_distance,
							min_distance_threshold,
							closest_positions,
							closest_segment_indices);
				}
			}

			// Anything intersecting polygon1's interior is considered zero distance
			// which is also below any possible minimum distance threshold.
			return AngularDistance::ZERO;
		}
	}

	return min_distance;
}
