/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_SMARTNODELINKEDLIST_H
#define GPLATES_UTILS_SMARTNODELINKEDLIST_H

#include <iterator>  // std::iterator, std::bidirectional_iterator_tag
#include <boost/operators.hpp>


namespace GPlatesUtils
{
	/**
	 * A doubly-linked list of "smart" nodes -- that is, nodes which are able to manage
	 * themselves.
	 *
	 * In contrast to the nodes in STL std::list, the nodes in this list are not just visible,
	 * they can be contained directly in another object.  As a result, they need to be able to
	 * splice themselves out of the list automatically upon destruction.
	 *
	 * The nodes may be used independently, but this SmartNodeLinkedList class provides a
	 * circular doubly-linked list with a sentinel node, plus conveniences like iterators and
	 * an "append" function.  The sentinel is contained within the list (which thus controls
	 * the lifetime of the sentinel), which means that there's no additional heap allocation
	 * for an empty list.
	 */
	template <typename T>
	class SmartNodeLinkedList
	{
	public:
		typedef T element_type;

		class Node
		{
		public:
			/**
			 * Construct a new node which contains @a elem.
			 */
			explicit
			Node(
					const element_type &elem):
				d_element(elem)
			{
				// NOTE: 'd_prev_ptr' and 'd_next_ptr' are initialised in the constructor
				// body because Visual Studio 8 (2005) gives a warning (C4355) about using
				// 'this' in member initialiser list (which is odd because it doesn't do
				// that with other classes that do the same thing). Also strange is this
				// warning doesn't happen when using precompiled headers in MSVC8.
				d_prev_ptr = this;
				d_next_ptr = this;
			}

			/**
			 * Copy-constructor.
			 *
			 * Not @em exact copy-construction, but the best we can do without trashing
			 * the list-structure around @a other.
			 *
			 * The copy-constructor doesn't copy the list-links, only the element; the
			 * list-links are initialised so the new Node instance has no neighbours.
			 */
			Node(
					const Node &other):
				d_element(other.d_element)
			{
				// NOTE: 'd_prev_ptr' and 'd_next_ptr' are initialised in the constructor
				// body because Visual Studio 8 (2005) gives a warning (C4355) about using
				// 'this' in member initialiser list (which is odd because it doesn't do
				// that with other classes that do the same thing). Also strange is this
				// warning doesn't happen when using precompiled headers in MSVC8.
				d_prev_ptr = this;
				d_next_ptr = this;
			}

			~Node()
			{
				splice_self_out();
			}

			element_type &
			element()
			{
				return d_element;
			}

			Node *
			prev() const
			{
				return d_prev_ptr;
			}

			Node *
			next() const
			{
				return d_next_ptr;
			}

			bool
			has_neighbours() const
			{
				return (d_prev_ptr != this);
			}

			/**
			 * Splice this node before @a other.
			 *
			 * It's fine to invoke this operation if this node is currently in a list;
			 * the node will automatically be spliced out first.
			 */
			void
			splice_self_before(
					Node &other)
			{
				if (has_neighbours()) {
					splice_self_out();
				}

				other.d_prev_ptr->d_next_ptr = this;
				d_prev_ptr = other.d_prev_ptr;
				other.d_prev_ptr = this;
				d_next_ptr = &other;
			}

			/**
			 * Splice this node out of the list (if this node is in a list).
			 *
			 * It's fine to invoke this operation if this node is not currently in a
			 * list; the operation will be a no-op.
			 */
			void
			splice_self_out()
			{
				// Re-direct the nodes on either side.
				// 
				// Note that this still works if there AREN'T any nodes on either
				// side:  In such cases, both members will be equal to 'this', so
				// you'll just be assigning 'this' to members which are already
				// 'this'.  Redundant, yes, but AWESOME cache locality.  :P
				d_next_ptr->d_prev_ptr = d_prev_ptr;
				d_prev_ptr->d_next_ptr = d_next_ptr;

				// Reset the members to safe values.
				d_prev_ptr = this;
				d_next_ptr = this;
			}

