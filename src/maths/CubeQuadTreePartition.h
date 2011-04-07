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

#ifndef GPLATES_MATHS_CUBEQUADTREEPARTITION_H
#define GPLATES_MATHS_CUBEQUADTREEPARTITION_H

#include <cmath>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/pool/object_pool.hpp>

#include "UnitVector3D.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/IntrusiveSinglyLinkedList.h"
#include "utils/Profile.h"


namespace GPlatesMaths
{
	/**
	 * A spatial partition of geometries on the globe based on a cube with each cube face
	 * containing a 'loose' quad tree.
	 *
	 * The cube is actually projected onto the globe (projected towards the centre of the globe).
	 * So while the quad tree of each cube face is nice and evenly subdivided at the face of the
	 * cube, each quad tree partition looks distorted when projected onto the globe.
	 * This is fine because when we add geometries to this spatial partition we project them,
	 * or more accurately their bounding circles, onto the appropriate cube face and work with
	 * the quad tree partition where it's nice and regular.
	 *
	 * Each quad tree is a 'loose' quad tree (google "Loose octrees" devised by Thatcher Ulrich).
	 *
	 * The 'loose' means the bounding square around a quad tree node is twice as large as the
	 * actual quad tree node itself. This avoids the problem with regular spatial partitions where
	 * small objects near the splitting lines of the root node (or any nodes near the root)
	 * causes those objects to be inserted into the root node thus losing any performance benefit
	 * that would be obtained by inserting further down in the tree.
	 *
	 * The level or depth at which to insert an element is determined by its bounding radius and
	 * the circle_centre within that level is determined by the centre vector of the bounding circle.
	 *
	 * Also if the bounds are exactly *twice* the size then we can determine the level or depth
	 * at which to insert an element in O(1) time (using a log2 on the element's bounding radius).
	 *
	 * This makes insertions quite fast which is useful for inserting *reconstructed* geometries
	 * at each reconstruction time. The spatial partition then tells us which *reconstructed*
	 * geometries are near each other and also allows hierachical bounds testing.
	 * So this spatial partition is useful for:
	 *  - View-frustum culling,
	 *  - Nearest neighbour testing,
	 *  - And, since rasters also use a cube quad tree (although non-'loose'), the ability to
	 *    find which geometries overlap which parts of a possibly reconstructed raster.
	 *
	 * The template parameter 'ElementType' can be any object that is copy-constructable and
	 * it's typically associated with a single geometry.
	 *
	 * The location at which an element is inserted into the spatial partition is determined
	 * by the bounding small circle of the geometry associated with it (the circle centre and radius).
	 */
	template <typename ElementType>
	class CubeQuadTreePartition :
			private boost::noncopyable
	{
	private:
		/**
		 * Linked list wrapper node around an element that has been added to a quad tree node.
		 */
		struct ElementListNode :
				public GPlatesUtils::IntrusiveSinglyLinkedList<ElementListNode>::Node
		{
			explicit
			ElementListNode(
					const ElementType &element_) :
				element(element_)
			{  }

			ElementType element;
		};

		//! Typedef for a list of elements.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<ElementListNode> element_list_type;

	public:
		/**
		 * Identifies a face of the cube.
		 */
		enum CubeFaceType
		{
			POSITIVE_X = 0,
			NEGATIVE_X,
			POSITIVE_Y,
			NEGATIVE_Y,
			POSITIVE_Z,
			NEGATIVE_Z,

			NUM_FACES
		};


		/**
		 * Used to specify the extent of the bounding circle of a geometry added to the spatial partition.
		 *
		 * Cosines and sines are used since they are more efficient than calculating 'acos', etc.
		 *
		 * Note: This is also useful for region-of-interest queries.
		 * For example, determining which geometries from one spatial partition are within a
		 * specified angular distance of geometries in another spatial partition - this can be
		 * achieved by *extending* the bounds of geometries added to one of the spatial partitions.
		 * Then a simple overlap test becomes a region-of-interest query - for example to perform
		 * a region-of-interest query of 10kms you would extend the bounding circle extent by
		 * the angle subtended by those 10kms.
		 */
		struct BoundingCircleExtent
		{
			/**
			 * Provide both the cosine and sine of the angular amount to extend the bounds by -
			 * this avoids a square root calculation to get the sine from the cosine.
			 *
			 * NOTE: The cosine is just a dot product of two unit vectors.
			 */
			BoundingCircleExtent(
					double cosine_extend_angle_,
					double sine_extend_angle_) :
				cosine_extend_angle(cosine_extend_angle_),
				sine_extend_angle(sine_extend_angle_)
			{  }

			/**
			 * Provide only the cosine of the angular amount to extend the bounds by -
			 * the sine will be calculated.
			 *
			 * NOTE: The cosine is just a dot product of two unit vectors.
			 */
			explicit
			BoundingCircleExtent(
					double cosine_extend_angle_) :
				cosine_extend_angle(cosine_extend_angle_),
				// The 1e-9 is to avoid sqrt of a negative number due to numerical tolerance.
				sine_extend_angle(std::sqrt(1+1e-9 - cosine_extend_angle_ * cosine_extend_angle_))
			{  }

			/**
			 * A member function for adding two extents - useful for extending a geometry's
			 * bounding circle for use in region-of-interest queries.
			 *
			 * Even though it works with cosines and sines it effectively adds
			 * the two angular extents as angles.
			 * For example, an extent with 'a' radians plus an extent with 'b' radians
			 * gives an extent with 'a+b' radians.
			 */
			inline
			BoundingCircleExtent
			operator+(
					const BoundingCircleExtent &rhs) const
			{
				return BoundingCircleExtent(
						// cos(a+b) = cos(a)cos(b) - sin(a)sin(b)
						cosine_extend_angle * rhs.cosine_extend_angle -
							sine_extend_angle * rhs.sine_extend_angle,
						// sin(a+b) = sin(a)cos(b) + cos(a)sin(b)
						sine_extend_angle * rhs.cosine_extend_angle +
							cosine_extend_angle * rhs.sine_extend_angle);
			}

			double cosine_extend_angle;
			double sine_extend_angle;
		};


		/**
		 * Constructor.
		 *
		 * @param maximum_depth is the deepest level that an element can be added to.
		 *        The maximum amount of memory required for the nodes themselves (assuming all
		 *        nodes of all levels of all quad trees contain elements) is
		 *        '6 * pow(4, maximum_depth) * 1.3 * 20' bytes - the 6 is for the six cube faces,
		 *        the 1.3 is summation over the levels and the last number is the size of a
		 *        quad tree node in bytes (on 32-bit systems).
		 *        This does not include the memory used by the elements themselves.
		 */
		explicit
		CubeQuadTreePartition(
				unsigned int maximum_depth);


		/**
		 * Add an element, to the spatial partition, that is associated with a point geometry.
		 *
		 * The location at which the element is added in the spatial partition
		 * is determined solely by the location of the point.
		 *
		 * To add a point geometry with a region-of-interest extent use the other @a add overload.
		 */
		void
		add(
				const ElementType &element,
				const UnitVector3D &point_geometry);


		/**
		 * Add an element, to the spatial partition, that has a finite spatial extent.
		 *
		 * The location at which the element is added in the spatial partition
		 * is determined by the small circle bounding the element's geometry - both
		 * the small circle centre and the small circle radius (in the form of a dot product).
		 *
		 * NOTE: This can also be used for region-of-interest queries if you add the
		 * bounding circle extent to the region-of-interest extent and pass that into this method.
		 */
		void
		add(
				const ElementType &element,
				const UnitVector3D &bounding_circle_centre,
				const BoundingCircleExtent &bounding_circle_extent);


		/**
		 * A node in a quad tree.
		 *
		 * This can be used to traverse a quad tree of one of the cube faces.
		 */
		class Node
		{
		public:
			Node()
			{
				d_children[0][0] = d_children[0][1] = d_children[1][0] = d_children[1][1] = NULL;
			}

		private:
			/**
			 * Pointers to child nodes.
			 *
			 * If a child node does not exist the corresponding pointer is NULL.
			 */
			Node *d_children[2][2];

			//! Any elements added to this quad tree node.
			element_list_type d_element_list;


			// Make a friend so can add elements.
			friend class CubeQuadTreePartition<ElementType>;

			/**
			 * Add an element already wrapped in a linked list node.
			 *
			 * The memory is managed by the caller.
			 */
			void
			add_element(
					ElementListNode *element_list_node)
			{
				d_element_list.push_front(element_list_node);
			}
		};

	private:
		//! Each cube face has a quad tree.
		struct QuadTree
		{
			//InitialFillNodesLookup initial_file_nodes_lookup;
			Node root_node;
		};

		//! Typedef for an object pool for type @a Node.
		typedef boost::object_pool<Node> quad_tree_node_pool_type;

		//! Typedef for an object pool for type @a ElementListNode.
		typedef boost::object_pool<ElementListNode> element_list_node_pool_type;


		/**
		 * A quad tree for each cube face. 
		 */
		QuadTree d_quad_trees[6];

		//! The maximum depth of any quad tree.
		unsigned int d_maximum_depth;

		/**
		 * Any elements that were too big to add to the root node of any quad tree.
		 *
		 * This happens if an element's bounding radius (projected onto the appropriate cube face)
		 * is larger than the width of the cube face (the loose bounds).
		 */
		element_list_type d_element_list;

		/**
		 * All quad tree nodes, except the root nodes, are stored in this pool.
		 */
		quad_tree_node_pool_type d_quad_tree_node_pool;

		/**
		 * All element linked list nodes are stored in this pool.
		 *
		 * These are linked list nodes containing the elements added to the spatial partition.
		 */
		element_list_node_pool_type d_element_list_node_pool;


		/**
		 * Determine which cube face the element's geometry belongs to.
		 * This is determined by the largest component of the element's
		 * bounding-circle-centre vector.
		 */
		static
		unsigned int
		get_cube_face(
				const UnitVector3D &circle_centre,
				double &circle_centre_x_in_cube_face_coords,
				double &circle_centre_y_in_cube_face_coords,
				double &circle_centre_z_in_cube_face_coords);
	};


