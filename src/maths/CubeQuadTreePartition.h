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
#include <iterator>
#include <vector>
#include <boost/mpl/if.hpp>
#include <boost/operators.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include "AngularExtent.h"
#include "ConstGeometryOnSphereVisitor.h"
#include "CubeCoordinateFrame.h"
#include "CubeQuadTree.h"
#include "CubeQuadTreeLocation.h"
#include "FiniteRotation.h"
#include "GeometryOnSphere.h"
#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "SmallCircleBounds.h"
#include "UnitVector3D.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/IntrusiveSinglyLinkedList.h"
#include "utils/Profile.h"
#include "utils/ReferenceCount.h"
#include "utils/SafeBool.h"


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
	 * the circle-centre within that level is determined by the centre vector of the bounding circle.
	 *
	 * Also if the bounds are exactly *twice* the size then we can determine the level or depth
	 * at which to insert an element in O(1) time (using a log2 on the element's bounding radius).
	 * In practise it ends up being faster and easier (for reasonable depths, eg, up to 8) to
	 * implement this as a loop since we need to check that interior nodes along the path have
	 * been created (this is because we don't fill the entire cube quad tree with empty nodes).
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
			public GPlatesUtils::ReferenceCount< CubeQuadTreePartition<ElementType> >
	{
	private:
		/**
		 * Linked list wrapper node around an element that has been added to a quad tree node.
		 */
		class ElementListNode :
				public GPlatesUtils::IntrusiveSinglyLinkedList<ElementListNode>::Node
		{
		public:
			explicit
			ElementListNode(
					const ElementType &element) :
				d_element(element)
			{  }

			ElementType &
			get_element()
			{
				return d_element;
			}

			const ElementType &
			get_element() const
			{
				return d_element;
			}

		private:
			ElementType d_element;
		};

		//! Typedef for the internal list of elements.
		typedef typename GPlatesUtils::IntrusiveSinglyLinkedList<ElementListNode> element_list_impl_type;

		/**
		 * A list of elements that belong to a single node in a quad tree.
		 */
		class ElementList
		{
		public:
			/**
			 * Typedef for a const iterator of the elements in the internal list.
			 *
			 * NOTE: Use as 'iter->get_element()' to get a reference to 'ElementType'.
			 */
			typedef typename element_list_impl_type::const_iterator const_iterator;

			//! Typedef for a non-const iterator of the elements in the internal list.
			typedef typename element_list_impl_type::iterator iterator;


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

			/**
			 * Returns true if this node has no objects of type 'ElementType' in it.
			 */
			bool
			empty() const
			{
				return d_element_list.empty();
			}

			/**
			 * Begin iterator over the elements (of type 'ElementType' in this list).
			 *
			 * NOTE: Use as 'iter->get_element()' to get a reference to 'ElementType'.
			 */
			const_iterator
			begin() const
			{
				return d_element_list.begin();
			}

			/**
			 * End iterator over the elements (of type 'ElementType' in this list).
			 */
			const_iterator
			end() const
			{
				return d_element_list.end();
			}

			//! Begin non-const iterator over the elements (of type 'ElementType' in this list).
			iterator
			begin()
			{
				return d_element_list.begin();
			}

			//! End non-const iterator over the elements (of type 'ElementType' in this list).
			iterator
			end()
			{
				return d_element_list.end();
			}

		private:
			//! Any elements added to this quad tree node.
			element_list_impl_type d_element_list;
		};

		/**
		 * Typedef for the 'loose' cube quad tree with nodes containing the type @a ElementList.
		 *
		 * It is 'loose' in that the spatial extent of each element in a quad tree node is not
		 * necessarily limited to the spatial extent of the quad tree node.
		 *
		 * Implementation detail: Note that we never remove nodes from the quad tree so we don't
		 * need to worry about the object pool clear policy (no nodes are returned to the pool).
		 */
		typedef CubeQuadTree<ElementList> cube_quad_tree_type;

		//! Typedef for a node of the cube quad tree.
		typedef typename cube_quad_tree_type::node_type cube_quad_tree_node_type;

	public:
		//! Typedef for the element type.
		typedef ElementType element_type;

		//! Typedef for this class type.
		typedef CubeQuadTreePartition<ElementType> this_type;

		//! A convenience typedef for a shared pointer to a non-const @a CubeQuadTreePartition.
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a CubeQuadTreePartition.
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;


		/**
		 * Iterator over the elements in cube quad tree node.
		 *
		 * 'ElementQualifiedType' can be either 'element_type' or 'const element_type'.
		 *
		 * This is similar to, in fact a wrapper around, the iterator in @a ElementList but
		 * is easier to use for clients since it dereferences directly to 'element_type'.
		 */
		template <class ElementQualifiedType>
		class ElementIterator :
				public std::iterator<std::forward_iterator_tag, ElementQualifiedType>,
				public boost::forward_iteratable<ElementIterator<ElementQualifiedType>, ElementQualifiedType *>
		{
		public:
			//! Typedef for the element list iterator.
			typedef typename boost::mpl::if_<
					boost::is_const<typename boost::remove_reference<ElementQualifiedType>::type >,
								typename ElementList::const_iterator,
								typename ElementList::iterator>::type
										element_list_iterator_type;

			explicit
			ElementIterator(
					element_list_iterator_type element_list_iterator) :
				d_element_list_iterator(element_list_iterator)
			{  }

			/**
			 * Implicit conversion constructor from 'iterator' to 'const_iterator'.
			 */
			ElementIterator(
					const ElementIterator<element_type> &rhs) :
				d_element_list_iterator(rhs.d_element_list_iterator)
			{  }

			//! 'operator->()' provided by base class boost::forward_iteratable.
			ElementQualifiedType &
			operator*() const
			{
				return d_element_list_iterator->get_element();
			}

			//! Post-increment operator provided by base class boost::forward_iteratable.
			ElementIterator &
			operator++()
			{
				++d_element_list_iterator;
				return *this;
			}

			//! Inequality operator provided by base class boost::forward_iteratable.
			friend
			bool
			operator==(
					const ElementIterator &lhs,
					const ElementIterator &rhs)
			{
				return lhs.d_element_list_iterator == rhs.d_element_list_iterator;
			}

		private:
			element_list_iterator_type d_element_list_iterator;

			friend class ElementIterator<typename boost::add_const<element_type>::type>; // The const iterator.
		};

		//! Typedef for element const iterator.
		typedef ElementIterator<const element_type> element_const_iterator;

		//! Typedef for element non-const iterator.
		typedef ElementIterator<element_type> element_iterator;



		/**
		 * A reference, or handle, to a node of this spatial partition.
		 *
		 * The size is equivalent to a pointer making it cheap to copy.
		 *
		 * 'NodeImplQualifiedType' can be either 'cube_quad_tree_node_type' or
		 * 'const cube_quad_tree_node_type'.
		 */
		template <class NodeImplQualifiedType>
		class NodeReference :
				public GPlatesUtils::SafeBool< NodeReference<NodeImplQualifiedType> >
		{
		public:
			//! Typedef for the element iterator.
			typedef typename boost::mpl::if_<
					boost::is_const<NodeImplQualifiedType>,
								element_const_iterator,
								element_iterator>::type
										element_iterator_type;

			//! Default constructor initialises to NULL reference.
			NodeReference() :
				d_node_impl(NULL)
			{  }

			//! Implicit conversion constructor for converting non-const to const.
			NodeReference(
					const typename CubeQuadTreePartition<element_type>::template NodeReference<cube_quad_tree_node_type> &rhs) :
				GPlatesUtils::SafeBool< NodeReference<NodeImplQualifiedType> >(rhs),
				d_node_impl(rhs.d_node_impl)
			{  }

			/**
			 * Use "if (ref)" or "if (!ref)" to effect this boolean test.
			 *
			 * Used by GPlatesUtils::SafeBool<>
			 */
			bool
			boolean_test() const
			{
				return d_node_impl != NULL;
			}

			//! Returns true if this node has no objects of type 'ElementType' in it.
			bool
			empty() const
			{
				return d_node_impl->get_element().empty();
			}

			//! Returns begin iterator for elements in this node.
			element_iterator_type
			begin() const
			{
				return element_iterator_type(d_node_impl->get_element().begin());
			}

			//! Returns end iterator for elements in this node.
			element_iterator_type
			end() const
			{
				return element_iterator_type(d_node_impl->get_element().end());
			}

			/**
			 * Returns the specified child node if it exists.
			 *
			 * NOTE: Be sure to check the returned reference with operator! before using.
			 */
			NodeReference
			get_child_node(
					unsigned int child_x_offset,
					unsigned int child_y_offset) const
			{
				return NodeReference(d_node_impl->get_child_node(child_x_offset, child_y_offset));
			}

		private:
			NodeImplQualifiedType *d_node_impl;

			explicit
			NodeReference(
					NodeImplQualifiedType *node_impl) :
				d_node_impl(node_impl)
			{  }

			// Make friend so can construct.
			friend class CubeQuadTreePartition<element_type>;
		};

		//! Typedef for a non-const reference to a node of this spatial partition.
		typedef NodeReference<cube_quad_tree_node_type> node_reference_type;

		//! Typedef for a const reference to a node of this spatial partition.
		typedef NodeReference<const cube_quad_tree_node_type> const_node_reference_type;


		//! Typedef for a location in the cube quad tree.
		typedef CubeQuadTreeLocation location_type;


		/**
		 * Iterator over the spatial partition.
		 *
		 * 'ElementQualifiedType' can be either 'element_type' or 'const element_type'.
		 */
		template <class ElementQualifiedType>
		class Iterator
		{
		public:
			//! Typedef for cube quad tree of appropriate const-ness.
			typedef typename GPlatesUtils::CopyConst<
					ElementQualifiedType, cube_quad_tree_type>::type
							cube_quad_tree_qualified_type;


			explicit
			Iterator(
					cube_quad_tree_qualified_type &cube_quad_tree);

			/**
			 * Implicit conversion constructor from 'iterator' to 'const_iterator'.
			 */
			Iterator(
					const Iterator<element_type> &rhs);

			/**
			 * Reset to the beginning of the sequence - only necessary if want to iterate
			 * over sequence *again* with same iterator - not needed on first iteration.
			 */
			void
			reset();

			/**
			 * Return type is 'element_type &'/'const element_type &' for 'iterator'/'const_iterator'.
			 */
			ElementQualifiedType &
			get_element() const;

			/**
			 * Returns the CubeQuadTreeLocation of the current element (returned by @a get_element).
			 */
			const location_type &
			get_location() const;

			void
			next();

			bool
			finished() const;

		private:
			//! Typedef for ElementList of appropriate const-ness.
			typedef typename GPlatesUtils::CopyConst<ElementQualifiedType, ElementList>::type
					element_list_qualified_type;

			//! Typedef for cube quad tree iterator.
			typedef typename cube_quad_tree_qualified_type::template Iterator<element_list_qualified_type>
					cube_quad_tree_iterator_type;

			cube_quad_tree_iterator_type d_cube_quad_tree_iterator;
			typename ElementList::const_iterator d_current_element_list_iterator;
			typename ElementList::const_iterator d_current_element_list_end;
			bool d_finished;

			void
			first();

			friend class Iterator<const element_type>; // The const iterator.
		};

		//! Typedef for iterator.
		typedef Iterator<element_type> iterator;

		//! Typedef for const iterator.
		typedef Iterator<const element_type> const_iterator;


		/**
		 * Creates a @a CubeQuadTreePartition object.
		 *
		 * @param maximum_quad_tree_depth is the deepest level that an element can be added to.
		 *        The maximum amount of memory required for the nodes themselves (assuming all
		 *        nodes of all levels of all quad trees contain elements) is
		 *        '6 * pow(4, maximum_quad_tree_depth) * 1.3 * 20' bytes - the 6 is for the six cube faces,
		 *        the 1.3 is summation over the levels and the last number is the size of a
		 *        quad tree node in bytes (on 32-bit systems).
		 *        This does not include the memory used by the elements themselves.
		 *
		 * NOTE: @a maximum_quad_tree_depth only applies to those elements added with geometry
		 * since the depth at which they are inserted depends on the spatial extent of the geometry.
		 * For top-down addition (such as mirroring another spatial partition) it is possible to
		 * go deeper than the maximum depth.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int maximum_quad_tree_depth)
		{
			return non_null_ptr_type(new CubeQuadTreePartition(maximum_quad_tree_depth));
		}


		//
		// Methods to query the spatial partition.
		//


		/**
		 * Returns the maximum depth of this spatial partition (see @a create).
		 */
		unsigned int
		get_maximum_quad_tree_depth() const
		{
			return d_maximum_quad_tree_depth;
		}


		/**
		 * Returns true if any elements have been added to this spatial partition.
		 */
		bool
		empty() const
		{
			return d_num_elements == 0;
		}


		/**
		 * Returns the number of elements that have been added to this spatial partition so far.
		 */
		unsigned int
		size() const
		{
			return d_num_elements;
		}


		/**
		 * Returns the begin iterator for elements in the root of the spatial partition.
		 *
		 * These are the elements added via the @a add overload accepting a single
		 * argument (the element).
		 */
		element_const_iterator
		begin_root_elements() const
		{
			const ElementList *element_list = d_cube_quad_tree->get_root_element();
			return element_list
					? element_const_iterator(element_list->begin())
					: element_const_iterator(d_dummy_empty_element_list_impl.begin());
		}


		/**
		 * Returns the end iterator for elements in the root of the spatial partition.
		 *
		 * These are the elements added via the @a add overload accepting a single
		 * argument (the element).
		 */
		element_const_iterator
		end_root_elements() const
		{
			const ElementList *element_list = d_cube_quad_tree->get_root_element();
			return element_list
					? element_const_iterator(element_list->end())
					: element_const_iterator(d_dummy_empty_element_list_impl.end());
		}


		/**
		 * Returns the non-const begin iterator for elements in the root of the spatial partition.
		 */
		element_iterator
		begin_root_elements()
		{
			ElementList *element_list = d_cube_quad_tree->get_root_element();
			return element_list
					? element_iterator(element_list->begin())
					: element_iterator(d_dummy_empty_element_list_impl.begin());
		}


		/**
		 * Returns the non-const end iterator for elements in the root of the spatial partition.
		 */
		element_iterator
		end_root_elements()
		{
			ElementList *element_list = d_cube_quad_tree->get_root_element();
			return element_list
					? element_iterator(element_list->end())
					: element_iterator(d_dummy_empty_element_list_impl.end());
		}


		/**
		 * Gets the root node of the specified cube face (quad tree), if it exists.
		 *
		 * NOTE: Be sure to check the returned reference with operator! before using.
		 */
		const_node_reference_type
		get_quad_tree_root_node(
				CubeCoordinateFrame::CubeFaceType cube_face) const
		{
			return const_node_reference_type(d_cube_quad_tree->get_quad_tree_root_node(cube_face));
		}

		/**
		 * Gets the non-const root node of the specified cube face (quad tree), if it exists.
		 */
		node_reference_type
		get_quad_tree_root_node(
				CubeCoordinateFrame::CubeFaceType cube_face)
		{
			return node_reference_type(d_cube_quad_tree->get_quad_tree_root_node(cube_face));
		}


		/**
		 * Gets the child node of the specified parent node, if it exists.
		 *
		 * NOTE: Be sure to check the returned reference with operator! before using.
		 */
		const_node_reference_type
		get_child_node(
				const_node_reference_type parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset) const
		{
			return const_node_reference_type(parent_node.get_child_node(child_x_offset, child_y_offset));
		}

		/**
		 * Gets the non-const child node of the specified parent node, if it exists.
		 */
		node_reference_type
		get_child_node(
				node_reference_type parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset)
		{
			return node_reference_type(parent_node.get_child_node(child_x_offset, child_y_offset));
		}


		/**
		 * Returns a non-const iterator over the elements of this spatial partition.
		 *
		 * This is a convenience for when you don't care about the order of iteration but
		 * just want to iterate over all elements in the spatial partition.
		 */
		iterator
		get_iterator()
		{
			return iterator(*d_cube_quad_tree);
		}


		/**
		 * Returns a const iterator over the elements of this spatial partition.
		 *
		 * This is a convenience for when you don't care about the order of iteration but
		 * just want to iterate over all elements in the spatial partition.
		 */
		const_iterator
		get_iterator() const
		{
			return const_iterator(*d_cube_quad_tree);
		}


		//
		// Methods to modify the spatial partition.
		//


		/**
		 * Clears the entire spatial partition.
		 */
		void
		clear()
		{
			// Clear the cube quad tree.
			d_cube_quad_tree->clear();

			// Destroying boost::object_pool and recreating is O(N).
			// Destroying each object in boost::object_pool is O(N^2).
			// So much better to destroy and recreate entire object pool.
			d_element_list_node_pool.reset(); // Release memory first.
			d_element_list_node_pool.reset(new element_list_node_pool_type());

			d_num_elements = 0;
		}


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
				const UnitVector3D &point_geometry,
				location_type *location_added = NULL);

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* point geometry.
		 */
		void
		add(
				const ElementType &element,
				const UnitVector3D &point_geometry,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			add(element, finite_rotation * point_geometry, location_added);
		}


		/**
		 * Add an element, to the spatial partition, that has a finite spatial extent.
		 *
		 * The location at which the element is added in the spatial partition
		 * is determined by the small circle bounding the element's geometry - determined by
		 * @a bounding_circle_centre and @a bounding_circle_extent.
		 */
		void
		add(
				const ElementType &element,
				const UnitVector3D &bounding_circle_centre,
				const AngularExtent &bounding_circle_extent,
				location_type *location_added = NULL);

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* bounding circle centre.
		 *
		 * This is efficient if you already have a bounding circle (for a geometry) since it
		 * avoids the need to rotate the geometry and calculate a new bounding circle.
		 */
		void
		add(
				const ElementType &element,
				const UnitVector3D &bounding_circle_centre,
				const AngularExtent &bounding_circle_extent,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			// Rotate only the bounding circle centre to avoid rotating the entire geometry.
			add(element, finite_rotation * bounding_circle_centre, bounding_circle_extent, location_added);
		}


		/**
		 * Add an element, to the spatial partition, using the spatial extent of the
		 * specified GeometryOnSphere object.
		 *
		 * This is a convenient wrapper around the @a add overload accepting a bounding circle.
		 */
		void
		add(
				const ElementType &element,
				const GeometryOnSphere &geometry,
				location_type *location_added = NULL)
		{
			AddGeometryOnSphere add_geometry(*this, element, location_added);
			geometry.accept_visitor(add_geometry);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* geometry.
		 *
		 * This is implemented to be more efficient than actually rotating the geometry and
		 * then inserting that.
		 */
		void
		add(
				const ElementType &element,
				const GeometryOnSphere &geometry,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			// Rotate only the geometry's bounding circle centre to avoid rotating the entire geometry.
			AddRotatedGeometryOnSphere add_geometry(*this, element, finite_rotation, location_added);
			geometry.accept_visitor(add_geometry);
		}

		/**
		 * Add an element, to the spatial partition, using the *expanded* (by region-of-interest)
		 * spatial extent of the specified GeometryOnSphere object.
		 *
		 * This is a convenient wrapper around the @a add overload accepting a bounding circle.
		 */
		void
		add(
				const ElementType &element,
				const GeometryOnSphere &geometry,
				const AngularExtent &region_of_interest,
				location_type *location_added = NULL)
		{
			AddRegionOfInterestGeometryOnSphere add_geometry(*this, element, region_of_interest, location_added);
			geometry.accept_visitor(add_geometry);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* geometry.
		 *
		 * This is implemented to be more efficient than actually rotating the geometry and
		 * then inserting that.
		 */
		void
		add(
				const ElementType &element,
				const GeometryOnSphere &geometry,
				const AngularExtent &region_of_interest,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			// Rotate only the geometry's bounding circle centre to avoid rotating the entire geometry.
			AddRegionOfInterestRotatedGeometryOnSphere add_geometry(*this, element, finite_rotation, region_of_interest, location_added);
			geometry.accept_visitor(add_geometry);
		}


		/**
		 * Add an element, to the spatial partition, using the specified PointOnSphere.
		 *
		 * This is a convenient wrapper around the @a add overload accepting a point.
		 */
		void
		add(
				const ElementType &element,
				const PointOnSphere &point_on_sphere,
				location_type *location_added = NULL)
		{
			add(element, point_on_sphere.position_vector(), location_added);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* point.
		 */
		void
		add(
				const ElementType &element,
				const PointOnSphere &point_on_sphere,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			add(element, point_on_sphere.position_vector(), finite_rotation, location_added);
		}

		/**
		 * Add an element, to the spatial partition, using the specified PointOnSphere but also using
		 * the finite bounding extent specified by @a region_of_interest (instead of a point insertion).
		 *
		 * This is a convenient wrapper around the @a add overload accepting a point.
		 */
		void
		add(
				const ElementType &element,
				const PointOnSphere &point_on_sphere,
				const AngularExtent &region_of_interest,
				location_type *location_added = NULL)
		{
			add(element, point_on_sphere.position_vector(), region_of_interest, location_added);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* point.
		 */
		void
		add(
				const ElementType &element,
				const PointOnSphere &point_on_sphere,
				const AngularExtent &region_of_interest,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			add(element, point_on_sphere.position_vector(), finite_rotation, region_of_interest, location_added);
		}


		/**
		 * Add an element, to the spatial partition, using the spatial extent of the
		 * specified MultiPointOnSphere object.
		 *
		 * This is a convenient wrapper around the @a add overload accepting a bounding circle.
		 */
		void
		add(
				const ElementType &element,
				const MultiPointOnSphere &multi_point_on_sphere,
				location_type *location_added = NULL)
		{
			const BoundingSmallCircle &bounding_small_circle = multi_point_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent(),
					location_added);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* geometry.
		 *
		 * This is implemented to be more efficient than actually rotating the geometry and
		 * then inserting that.
		 */
		void
		add(
				const ElementType &element,
				const MultiPointOnSphere &multi_point_on_sphere,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			// Rotate only the geometry's bounding circle centre to avoid rotating the entire geometry.
			const BoundingSmallCircle &bounding_small_circle = multi_point_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent(),
					finite_rotation,
					location_added);
		}

		/**
		 * Add an element, to the spatial partition, using the *expanded* (by region-of-interest)
		 * spatial extent of the specified MultiPointOnSphere object.
		 *
		 * This is a convenient wrapper around the @a add overload accepting a bounding circle.
		 */
		void
		add(
				const ElementType &element,
				const MultiPointOnSphere &multi_point_on_sphere,
				const AngularExtent &region_of_interest,
				location_type *location_added = NULL)
		{
			const BoundingSmallCircle &bounding_small_circle = multi_point_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent() + region_of_interest,
					location_added);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* geometry.
		 *
		 * This is implemented to be more efficient than actually rotating the geometry and
		 * then inserting that.
		 */
		void
		add(
				const ElementType &element,
				const MultiPointOnSphere &multi_point_on_sphere,
				const AngularExtent &region_of_interest,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			// Rotate only the geometry's bounding circle centre to avoid rotating the entire geometry.
			const BoundingSmallCircle &bounding_small_circle = multi_point_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent() + region_of_interest,
					finite_rotation,
					location_added);
		}


		/**
		 * Add an element, to the spatial partition, using the spatial extent of the
		 * specified PolylineOnSphere object.
		 *
		 * This is a convenient wrapper around the @a add overload accepting a bounding circle.
		 */
		void
		add(
				const ElementType &element,
				const PolylineOnSphere &polyline_on_sphere,
				location_type *location_added = NULL)
		{
			const BoundingSmallCircle &bounding_small_circle = polyline_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent(),
					location_added);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* geometry.
		 *
		 * This is implemented to be more efficient than actually rotating the geometry and
		 * then inserting that.
		 */
		void
		add(
				const ElementType &element,
				const PolylineOnSphere &polyline_on_sphere,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			// Rotate only the geometry's bounding circle centre to avoid rotating the entire geometry.
			const BoundingSmallCircle &bounding_small_circle = polyline_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent(),
					finite_rotation,
					location_added);
		}

		/**
		 * Add an element, to the spatial partition, using the *expanded* (by region-of-interest)
		 * spatial extent of the specified PolylineOnSphere object.
		 *
		 * This is a convenient wrapper around the @a add overload accepting a bounding circle.
		 */
		void
		add(
				const ElementType &element,
				const PolylineOnSphere &polyline_on_sphere,
				const AngularExtent &region_of_interest,
				location_type *location_added = NULL)
		{
			const BoundingSmallCircle &bounding_small_circle = polyline_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent() + region_of_interest,
					location_added);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* geometry.
		 *
		 * This is implemented to be more efficient than actually rotating the geometry and
		 * then inserting that.
		 */
		void
		add(
				const ElementType &element,
				const PolylineOnSphere &polyline_on_sphere,
				const AngularExtent &region_of_interest,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			// Rotate only the geometry's bounding circle centre to avoid rotating the entire geometry.
			const BoundingSmallCircle &bounding_small_circle = polyline_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent() + region_of_interest,
					finite_rotation,
					location_added);
		}


		/**
		 * Add an element, to the spatial partition, using the spatial extent of the
		 * specified PolygonOnSphere object.
		 *
		 * This is a convenient wrapper around the @a add overload accepting a bounding circle.
		 */
		void
		add(
				const ElementType &element,
				const PolygonOnSphere &polygon_on_sphere,
				location_type *location_added = NULL)
		{
			const BoundingSmallCircle &bounding_small_circle = polygon_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent(),
					location_added);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* geometry.
		 *
		 * This is implemented to be more efficient than actually rotating the geometry and
		 * then inserting that.
		 */
		void
		add(
				const ElementType &element,
				const PolygonOnSphere &polygon_on_sphere,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			// Rotate only the geometry's bounding circle centre to avoid rotating the entire geometry.
			const BoundingSmallCircle &bounding_small_circle = polygon_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent(),
					finite_rotation,
					location_added);
		}

		/**
		 * Add an element, to the spatial partition, using the *expanded* (by region-of-intereset)
		 * spatial extent of the specified PolygonOnSphere object.
		 *
		 * This is a convenient wrapper around the @a add overload accepting a bounding circle.
		 */
		void
		add(
				const ElementType &element,
				const PolygonOnSphere &polygon_on_sphere,
				const AngularExtent &region_of_interest,
				location_type *location_added = NULL)
		{
			const BoundingSmallCircle &bounding_small_circle = polygon_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent() + region_of_interest,
					location_added);
		}

		/**
		 * Same as the above overload of @a add but location of insertion is the *rotated* geometry.
		 *
		 * This is implemented to be more efficient than actually rotating the geometry and
		 * then inserting that.
		 */
		void
		add(
				const ElementType &element,
				const PolygonOnSphere &polygon_on_sphere,
				const AngularExtent &region_of_interest,
				const FiniteRotation &finite_rotation,
				location_type *location_added = NULL)
		{
			// Rotate only the geometry's bounding circle centre to avoid rotating the entire geometry.
			const BoundingSmallCircle &bounding_small_circle = polygon_on_sphere.get_bounding_small_circle();
			add(
					element,
					bounding_small_circle.get_centre(),
					bounding_small_circle.get_angular_extent() + region_of_interest,
					finite_rotation,
					location_added);
		}


		/**
		 * Add an element, to the spatial partition, at the location specified.
		 *
		 * @a location_added can be different than @a location if the latter is the location
		 * in another spatial partition and it is deeper than the maximum depth of this spatial
		 * partition. In this case it'll be added at the maximum depth of this spatial partition.
		 */
		void
		add(
				const ElementType &element,
				const location_type &location,
				location_type *location_added = NULL);


		//
		// Methods for building the cube quad tree in a top-down manner.
		//


		/**
		 * Add an element, to the spatial partition, at the root of the entire cube quad tree.
		 *
		 * Since no spatial information is provided, the location in the cube quad tree cannot
		 * be determined and hence the element must be added to the root of the cube quad tree.
		 *
		 * This is only useful if you know the element's corresponding spatial extents are
		 * very large (ie, larger than a cube face) or you don't know the spatial extents but
		 * still want to add an element to the cube quad tree. In the latter case it just means
		 * the efficiency of the spatial partition will not be that good since elements are
		 * added at the root of the partition.
		 */
		void
		add_unpartitioned(
				const ElementType &element,
				location_type *location_added = NULL)
		{
			add(element, d_cube_quad_tree->get_or_create_root_element());

			if (location_added)
			{
				// Added to the root of the cube (not in any quad trees).
				*location_added = location_type();
			}
		}


		/**
		 * Gets, or creates if does not exist, the root node of the specified cube face (quad tree).
		 *
		 * NOTE: The returned reference is guaranteed to be valid - you do *not* need to check it
		 * with operator! before using.
		 *
		 * You can then add elements to the node using the @a add overload accepting
		 * a @a node_reference_type.
		 */
		node_reference_type
		get_or_create_quad_tree_root_node(
				CubeCoordinateFrame::CubeFaceType cube_face)
		{
			return node_reference_type(&d_cube_quad_tree->get_or_create_quad_tree_root_node(cube_face));
		}


		/**
		 * Gets, or creates if does not exist, the child node of the specified parent node.
		 *
		 * NOTE: The returned reference is guaranteed to be valid - you do *not* need to check it
		 * with operator! before using.
		 *
		 * You can then add elements to the node using the @a add overload accepting
		 * a @a node_reference_type.
		 */
		node_reference_type
		get_or_create_child_node(
				node_reference_type parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset)
		{
			return node_reference_type(
					&d_cube_quad_tree->get_or_create_child_node(
							*parent_node.d_node_impl, child_x_offset, child_y_offset));
		}


		/**
		 * Add an element, to the spatial partition, at the node location specified.
		 * 
		 * @param cube_quad_tree_node a cube quad tree node obtained from
		 *        @a get_or_create_quad_tree_root_node or @a get_or_create_child_node.
		 *
		 * This is useful when traversing an existing spatial partition and mirroring it
		 * into a new spatial partition - it's a cheaper way to add elements since the location
		 * in the quad tree does not need to be determined (it's already been determined by the
		 * spatial partition being mirrored and implicit in @a cube_quad_tree_node).
		 *
		 * NOTE: @a loose_cube_quad_tree_node must have been obtained from this spatial partition,
		 * otherwise undefined program behaviour will result.
		 */
		void
		add(
				const ElementType &element,
				node_reference_type cube_quad_tree_node)
		{
			add(element, cube_quad_tree_node.d_node_impl->get_element());
		}

	private:
		/**
		 * Typedef for an object pool for type @a ElementListNode.
		 *
		 * Note that, as with @a cube_quad_tree_type, we never return any element list nodes
		 * back to the pool (which is why we're using boost::object_pool instead
		 * of GPlatesUtils::ObjectPool).
		 */
		typedef boost::object_pool<ElementListNode> element_list_node_pool_type;


		/**
		 * Add @a GeometryOnSphere derived objects to this spatial partition.
		 */
		struct AddGeometryOnSphere :
				public ConstGeometryOnSphereVisitor
		{
			AddGeometryOnSphere(
					CubeQuadTreePartition<ElementType> &spatial_partition,
					const ElementType &element,
					location_type *location_added) :
				d_spatial_partition(spatial_partition),
				d_element(element),
				d_location_added(location_added)
			{  }

			virtual
			void
			visit_multi_point_on_sphere(
					MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_spatial_partition.add(d_element, *multi_point_on_sphere, d_location_added);
			}

			virtual
			void
			visit_point_on_sphere(
					PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{
				d_spatial_partition.add(d_element, point_on_sphere->position_vector(), d_location_added);
			}

			virtual
			void
			visit_polygon_on_sphere(
					PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_spatial_partition.add(d_element, *polygon_on_sphere, d_location_added);
			}

			virtual
			void
			visit_polyline_on_sphere(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_spatial_partition.add(d_element, *polyline_on_sphere, d_location_added);
			}

			CubeQuadTreePartition<ElementType> &d_spatial_partition;
			const ElementType &d_element;
			location_type *d_location_added;
		};


		/**
		 * Add @a GeometryOnSphere derived objects to this spatial partition in at
		 * their *rotated* locations.
		 */
		struct AddRotatedGeometryOnSphere :
				public ConstGeometryOnSphereVisitor
		{
			AddRotatedGeometryOnSphere(
					CubeQuadTreePartition<ElementType> &spatial_partition,
					const ElementType &element,
					const FiniteRotation &finite_rotation,
					location_type *location_added) :
				d_spatial_partition(spatial_partition),
				d_element(element),
				d_finite_rotation(finite_rotation),
				d_location_added(location_added)
			{  }

			virtual
			void
			visit_multi_point_on_sphere(
					MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_spatial_partition.add(d_element, *multi_point_on_sphere, d_finite_rotation, d_location_added);
			}

			virtual
			void
			visit_point_on_sphere(
					PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{
				d_spatial_partition.add(d_element, point_on_sphere->position_vector(), d_finite_rotation, d_location_added);
			}

			virtual
			void
			visit_polygon_on_sphere(
					PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_spatial_partition.add(d_element, *polygon_on_sphere, d_finite_rotation, d_location_added);
			}

			virtual
			void
			visit_polyline_on_sphere(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_spatial_partition.add(d_element, *polyline_on_sphere, d_finite_rotation, d_location_added);
			}

			CubeQuadTreePartition<ElementType> &d_spatial_partition;
			const ElementType &d_element;
			const FiniteRotation &d_finite_rotation;
			location_type *d_location_added;
		};


		/**
		 * Add @a GeometryOnSphere derived objects (with extended bounding circles) to this spatial partition.
		 */
		struct AddRegionOfInterestGeometryOnSphere :
				public ConstGeometryOnSphereVisitor
		{
			AddRegionOfInterestGeometryOnSphere(
					CubeQuadTreePartition<ElementType> &spatial_partition,
					const ElementType &element,
					const AngularExtent &region_of_interest,
					location_type *location_added) :
				d_spatial_partition(spatial_partition),
				d_element(element),
				d_region_of_interest(region_of_interest),
				d_location_added(location_added)
			{  }

			virtual
			void
			visit_multi_point_on_sphere(
					MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_spatial_partition.add(d_element, *multi_point_on_sphere, d_region_of_interest, d_location_added);
			}

			virtual
			void
			visit_point_on_sphere(
					PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{
				d_spatial_partition.add(d_element, point_on_sphere->position_vector(), d_region_of_interest, d_location_added);
			}

			virtual
			void
			visit_polygon_on_sphere(
					PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_spatial_partition.add(d_element, *polygon_on_sphere, d_region_of_interest, d_location_added);
			}

			virtual
			void
			visit_polyline_on_sphere(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_spatial_partition.add(d_element, *polyline_on_sphere, d_region_of_interest, d_location_added);
			}

			CubeQuadTreePartition<ElementType> &d_spatial_partition;
			const ElementType &d_element;
			const AngularExtent &d_region_of_interest;
			location_type *d_location_added;
		};


		/**
		 * Add @a GeometryOnSphere derived objects (with extended bounding circles) to this
		 * spatial partition in at their *rotated* locations.
		 */
		struct AddRegionOfInterestRotatedGeometryOnSphere :
				public ConstGeometryOnSphereVisitor
		{
			AddRegionOfInterestRotatedGeometryOnSphere(
					CubeQuadTreePartition<ElementType> &spatial_partition,
					const ElementType &element,
					const FiniteRotation &finite_rotation,
					const AngularExtent &region_of_interest,
					location_type *location_added) :
				d_spatial_partition(spatial_partition),
				d_element(element),
				d_finite_rotation(finite_rotation),
				d_region_of_interest(region_of_interest),
				d_location_added(location_added)
			{  }

			virtual
			void
			visit_multi_point_on_sphere(
					MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_spatial_partition.add(d_element, *multi_point_on_sphere, d_region_of_interest, d_finite_rotation, d_location_added);
			}

			virtual
			void
			visit_point_on_sphere(
					PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{
				d_spatial_partition.add(d_element, point_on_sphere->position_vector(), d_region_of_interest, d_finite_rotation, d_location_added);
			}

			virtual
			void
			visit_polygon_on_sphere(
					PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_spatial_partition.add(d_element, *polygon_on_sphere, d_region_of_interest, d_finite_rotation, d_location_added);
			}

			virtual
			void
			visit_polyline_on_sphere(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_spatial_partition.add(d_element, *polyline_on_sphere, d_region_of_interest, d_finite_rotation, d_location_added);
			}

			CubeQuadTreePartition<ElementType> &d_spatial_partition;
			const ElementType &d_element;
			const FiniteRotation &d_finite_rotation;
			const AngularExtent &d_region_of_interest;
			location_type *d_location_added;
		};


		/**
		 * All element linked list nodes are stored in this pool.
		 *
		 * These are linked list nodes containing the elements added to the spatial partition.
		 *
		 * It's a scoped_ptr so we can clear it simply by destroying it and recreating a new one.
		 */
		boost::scoped_ptr<element_list_node_pool_type> d_element_list_node_pool;

		/**
		 * The cube quad tree.
		 *
		 * This is what the user will traverse once we've built the spatial partition.
		 */
		typename cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;

		//! The maximum depth of any quad tree.
		unsigned int d_maximum_quad_tree_depth;

		//! The number of elements that have been added to this spatial partition.
		unsigned int d_num_elements;

		/**
		 * Used solely for the purpose of returning an empty iteration range when clients
		 * request the root elements but there are none.
		 */
		element_list_impl_type d_dummy_empty_element_list_impl;


		/**
		 * Constructor.
		 */
		explicit
		CubeQuadTreePartition(
				unsigned int maximum_quad_tree_depth) :
			d_element_list_node_pool(new element_list_node_pool_type()),
			d_cube_quad_tree(cube_quad_tree_type::create()),
			d_maximum_quad_tree_depth(maximum_quad_tree_depth),
			d_num_elements(0)
		{  }

		/**
		 * NOTE: All adds should go through here to keep track of whether the spatial partition
		 * is empty or not.
		 */
		void
		add(
				const ElementType &element,
				ElementList &element_list)
		{
			// Store the element in a list node that's allocated from a pool and
			// add the element to the element list.
			element_list.add_element(d_element_list_node_pool->construct(element));

			++d_num_elements;
		}
	};


	template <typename ElementType>
	void
	CubeQuadTreePartition<ElementType>::add(
			const ElementType &element,
			const UnitVector3D &point_geometry,
			location_type *location_added)
	{
		// Get the nearest cube face to project point geometry onto.
		// Also get the cube face coord system from which to index into its quad tree.
		double circle_centre_x_in_cube_face_coords;
		double circle_centre_y_in_cube_face_coords;
		double circle_centre_z_in_cube_face_coords;
		const CubeCoordinateFrame::CubeFaceType cube_face =
				CubeCoordinateFrame::get_cube_face_and_transformed_position(
						point_geometry,
						circle_centre_x_in_cube_face_coords,
						circle_centre_y_in_cube_face_coords,
						circle_centre_z_in_cube_face_coords);

		// Negate the local z coordinate to convert it to global coordinate.
		double circle_centre_z_in_global_coords = -circle_centre_z_in_cube_face_coords;
		const double inv_circle_centre_z_in_global_coords = 1.0 / circle_centre_z_in_global_coords;

		//
		// Since the point geometry has no spatial extents we put it in the deepest
		// level of the quad tree - being a point it will never extend outside the bounds
		// of a node at the deepest level.
		//

		// Calculate the x,y offsets of the quad tree node position in the deepest level.
		// Use a small numerical tolerance to ensure the we keep the offset within range.
		const unsigned int max_level_width_in_nodes = (1 << d_maximum_quad_tree_depth);
		//
		// Note: Using static_cast<int> instead of static_cast<unsigned int> since
		// Visual Studio optimises for 'int' and not 'unsigned int'.
		//
		const int node_x_offset_at_max_depth = static_cast<int>(
				(0.5 - 1e-6) * max_level_width_in_nodes *
					(1 + inv_circle_centre_z_in_global_coords * circle_centre_x_in_cube_face_coords));
		const int node_y_offset_at_max_depth = static_cast<int>(
				(0.5 - 1e-6) * max_level_width_in_nodes *
					(1 + inv_circle_centre_z_in_global_coords * circle_centre_y_in_cube_face_coords));

		// We need to make sure the appropriate interior quad tree nodes are generated,
		// if they've not already been, along the way.
		unsigned int num_levels_to_max_depth = d_maximum_quad_tree_depth;
		int prev_node_x_offset = 0;
		int prev_node_y_offset = 0;
		// Get the root node of the quad tree of the desired cube face.
		cube_quad_tree_node_type *current_node =
				&d_cube_quad_tree->get_or_create_quad_tree_root_node(cube_face);
		while (num_levels_to_max_depth)
		{
			--num_levels_to_max_depth;

			const int node_x_offset =
					(node_x_offset_at_max_depth >> num_levels_to_max_depth);
			const int node_y_offset =
					(node_y_offset_at_max_depth >> num_levels_to_max_depth);

			const int child_x_offset = node_x_offset - 2 * prev_node_x_offset;
			const int child_y_offset = node_y_offset - 2 * prev_node_y_offset;

			// Make the child node the current node.
			current_node = &d_cube_quad_tree->get_or_create_child_node(
					*current_node, child_x_offset, child_y_offset);

			prev_node_x_offset = node_x_offset;
			prev_node_y_offset = node_y_offset;
		}

		add(element, current_node->get_element());

		if (location_added)
		{
			*location_added = location_type(
					cube_face,
					// Point geometry is added at the maximum depth...
					d_maximum_quad_tree_depth,
					node_x_offset_at_max_depth,
					node_y_offset_at_max_depth);
		}
	}


	template <typename ElementType>
	void
	CubeQuadTreePartition<ElementType>::add(
			const ElementType &element,
			const UnitVector3D &bounding_circle_centre,
			const AngularExtent &bounding_circle_extent,
			location_type *location_added)
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
		const CubeCoordinateFrame::CubeFaceType cube_face =
				CubeCoordinateFrame::get_cube_face_and_transformed_position(
						bounding_circle_centre,
						circle_centre_x_in_cube_face_coords,
						circle_centre_y_in_cube_face_coords,
						circle_centre_z_in_cube_face_coords);

		// Negate the local z coordinate to convert it to global coordinate.
		const double circle_centre_z_in_global_coords = -circle_centre_z_in_cube_face_coords;

		//
		// Project the bounding circle centre vector onto the cube face.
		//

		// See if the bounding circle is larger than a hemi-sphere.
		// If it is then it's too big to project onto the cube face.
		if (bounding_circle_extent.get_cosine().is_precisely_less_than(1e-4))
		{
			add(element, d_cube_quad_tree->get_or_create_root_element());

			if (location_added)
			{
				// Added to the root of the cube (not in any quad trees).
				*location_added = location_type();
			}

			return;
		}

		const double sin_e =
				std::sqrt(1+1e-12f - circle_centre_z_in_global_coords * circle_centre_z_in_global_coords);
		const double sin_a = bounding_circle_extent.get_sine().dval();

		const double sin_e_sin_a = sin_e * sin_a;
		const double cos_e_cos_a =
				circle_centre_z_in_global_coords * bounding_circle_extent.get_cosine().dval();

		// See if we can even calculate the maximum projected radius on the cube face.
		// If we can't then it means the bounding circle has a position and extent that
		// cannot be projected onto the cube face (ie, it wraps around the globe enough away
		// from the cube face that the projection onto the cube face is no longer well-defined).
		if (cos_e_cos_a < sin_e_sin_a + 1e-6)
		{
			add(element, d_cube_quad_tree->get_or_create_root_element());

			if (location_added)
			{
				// Added to the root of the cube (not in any quad trees).
				*location_added = location_type();
			}

			return;
		}

		const double max_projected_radius_on_cube_face =
				sin_a / (circle_centre_z_in_global_coords * (cos_e_cos_a - sin_e_sin_a));

		// The half-width of a quad tree node.
		// The root node is a whole cube face which has a half-width of 1.0 for a unit sphere.
		// Subtract a little bit to give a bit of padding to the bounds for numerical tolerance.
		double quad_tree_node_half_width = 1 - 1e-6;

		// If the max projected radius is larger than half the width of the cube face (ie 1.0)
		// then it is too large to fit within the 'loose' bounding square of the
		// root quad tree node of the cube face.
		if (max_projected_radius_on_cube_face > quad_tree_node_half_width)
		{
			// Add the element to the list of elements that don't belong to any quad tree.
			add(element, d_cube_quad_tree->get_or_create_root_element());

			if (location_added)
			{
				// Added to the root of the cube (not in any quad trees).
				*location_added = location_type();
			}

			return;
		}

		const double inv_circle_centre_z_in_global_coords = 1.0 / circle_centre_z_in_global_coords;

		// Calculate the x,y offsets of the quad tree node position as if it was
		// in the deepest level.
		// Use a small numerical tolerance to ensure the we keep the offset within range.
		const unsigned int max_level_width_in_nodes = (1 << d_maximum_quad_tree_depth);
		//
		// Note: Using static_cast<int> instead of static_cast<unsigned int> since
		// Visual Studio optimises for 'int' and not 'unsigned int'.
		//
		const int node_x_offset_at_max_depth = static_cast<int>(
				(0.5 - 1e-6) * max_level_width_in_nodes *
					(1 + inv_circle_centre_z_in_global_coords * circle_centre_x_in_cube_face_coords));
		const int node_y_offset_at_max_depth = static_cast<int>(
				(0.5 - 1e-6) * max_level_width_in_nodes *
					(1 + inv_circle_centre_z_in_global_coords * circle_centre_y_in_cube_face_coords));

		// Using the max projected radius (onto cube face) determine the level at which
		// to store in the cube face's quad tree.
		// Also generate the interior quad tree nodes as required along the way.
		unsigned int num_levels_to_max_depth = d_maximum_quad_tree_depth;
		int prev_node_x_offset = 0;
		int prev_node_y_offset = 0;
		// Get the root node of the quad tree of the desired cube face.
		cube_quad_tree_node_type *current_node =
				&d_cube_quad_tree->get_or_create_quad_tree_root_node(cube_face);
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

			// Make the child node the current node.
			current_node = &d_cube_quad_tree->get_or_create_child_node(
					*current_node, child_x_offset, child_y_offset);

			prev_node_x_offset = node_x_offset;
			prev_node_y_offset = node_y_offset;
			quad_tree_node_half_width *= 0.5;
		}

		add(element, current_node->get_element());

		if (location_added)
		{
			*location_added = location_type(
					cube_face,
					d_maximum_quad_tree_depth - num_levels_to_max_depth,
					prev_node_x_offset,
					prev_node_y_offset);
		}
	}


	template <typename ElementType>
	void
	CubeQuadTreePartition<ElementType>::add(
			const ElementType &element,
			const location_type &location,
			location_type *location_added)
	{
		// If the location is at the root of the cube (not in any quad trees) then
		// add as unpartitioned.
		if (!location.get_node_location())
		{
			add_unpartitioned(element);

			if (location_added)
			{
				// Added to the root of the cube (not in any quad trees).
				*location_added = location_type();
			}

			return;
		}

		location_type::NodeLocation node_location = location.get_node_location().get();
		// Adjust the location to add if it specifies a depth greater than our maximum depth.
		if (node_location.quad_tree_depth > d_maximum_quad_tree_depth)
		{
			const int depth_difference = node_location.quad_tree_depth - d_maximum_quad_tree_depth;
			node_location.x_node_offset >>= depth_difference;
			node_location.y_node_offset >>= depth_difference;
			node_location.quad_tree_depth = d_maximum_quad_tree_depth;
		}

		// Starting at the root node generate interior quad tree nodes along the way as required.
		unsigned int num_levels_to_depth = node_location.quad_tree_depth;
		int prev_node_x_offset = 0;
		int prev_node_y_offset = 0;
		// Get the root node of the quad tree of the desired cube face.
		cube_quad_tree_node_type *current_node =
				&d_cube_quad_tree->get_or_create_quad_tree_root_node(node_location.cube_face);
		while (num_levels_to_depth)
		{
			--num_levels_to_depth;

			const int node_x_offset =
					(node_location.x_node_offset >> num_levels_to_depth);
			const int node_y_offset =
					(node_location.y_node_offset >> num_levels_to_depth);

			const int child_x_offset = node_x_offset - 2 * prev_node_x_offset;
			const int child_y_offset = node_y_offset - 2 * prev_node_y_offset;

			// Make the child node the current node.
			current_node = &d_cube_quad_tree->get_or_create_child_node(
					*current_node, child_x_offset, child_y_offset);

			prev_node_x_offset = node_x_offset;
			prev_node_y_offset = node_y_offset;
		}

		add(element, current_node->get_element());

		if (location_added)
		{
			*location_added = location_type(node_location);
		}
	}


	//
	// Iterator implementation
	//

	template <typename ElementType>
	template <typename ElementQualifiedType>
	CubeQuadTreePartition<ElementType>::Iterator<ElementQualifiedType>::Iterator(
			const Iterator<element_type> &rhs) :
		d_cube_quad_tree_iterator(rhs.d_cube_quad_tree_iterator),
		d_current_element_list_iterator(rhs.d_current_element_list_iterator),
		d_current_element_list_end(rhs.d_current_element_list_end),
		d_finished(rhs.d_finished)
	{
	}

	template <typename ElementType>
	template <typename ElementQualifiedType>
	CubeQuadTreePartition<ElementType>::Iterator<ElementQualifiedType>::Iterator(
			cube_quad_tree_qualified_type &cube_quad_tree) :
		d_cube_quad_tree_iterator(cube_quad_tree.get_iterator())
	{
		first();
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	void
	CubeQuadTreePartition<ElementType>::Iterator<ElementQualifiedType>::reset()
	{
		first();
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	void
	CubeQuadTreePartition<ElementType>::Iterator<ElementQualifiedType>::first()
	{
		d_finished = false;
		d_cube_quad_tree_iterator.reset();
		if (d_cube_quad_tree_iterator.finished())
		{
			d_finished = true;
			return;
		}

		const ElementList &element_list = d_cube_quad_tree_iterator.get_element();
		d_current_element_list_iterator = element_list.begin();
		d_current_element_list_end = element_list.end();

		// Find the first cube quad tree node that is not empty.
		while (d_current_element_list_iterator == d_current_element_list_end)
		{
			d_cube_quad_tree_iterator.next();
			if (d_cube_quad_tree_iterator.finished())
			{
				d_finished = true;
				return;
			}

			const ElementList &next_element_list = d_cube_quad_tree_iterator.get_element();
			d_current_element_list_iterator = next_element_list.begin();
			d_current_element_list_end = next_element_list.end();
		}
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	ElementQualifiedType &
	CubeQuadTreePartition<ElementType>::Iterator<ElementQualifiedType>::get_element() const
	{
		return d_current_element_list_iterator->get_element();
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	const typename CubeQuadTreePartition<ElementType>::location_type &
	CubeQuadTreePartition<ElementType>::Iterator<ElementQualifiedType>::get_location() const
	{
		return d_cube_quad_tree_iterator.get_location();
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	bool
	CubeQuadTreePartition<ElementType>::Iterator<ElementQualifiedType>::finished() const
	{
		return d_finished;
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	void
	CubeQuadTreePartition<ElementType>::Iterator<ElementQualifiedType>::next()
	{
		++d_current_element_list_iterator;

		while (d_current_element_list_iterator == d_current_element_list_end)
		{
			d_cube_quad_tree_iterator.next();
			if (d_cube_quad_tree_iterator.finished())
			{
				d_finished = true;
				return;
			}

			const ElementList &element_list = d_cube_quad_tree_iterator.get_element();
			d_current_element_list_iterator = element_list.begin();
			d_current_element_list_end = element_list.end();
		}
	}
}

#endif // GPLATES_MATHS_CUBEQUADTREEPARTITION_H