		private:
			element_type d_element;
			Node *d_prev_ptr;
			Node *d_next_ptr;

			// Disallow copy-assignment.
			Node &
			operator=(
					const Node &other);
		};

		class NodeIterator:
				public std::iterator<std::bidirectional_iterator_tag, element_type>,
				public boost::bidirectional_iteratable<NodeIterator, element_type *>
		{
		public:
			explicit
			NodeIterator(
					Node &node):
				d_node_ptr(&node)
			{  }

			element_type &
			operator*() const
			{
				return access_element();
			}

			Node *
			get() const
			{
				return d_node_ptr;
			}

			/**
			 * Pre-increment the iterator.
			 */
			NodeIterator &
			operator++()
			{
				increment();
				return *this;
			}

			/**
			 * Pre-decrement the iterator.
			 */
			NodeIterator &
			operator--()
			{
				decrement();
				return *this;
			}

			bool
			operator==(
					const NodeIterator &other) const
			{
				return (other.d_node_ptr == d_node_ptr);
			}

		private:
			Node *d_node_ptr;

			element_type &
			access_element() const
			{
				return d_node_ptr->element();
			}

			void
			increment()
			{
				d_node_ptr = d_node_ptr->next();
			}

			void
			decrement()
			{
				d_node_ptr = d_node_ptr->prev();
			}
		};

		typedef NodeIterator iterator;

		/**
		 * Construct a new SmartNodeLinkedList, using @a null_elem_for_sentinel as the
		 * element contained in the sentinel node.
		 */
		explicit
		SmartNodeLinkedList(
				const element_type &null_elem_for_sentinel = element_type()):
			d_sentinel(null_elem_for_sentinel)
		{  }

		/**
		 * Copy-constructor.
		 *
		 * Note that we're relying upon the copy-constructor of Node (which is invoked for
		 * the sentinel member) to perform the appropriate actions to ensure that we don't
		 * trash the list-structure of @a other.
		 *
		 * Currently, the copy-constructor of Node doesn't copy the list-links, only the
		 * element; the list-links are initialised so the Node has no neighbours.  As a
		 * result, copy-constructing a SmartNodeLinkedList simply creates a new instance
		 * with the same sentinel element.
		 */
		SmartNodeLinkedList(
				const SmartNodeLinkedList &other):
			d_sentinel(other.d_sentinel)
		{  }

		bool
		empty() const
		{
			return !d_sentinel.has_neighbours();
		}

		iterator
		begin()
		{
			return iterator(*(d_sentinel.next()));
		}

		iterator
		end()
		{
			return iterator(d_sentinel);
		}

		void
		append(
				Node &new_node)
		{
			new_node.splice_self_before(d_sentinel);
		}

	private:
		Node d_sentinel;

		// Disallow copy-assignment.
		SmartNodeLinkedList &
		operator=(
				const SmartNodeLinkedList &other);
	};


	/**
	 * This is equivalent to std::list::splice except there's no need to specify the
	 * list objects themselves (as only the list node objects are required).
	 *
	 * Removes a linked list node referenced by @a where_to_remove_from_source_list
	 * and inserts it before the linked list node referenced by @a where_to_insert_into_destination_list.
	 */
	template <typename T>
	void
	splice(
			typename SmartNodeLinkedList<T>::iterator where_to_insert_into_destination_list,
			typename SmartNodeLinkedList<T>::iterator where_to_remove_from_source_list)
	{
		where_to_remove_from_source_list.get()->splice_self_before(
				*where_to_insert_into_destination_list.get());
	}

	/**
	 * Same as the other overload of @a splice except directly referencing the node from source list.
	 */
	template <typename T>
	void
	splice(
			typename SmartNodeLinkedList<T>::iterator where_to_insert_into_destination_list,
			typename SmartNodeLinkedList<T>::Node &node_to_remove_from_source_list)
	{
		node_to_remove_from_source_list.splice_self_before(
				*where_to_insert_into_destination_list.get());
	}
}

#endif // GPLATES_UTILS_SMARTNODELINKEDLIST_H