	template <typename ElementType>
	CubeQuadTreePartition<ElementType>::CubeQuadTreePartition(
			unsigned int maximum_depth) :
		d_maximum_depth(maximum_depth)
	{
	}


	template <typename ElementType>
	unsigned int
	CubeQuadTreePartition<ElementType>::get_cube_face(
			const UnitVector3D &circle_centre,
			double &circle_centre_x_in_cube_face_coords,
			double &circle_centre_y_in_cube_face_coords,
			double &circle_centre_z_in_cube_face_coords)
	{
		const double &circle_centre_x = circle_centre.x().dval();
		const double &circle_centre_y = circle_centre.y().dval();
		const double &circle_centre_z = circle_centre.z().dval();

		// This is much faster than using std::fabsf (according to the profiler).
		// It seems 'std::fabsf' sets the rounding mode of the floating-point unit for
		// each call to 'std::fabsf' which is slow.
		const double abs_circle_centre_x = (circle_centre_x > 0) ? circle_centre_x : -circle_centre_x;
		const double abs_circle_centre_y = (circle_centre_y > 0) ? circle_centre_y : -circle_centre_y;
		const double abs_circle_centre_z = (circle_centre_z > 0) ? circle_centre_z : -circle_centre_z;

		unsigned int cube_face;
		// These are set using the same cube face coord system used
		// in GPlatesOpenGL::GLCubeSubdivision::UV_FACE_DIRECTIONS.
		//
		// TODO: Make that common code more explicit along with the CubeFaceType enumeration.
		if (abs_circle_centre_x > abs_circle_centre_y)
		{
			if (abs_circle_centre_x > abs_circle_centre_z)
			{
				if (circle_centre_x > 0)
				{
					cube_face = POSITIVE_X;
					circle_centre_z_in_cube_face_coords = circle_centre_x;
					circle_centre_x_in_cube_face_coords = -circle_centre_z;
					circle_centre_y_in_cube_face_coords = -circle_centre_y;
				}
				else
				{
					cube_face = NEGATIVE_X;
					circle_centre_z_in_cube_face_coords = -circle_centre_x;
					circle_centre_x_in_cube_face_coords = circle_centre_z;
					circle_centre_y_in_cube_face_coords = -circle_centre_y;
				}
			}
			else
			{
				if (circle_centre_z > 0)
				{
					cube_face = POSITIVE_Z;
					circle_centre_z_in_cube_face_coords = circle_centre_z;
					circle_centre_x_in_cube_face_coords = circle_centre_x;
					circle_centre_y_in_cube_face_coords = -circle_centre_y;
				}
				else
				{
					cube_face = NEGATIVE_Z;
					circle_centre_z_in_cube_face_coords = -circle_centre_z;
					circle_centre_x_in_cube_face_coords = -circle_centre_x;
					circle_centre_y_in_cube_face_coords = -circle_centre_y;
				}
			}
		}
		else if (abs_circle_centre_y > abs_circle_centre_z)
		{
			if (circle_centre_y > 0)
			{
				cube_face = POSITIVE_Y;
				circle_centre_z_in_cube_face_coords = circle_centre_y;
				circle_centre_x_in_cube_face_coords = circle_centre_x;
				circle_centre_y_in_cube_face_coords = circle_centre_z;
			}
			else
			{
				cube_face = NEGATIVE_Y;
				circle_centre_z_in_cube_face_coords = -circle_centre_y;
				circle_centre_x_in_cube_face_coords = circle_centre_x;
				circle_centre_y_in_cube_face_coords = -circle_centre_z;
			}
		}
		else
		{
			if (circle_centre_z > 0)
			{
				cube_face = POSITIVE_Z;
				circle_centre_z_in_cube_face_coords = circle_centre_z;
				circle_centre_x_in_cube_face_coords = circle_centre_x;
				circle_centre_y_in_cube_face_coords = -circle_centre_y;
			}
			else
			{
				cube_face = NEGATIVE_Z;
				circle_centre_z_in_cube_face_coords = -circle_centre_z;
				circle_centre_x_in_cube_face_coords = -circle_centre_x;
				circle_centre_y_in_cube_face_coords = -circle_centre_y;
			}
		}

		return cube_face;
	}


