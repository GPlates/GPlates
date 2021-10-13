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

#ifndef GPLATES_MATHS_CUBEQUADTREE_H
#define GPLATES_MATHS_CUBEQUADTREE_H

#include <vector>
#include <boost/optional.hpp>

#include "CubeCoordinateFrame.h"

#include "utils/CopyConst.h"
#include "utils/ObjectPool.h"
#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	/**
	 * Boilerplate code for creating and traversing a cube quad tree - a cube with each face
	 * containing a quad tree.
	 *
	 * Each quad tree node created contains an object of type 'ElementType'.
	 *
	 * Template parameter 'ElementType' must be copy-constructable and copy-assignable.
	 */
	template <typename ElementType>
	class CubeQuadTree :
			public GPlatesUtils::ReferenceCount< CubeQuadTree<ElementType> >
	{
	public:
		//! Typedef for the element type.
		typedef ElementType element_type;

		//! Typedef for this class type.
		typedef CubeQuadTree<ElementType> this_type;

		//! A convenience typedef for a shared pointer to a non-const @a CubeQuadTree.
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a CubeQuadTree.
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;


		/**
		 * Creates a @a CubeQuadTree object.
		 *
		 * You can also create one directly using constructor.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new CubeQuadTree());
		}


		/**
		 * A node in a quad tree.
		 *
		 * This can be used to traverse a quad tree of one of the cube faces.
		 */
		class Node
		{
		public:
			/**
			 * Returns element stored in quad tree node.
			 */
			const ElementType &
			get_element() const
			{
				return d_element;
			}

			/**
			 * Returns element stored in quad tree node.
			 */
			ElementType &
			get_element()
			{
				return d_element;
			}


			/**
			 * Returns the specified child node if it exists, otherwise NULL.
			 */
			const Node *
			get_child_node(
					unsigned int child_x_offset,
					unsigned int child_y_offset) const
			{
				return d_children[child_y_offset][child_x_offset].get_ptr();
			}

			/**
			 * Returns the specified child node if it exists, otherwise NULL.
			 */
			Node *
			get_child_node(
					unsigned int child_x_offset,
					unsigned int child_y_offset)
			{
				return d_children[child_y_offset][child_x_offset].get_ptr();
			}


			//! Typedef for a node pointer - used when building cube quad tree.
			typedef typename GPlatesUtils::ObjectPool<Node>::object_ptr_type ptr_type;

		private:
			/**
			 * Pointers to child nodes if they exist.
			 */
			ptr_type d_children[2][2];

			/**
			 * The element type to store in each quad tree node.
			 */
			ElementType d_element;


			explicit
			Node(
					const ElementType &element) :
				d_element(element)
			{  }

			//! Default constructor - 'ElementType' must have a default constructor.
			Node()
			{  }

			// Make a friend add/remove child nodes more easily and to construct.
			friend class CubeQuadTree<ElementType>;
		};

		//! Typedef for the quad tree node type.
		typedef Node node_type;


		/**
		 * Iterator over the cube quad tree.
		 *
		 * 'ElementQualifiedType' can be either 'element_type' or 'const element_type'.
		 */
		template <class ElementQualifiedType>
		class Iterator
		{
		public:
			//! Typedef for a cube quad tree of appropriate const-ness.
			typedef typename GPlatesUtils::CopyConst<
					ElementQualifiedType, CubeQuadTree<element_type> >::type
							cube_quad_tree_qualified_type;

			//! Typedef for cube quad tree node of appropriate const-ness.
			typedef typename GPlatesUtils::CopyConst<ElementQualifiedType, Node>::type node_qualified_type;


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

			void
			next();

			bool
			finished() const;

		private:
			struct NodeLocation
			{
				explicit
				NodeLocation(
						node_qualified_type *node_) :
					node(node_),
					child_x_offset(0),
					child_y_offset(0)
				{  }

				// Assists with conversion from 'iterator' to 'const_iterator'.
				NodeLocation(
						const typename Iterator<element_type>::NodeLocation &rhs) :
					node(rhs.node),
					child_x_offset(rhs.child_x_offset),
					child_y_offset(rhs.child_y_offset)
				{  }

				node_qualified_type *node;
				unsigned short child_x_offset;
				unsigned short child_y_offset;
			};

			cube_quad_tree_qualified_type *d_cube_quad_tree;
			std::vector<NodeLocation> d_current_quad_tree_traversal_stack;
			unsigned short d_current_cube_face;
			bool d_at_root_element;
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
		 * Returns a non-const iterator over the elements of this cube quad tree.
		 *
		 * This is a convenience for when you don't care about the order of iteration but
		 * just want to iterate over all elements in the cube quad tree.
		 */
		iterator
		get_iterator()
		{
			return iterator(*this);
		}


		/**
		 * Returns a const iterator over the elements of this cube quad tree.
		 *
		 * This is a convenience for when you don't care about the order of iteration but
		 * just want to iterate over all elements in the cube quad tree.
		 */
		const_iterator
		get_iterator() const
		{
			return const_iterator(*this);
		}





		/**
		 * Returns the root element if it exists, otherwise NULL.
		 *
		 * This element corresponds to the root of the entire cube.
		 * An example use is geometries that don't fit into any quad tree (or cube face).
		 */
		const ElementType *
		get_root_element() const
		{
			return d_root_element ? &d_root_element.get() : NULL;
		}

		/**
		 * Returns the root element if it exists, otherwise NULL.
		 *
		 * This element corresponds to the root of the entire cube.
		 * An example use is geometries that don't fit into any quad tree (or cube face).
		 */
		ElementType *
		get_root_element()
		{
			return d_root_element ? &d_root_element.get() : NULL;
		}

		/**
		 * Gets the root element.
		 *
		 * Creates a new root element if one doesn't already exist and
		 * initialises it with a default-constructed 'ElementType'.
		 *
		 * This method requires 'ElementType' to have a default constructor.
		 */
		ElementType &
		get_or_create_root_element();

		/**
		 * Sets the root element.
		 */
		void
		set_root_element(
				const ElementType &root_element)
		{
			d_root_element = root_element;
		}

		/**
		 * Clears the entire cube quad tree including the root element.
		 *
		 * In fact this is the only way to clear the root element.
		 * This is because it effectively represents the root of the cube quad tree
		 * and clearing the root should clear everything below it (ie, all the cube face quad trees).
		 */
		void
		clear()
		{
			remove_quad_tree_root_node(CubeCoordinateFrame::POSITIVE_X);
			remove_quad_tree_root_node(CubeCoordinateFrame::NEGATIVE_X);
			remove_quad_tree_root_node(CubeCoordinateFrame::POSITIVE_Y);
			remove_quad_tree_root_node(CubeCoordinateFrame::NEGATIVE_Y);
			remove_quad_tree_root_node(CubeCoordinateFrame::POSITIVE_Z);
			remove_quad_tree_root_node(CubeCoordinateFrame::NEGATIVE_Z);
			d_root_element = boost::none;
		}


		/**
		 * Returns the root quad tree node of the specified cube face if it exists, otherwise NULL.
		 */
		const Node *
		get_quad_tree_root_node(
				CubeCoordinateFrame::CubeFaceType cube_face) const
		{
			return d_quad_trees[cube_face].root_node.get_ptr();
		}

		/**
		 * Returns the root quad tree node of the specified cube face if it exists, otherwise NULL.
		 */
		Node *
		get_quad_tree_root_node(
				CubeCoordinateFrame::CubeFaceType cube_face)
		{
			return d_quad_trees[cube_face].root_node.get_ptr();
		}


		/**
		 * Gets the root node of the specified cube face (quad tree).
		 *
		 * Creates a new root node if one doesn't already exist and
		 * initialises it with a default-constructed 'ElementType'.
		 *
		 * This method requires 'ElementType' to have a default constructor.
		 */
		Node &
		get_or_create_quad_tree_root_node(
				CubeCoordinateFrame::CubeFaceType cube_face);


		/**
		 * Sets the specified root node to the specified element type.
		 *
		 * If the root node exists then it is recursively removed first.
		 */
		Node &
		set_quad_tree_root_node(
				CubeCoordinateFrame::CubeFaceType cube_face,
				const ElementType &element)
		{
			typename Node::ptr_type root_node = create_node(element);
			set_quad_tree_root_node(cube_face, root_node);
			return root_node.get();
		}

		/**
		 * Removes the specified root node, if it exists, and recursively removes any descendants.
		 */
		void
		remove_quad_tree_root_node(
				CubeCoordinateFrame::CubeFaceType cube_face);


		/**
		 * Gets the child node of the specified parent node.
		 *
		 * Creates a new child node if one doesn't already exist and
		 * initialises it with a default-constructed 'ElementType'.
		 *
		 * This method requires 'ElementType' to have a default constructor.
		 */
		Node &
		get_or_create_child_node(
				Node &parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset);


		/**
		 * Sets the child node of the specified parent node to the specified element type.
		 *
		 * If the child node exists then it is recursively removed first.
		 */
		Node &
		set_child_node(
				Node &parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset,
				const ElementType &element)
		{
			typename Node::ptr_type child_node = create_node(element);
			set_child_node(parent_node, child_x_offset, child_y_offset, child_node);
			return child_node.get();
		}


		/**
		 * Removes the child of the specified parent node, if it exists, and recursively removes any descendants.
		 */
		void
		remove_child_node(
				Node &parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset);


		//
		// The following support a more builder-pattern style of creating a cube quad tree where
		// nodes are created first and then later attached to this cube quad tree.
		//
		// This is a bit more dangerous since it's possible for the user to create nodes and
		// never attach them to the cube quad tree - they will eventually get released but only
		// when 'this' cube quad tree is destroyed.
		//

		/**
		 * Creates a 'dangling' quad tree node containing a copy of 'element'.
		 *
		 * If it's not attached to 'this' cube quad tree or released with @a release_node it
		 * will still get destroyed when 'this' is destroyed.
		 *
		 * Once attached to 'this' cube quad tree you should not call @a release_node with it.
		 */
		typename Node::ptr_type
		create_node(
				const ElementType &element)
		{
			return d_quad_tree_node_pool.add(Node(element));
		}

		/**
		 * Releases a 'dangling' quad tree node - should only be used if you created @a node
		 * with @a create_node and decided not to attach it to 'this' cube quad tree
		 * (eg, an exception was thrown after @a create_node but before it could be attached).
		 *
		 * Since the node gets released when 'this' is destroyed regardless, it won't result
		 * in a permanent memory leak if you don't attach it to 'this' cube quad tree and
		 * don't call @a release_node.
		 *
		 * NOTE: This should never be called on a node that is part of 'this' cube quad tree
		 * otherwise it will corrupt the cube quad tree.
		 */
		void
		release_node(
				typename Node::ptr_type node)
		{
			d_quad_tree_node_pool.release(node);
		}

		/**
		 * An alternative to the other overload of this method that uses @a create_node.
		 *
		 * This allows the user to build a quad tree before attaching it to 'this' cube quad tree.
		 */
		void
		set_quad_tree_root_node(
				CubeCoordinateFrame::CubeFaceType cube_face,
				typename Node::ptr_type root_node);

		/**
		 * An alternative to the other overload of this method that uses @a create_node.
		 *
		 * This allows the user to build a quad tree before attaching it to 'this' cube quad tree.
		 */
		void
		set_child_node(
				Node &parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset,
				typename Node::ptr_type child_node);

	private:
		/**
		 * Each cube face has a quad tree.
		 */
		struct QuadTree
		{
			typename Node::ptr_type root_node;
		};

		//! Typedef for an object pool for type @a Node.
		typedef GPlatesUtils::ObjectPool<Node> quad_tree_node_pool_type;


		/**
		 * All quad tree nodes, except the root nodes, are stored in this pool.
		 */
		quad_tree_node_pool_type d_quad_tree_node_pool;

		/**
		 * The root element of the entire cube.
		 *
		 * Typically used when a geometry does not fit into any quad tree (cube face).
		 */
		boost::optional<ElementType> d_root_element;

		/**
		 * A quad tree for each cube face. 
		 */
		QuadTree d_quad_trees[CubeCoordinateFrame::NUM_FACES];


		CubeQuadTree()
		{  }
	};


	template <typename ElementType>
	ElementType &
	CubeQuadTree<ElementType>::get_or_create_root_element()
	{
		if (!d_root_element)
		{
			d_root_element = ElementType();
		}

		return d_root_element.get();
	}


	template <typename ElementType>
	typename CubeQuadTree<ElementType>::Node &
	CubeQuadTree<ElementType>::get_or_create_quad_tree_root_node(
			CubeCoordinateFrame::CubeFaceType cube_face)
	{
		typename Node::ptr_type &root_node_ptr = d_quad_trees[cube_face].root_node;
		if (!root_node_ptr)
		{
			root_node_ptr = d_quad_tree_node_pool.add(Node());
		}

		return *root_node_ptr;
	}


	template <typename ElementType>
	void
	CubeQuadTree<ElementType>::set_quad_tree_root_node(
			CubeCoordinateFrame::CubeFaceType cube_face,
			typename Node::ptr_type root_node)
	{
		typename Node::ptr_type &root_node_ptr = d_quad_trees[cube_face].root_node;
		if (root_node_ptr)
		{
			remove_quad_tree_root_node(cube_face);
		}

		root_node_ptr = root_node;
	}


	template <typename ElementType>
	void
	CubeQuadTree<ElementType>::remove_quad_tree_root_node(
			CubeCoordinateFrame::CubeFaceType cube_face)
	{
		typename Node::ptr_type &root_node_ptr = d_quad_trees[cube_face].root_node;
		if (root_node_ptr)
		{
			// Remove the children recursively as needed.
			Node &root_node_ref = *root_node_ptr;
			remove_child_node(root_node_ref, 0, 0);
			remove_child_node(root_node_ref, 0, 1);
			remove_child_node(root_node_ref, 1, 0);
			remove_child_node(root_node_ref, 1, 1);

			// Reuse the root node by returning it to the pool.
			d_quad_tree_node_pool.release(root_node_ptr);
			root_node_ptr = typename Node::ptr_type();
		}
	}


	template <typename ElementType>
	typename CubeQuadTree<ElementType>::Node &
	CubeQuadTree<ElementType>::get_or_create_child_node(
			Node &parent_node,
			unsigned int child_x_offset,
			unsigned int child_y_offset)
	{
		typename Node::ptr_type &child_node_ptr = parent_node.d_children[child_y_offset][child_x_offset];
		if (!child_node_ptr)
		{
			child_node_ptr = d_quad_tree_node_pool.add(Node());
		}

		return *child_node_ptr;
	}


	template <typename ElementType>
	void
	CubeQuadTree<ElementType>::set_child_node(
			Node &parent_node,
			unsigned int child_x_offset,
			unsigned int child_y_offset,
			typename Node::ptr_type child_node)
	{
		typename Node::ptr_type &child_node_ptr = parent_node.d_children[child_y_offset][child_x_offset];
		if (child_node_ptr)
		{
			remove_child_node(parent_node, child_x_offset, child_y_offset);
		}

		child_node_ptr = child_node;
	}


	template <typename ElementType>
	void
	CubeQuadTree<ElementType>::remove_child_node(
			Node &parent_node,
			unsigned int child_x_offset,
			unsigned int child_y_offset)
	{
		typename Node::ptr_type &child_node_ptr = parent_node.d_children[child_y_offset][child_x_offset];
		if (child_node_ptr)
		{
			// Remove recursively as needed.
			Node &child_node_ref = *child_node_ptr;
			remove_child_node(child_node_ref, 0, 0);
			remove_child_node(child_node_ref, 0, 1);
			remove_child_node(child_node_ref, 1, 0);
			remove_child_node(child_node_ref, 1, 1);

			// Reuse the child node by returning it to the pool.
			d_quad_tree_node_pool.release(child_node_ptr);
			child_node_ptr = typename Node::ptr_type();
		}
	}


	//
	// Iterator implementation
	//

	template <typename ElementType>
	template <typename ElementQualifiedType>
	CubeQuadTree<ElementType>::Iterator<ElementQualifiedType>::Iterator(
			const Iterator<element_type> &rhs) :
		d_cube_quad_tree(rhs.d_cube_quad_tree),
		d_current_quad_tree_traversal_stack(
				rhs.d_current_quad_tree_traversal_stack.begin(),
				rhs.d_current_quad_tree_traversal_stack.end()),
		d_current_cube_face(rhs.d_current_cube_face),
		d_at_root_element(rhs.d_at_root_element),
		d_finished(rhs.d_finished)
	{
	}

	template <typename ElementType>
	template <typename ElementQualifiedType>
	CubeQuadTree<ElementType>::Iterator<ElementQualifiedType>::Iterator(
			cube_quad_tree_qualified_type &cube_quad_tree) :
		d_cube_quad_tree(&cube_quad_tree)
	{
		first();
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	void
	CubeQuadTree<ElementType>::Iterator<ElementQualifiedType>::reset()
	{
		d_current_quad_tree_traversal_stack.clear();
		first();
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	void
	CubeQuadTree<ElementType>::Iterator<ElementQualifiedType>::first()
	{
		d_current_cube_face = 0;
		d_finished = false;
		if (d_cube_quad_tree->get_root_element())
		{
			d_at_root_element = true;
		}
		else
		{
			d_at_root_element = false;
			// Move to the first element.
			next();
		}
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	ElementQualifiedType &
	CubeQuadTree<ElementType>::Iterator<ElementQualifiedType>::get_element() const
	{
		return d_at_root_element
				? *d_cube_quad_tree->get_root_element()
				: d_current_quad_tree_traversal_stack.back().node->get_element();
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	bool
	CubeQuadTree<ElementType>::Iterator<ElementQualifiedType>::finished() const
	{
		return d_finished;
	}


	template <typename ElementType>
	template <typename ElementQualifiedType>
	void
	CubeQuadTree<ElementType>::Iterator<ElementQualifiedType>::next()
	{
		// If at the root element then transition to the quad tree of the first cube face.
		if (d_at_root_element)
		{
			d_at_root_element = false;
			d_current_cube_face = 0;
		}

		// Infinite loop.
		for (;;)
		{
			// If the quad tree traversal stack is empty then we are at the root node
			// of the quad tree of the current cube face.
			if (d_current_quad_tree_traversal_stack.empty())
			{
				// If there's no more cube faces to traverse then we're finished.
				if (d_current_cube_face == 6)
				{
					d_finished = true;
					return;
				}

				// Get the current quad tree's root node.
				node_qualified_type *const root_node =
						d_cube_quad_tree->get_quad_tree_root_node(
								static_cast<CubeCoordinateFrame::CubeFaceType>(d_current_cube_face));

				// Move to the next cube face.
				++d_current_cube_face;

				if (!root_node)
				{
					// Continue to the next cube face since there's no quad tree root node.
					continue;
				}

				// Push the root node onto the traversal stack and return.
				d_current_quad_tree_traversal_stack.push_back(NodeLocation(root_node));
				return;
			}

			NodeLocation &node_location = d_current_quad_tree_traversal_stack.back();

			while (node_location.child_y_offset < 2)
			{
				// Look for a child node of the current node.
				node_qualified_type *const child_node =
						node_location.node->get_child_node(
								node_location.child_x_offset, node_location.child_y_offset);

				// Move to the next child position.
				if (++node_location.child_x_offset == 2)
				{
					node_location.child_x_offset = 0;
					// Note that this can increment to 2.
					++node_location.child_y_offset;
				}

				if (child_node)
				{
					// Push the child node onto the traversal stack and return.
					d_current_quad_tree_traversal_stack.push_back(NodeLocation(child_node));
					return;
				}
			}

			// No more child nodes for the current node.
			d_current_quad_tree_traversal_stack.pop_back();
		}

		// Cannot get here because there are no 'break's in the above infinite loop.
	}
}

#endif // GPLATES_MATHS_CUBEQUADTREE_H