	template <typename ElementType>
	void
	CubeQuadTreePartition<ElementType>::add(
			const ElementType &element,
			const UnitVector3D &point_geometry)
	{
		// Get the nearest cube face to project point geometry onto.
		// Also get the cube face coord system from which to index into its quad tree.
		double circle_centre_x_in_cube_face_coords;
		double circle_centre_y_in_cube_face_coords;
		double circle_centre_z_in_cube_face_coords;
		const unsigned int cube_face =
				get_cube_face(
						point_geometry,
						circle_centre_x_in_cube_face_coords,
						circle_centre_y_in_cube_face_coords,
						circle_centre_z_in_cube_face_coords);

		const double inv_circle_centre_z_in_cube_face_coords =
				1.0 / circle_centre_z_in_cube_face_coords;

		//
		// Since the point geometry has no spatial extents we put it in the deepest
		// level of the quad tree - being a point it will never extend outside the bounds
		// of a node at the deepest level.
		//

		// Calculate the x,y offsets of the quad tree node position in the deepest level.
		// Use a small numerical tolerance to ensure the we keep the offset within range.
		const unsigned int max_level_width_in_nodes = (1 << d_maximum_depth);
		//
		// Note: Using static_cast<int> instead of static_cast<unsigned int> since
		// Visual Studio optimises for 'int' and not 'unsigned int'.
		//
		const int node_x_offset_at_max_depth = static_cast<int>(
				(0.5 - 1e-6) * max_level_width_in_nodes *
					(1 + inv_circle_centre_z_in_cube_face_coords * circle_centre_x_in_cube_face_coords));
		const int node_y_offset_at_max_depth = static_cast<int>(
				(0.5 - 1e-6) * max_level_width_in_nodes *
					(1 + inv_circle_centre_z_in_cube_face_coords * circle_centre_y_in_cube_face_coords));

		// We need to make sure the appropriate interior quad tree nodes are generated,
		// if they've not already been, along the way.
		unsigned int num_levels_to_max_depth = d_maximum_depth;
		int prev_node_x_offset = 0;
		int prev_node_y_offset = 0;
		Node *current_node = &d_quad_trees[cube_face].root_node;
		while (num_levels_to_max_depth)
		{
			--num_levels_to_max_depth;

			const int node_x_offset =
					(node_x_offset_at_max_depth >> num_levels_to_max_depth);
			const int node_y_offset =
					(node_y_offset_at_max_depth >> num_levels_to_max_depth);

			const int child_x_offset = node_x_offset - 2 * prev_node_x_offset;
			const int child_y_offset = node_y_offset - 2 * prev_node_y_offset;

			Node *&child_node_ptr = current_node->d_children[child_y_offset][child_x_offset];
			if (child_node_ptr == NULL)
			{
				// Create a new child node.
				child_node_ptr = d_quad_tree_node_pool.construct();
			}
			// Make the child node the current node.
			current_node = child_node_ptr;

			prev_node_x_offset = node_x_offset;
			prev_node_y_offset = node_y_offset;
		}

		// Store the element in a list node that's allocated from a pool and
		// add the element to the quad tree node.
		current_node->add_element(
				d_element_list_node_pool.construct(element));
	}


	template <typename ElementType>
	void
	CubeQuadTreePartition<ElementType>::add(
			const ElementType &element,
			const UnitVector3D &bounding_circle_centre,
			const BoundingCircleExtent &bounding_circle_extent)
	{
		//
		// NOTE: This method needs to be efficient because it is called
		// for all geometries on the globe at each reconstruction time.
		// So doing things like hand-coding 'std::fabs' make a difference.
		//

		// Get the nearest cube face to project circle centre onto.
		// Also get the cube face coord system from which to index into its quad tree.
		double circle_centre_x_in_cube_face_coords;
		double circle_centre_y_in_cube_face_coords;
		double circle_centre_z_in_cube_face_coords;
		const unsigned int cube_face =
				get_cube_face(
						bounding_circle_centre,
						circle_centre_x_in_cube_face_coords,
						circle_centre_y_in_cube_face_coords,
						circle_centre_z_in_cube_face_coords);

		//
		// Project the bounding circle centre vector onto the cube face.
		//

		// See if the bounding circle is larger than a hemi-sphere.
		// If it is then it's too big to project onto the cube face.
		if (bounding_circle_extent.cosine_extend_angle < 1e-4)
		{
			// Store the element in a list node that's allocated from a pool and
			// add the element to the list of elements that don't belong to any quad tree.
			d_element_list.push_front(
					d_element_list_node_pool.construct(element));
			return;
		}

		const double sin_e =
				std::sqrt(1+1e-12f - circle_centre_z_in_cube_face_coords * circle_centre_z_in_cube_face_coords);
		const double sin_a = bounding_circle_extent.sine_extend_angle;

		const double sin_e_sin_a = sin_e * sin_a;
		const double cos_e_cos_a =
				circle_centre_z_in_cube_face_coords * bounding_circle_extent.cosine_extend_angle;

		// See if we can even calculate the maximum projected radius on the cube face.
		// If we can't then it means the bounding circle has a position and extent that
		// cannot be projected onto the cube face (ie, it wraps around the globe enough away
		// from the cube face enough that projected into onto the cube face is no longer well-defined).
		if (cos_e_cos_a < sin_e_sin_a + 1e-6)
		{
			// Store the element in a list node that's allocated from a pool and
			// add the element to the list of elements that don't belong to any quad tree.
			d_element_list.push_front(
					d_element_list_node_pool.construct(element));
			return;
		}

		const double max_projected_radius_on_cube_face =
				sin_a / (circle_centre_z_in_cube_face_coords * (cos_e_cos_a - sin_e_sin_a));

		// The half-width of a quad tree node.
		// The root node is a whole cube face which has a half-width of 1.0 for a unit sphere.
		// Subtract a little bit to give a bit of padding to the bounds for numerical tolerance.
		double quad_tree_node_half_width = 1 - 1e-6;

		// If the max projected radius is larger than half the width of the cube face (ie 1.0)
		// then it is too large to fit within the 'loose' bounding square of the
		// root quad tree node of the cube face.
		if (max_projected_radius_on_cube_face > quad_tree_node_half_width)
		{
			// Store the element in a list node that's allocated from a pool and
			// add the element to the list of elements that don't belong to any quad tree.
			d_element_list.push_front(
					d_element_list_node_pool.construct(element));
			return;
		}

		const double inv_circle_centre_z_in_cube_face_coords =
				1.0 / circle_centre_z_in_cube_face_coords;

		// Calculate the x,y offsets of the quad tree node position as if it was
		// in the deepest level.
		// Use a small numerical tolerance to ensure the we keep the offset within range.
		const unsigned int max_level_width_in_nodes = (1 << d_maximum_depth);
		//
		// Note: Using static_cast<int> instead of static_cast<unsigned int> since
		// Visual Studio optimises for 'int' and not 'unsigned int'.
		//
		const int node_x_offset_at_max_depth = static_cast<int>(
				(0.5 - 1e-6) * max_level_width_in_nodes *
					(1 + inv_circle_centre_z_in_cube_face_coords * circle_centre_x_in_cube_face_coords));
		const int node_y_offset_at_max_depth = static_cast<int>(
				(0.5 - 1e-6) * max_level_width_in_nodes *
					(1 + inv_circle_centre_z_in_cube_face_coords * circle_centre_y_in_cube_face_coords));

		// Using the max projected radius (onto cube face) determine the level at which
		// to store in the cube face's quad tree.
		// Also generate the interior quad tree nodes as required along the way.
		unsigned int num_levels_to_max_depth = d_maximum_depth;
		int prev_node_x_offset = 0;
		int prev_node_y_offset = 0;
		Node *current_node = &d_quad_trees[cube_face].root_node;
		quad_tree_node_half_width *= 0.5;
		while (num_levels_to_max_depth &&
			max_projected_radius_on_cube_face < quad_tree_node_half_width)
		{
			--num_levels_to_max_depth;

			const int node_x_offset =
					(node_x_offset_at_max_depth >> num_levels_to_max_depth);
			const int node_y_offset =
					(node_y_offset_at_max_depth >> num_levels_to_max_depth);

			const int child_x_offset = node_x_offset - 2 * prev_node_x_offset;
			const int child_y_offset = node_y_offset - 2 * prev_node_y_offset;

			Node *&child_node_ptr = current_node->d_children[child_y_offset][child_x_offset];
			if (child_node_ptr == NULL)
			{
				// Create a new child node.
				child_node_ptr = d_quad_tree_node_pool.construct();
			}
			// Make the child node the current node.
			current_node = child_node_ptr;

			prev_node_x_offset = node_x_offset;
			prev_node_y_offset = node_y_offset;
			quad_tree_node_half_width *= 0.5;
		}

		// Store the element in a list node that's allocated from a pool and
		// add the element to the quad tree node.
		current_node->add_element(
				d_element_list_node_pool.construct(element));
	}
}

#endif // GPLATES_MATHS_CUBEQUADTREEPARTITION_H
