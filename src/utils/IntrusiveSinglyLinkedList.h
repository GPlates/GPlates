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

#ifndef GPLATES_UTILS_INTRUSIVESINGLYLINKEDLIST_H
#define GPLATES_UTILS_INTRUSIVESINGLYLINKEDLIST_H

#include <iterator>  // std::iterator
#include <boost/operators.hpp>
#include <boost/type_traits/add_const.hpp>


namespace GPlatesUtils
{
	/**
	 * A low-level intrusive singly-linked list to be used only where speed and
	 * memory efficiency are required (otherwise a 'std::list' should be sufficient).
	 *
	 * NOTE: This list supports tail-sharing between lists - that is, multiple lists can share
	 * the same common tail sequence of elements - all methods support this.
	 * This can be useful when traversing a directed-acyclic graph structure and maintaining
	 * a list of ancestors for each node in the graph.
	 *
	 * This class is designed to use minimal memory and only supports some basic list operations
	 * and forward iteration over the list.
	 *
	 * Memory management of the list nodes is the responsibility of the client.
	 *
	 * Template parameter ElementNodeType must inherit publicly
	 * from IntrusiveSinglyLinkedList<ElementNodeType>::Node.
	 *
	 * An example:
	 *
	 *   struct ElementNode :
	 *      public IntrusiveSinglyLinkedList<ElementNode>::Node
	 *   {
	 *      int var;
	 *   };
	 *
	 *   IntrusiveSinglyLinkedList<ElementNode> list;
	 *   ElementNode element_node = { 1 };
	 *   list.push_front(element_node);
	 *
	 * When an ElementNodeType instance can be in more than one list it needs to inherit
	 * from Node more than once - and should use different types for NodeTag to distinguish
	 * between the lists.
	 *
	 * An example where a list node element is added to two different lists:
	 *
	 *   struct ElementNode :
	 *      public IntrusiveSinglyLinkedList<ElementNode, FirstListTag>::Node,
	 *      public IntrusiveSinglyLinkedList<ElementNode, SecondListTag>::Node
	 *   {
	 *      int var;
	 *   };
	 *
	 *   IntrusiveSinglyLinkedList<ElementNode, FirstListTag> first_list;
	 *   IntrusiveSinglyLinkedList<ElementNode, SecondListTag> second_list;
	 *   ElementNode element_node = { 1 };
	 *   first_list.push_front(element_node);
	 *   second_list.push_front(element_node);
	 *
	 * Once the minimum version requirement of the boost library is raised to 1.35
	 * we could instead use the boost version of an intrusive singly linked list 'boost::slist'.
	 */
	template <class ElementNodeType, class NodeTag = void>
	class IntrusiveSinglyLinkedList
	{
	public:
		//! The template parameter ElementNodeType must inherit publicly from this class.
		class Node
		{
		public:
			Node() :
				d_next_node(NULL)
			{  }

			ElementNodeType *
			get_next_node() const
			{
				return d_next_node;
			}

		private:
			friend class IntrusiveSinglyLinkedList<ElementNodeType,NodeTag>;

			ElementNodeType *d_next_node;
		};


		/**
		 * Iterator over the list.
		 *
		 * 'ElementNodeQualifiedType' can be either 'ElementNodeType' or 'const ElementNodeType'.
		 */
		template <class ElementNodeQualifiedType>
		class Iterator :
				public std::iterator<std::forward_iterator_tag, ElementNodeQualifiedType>,
				public boost::forward_iteratable<Iterator<ElementNodeQualifiedType>, ElementNodeQualifiedType *>
		{
		public:
			explicit
			Iterator(
					ElementNodeQualifiedType *node = NULL) :
				d_node(node)
			{  }

			/**
			 * Implicit conversion constructor from 'iterator' to 'const_iterator'.
			 */
			Iterator(
					const Iterator<ElementNodeType> &rhs) :
				d_node(rhs.d_node)
			{  }

			/**
			 * Dereference operator returns the list element of type 'ElementNodeQualifiedType'
			 * which is either 'ElementNodeType' or 'const ElementNodeType' depending on whether
			 * this is an 'iterator' or 'const_iterator'.
			 *
			 * 'operator->()' provided by base class boost::forward_iteratable.
			 */
			ElementNodeQualifiedType &
			operator*() const
			{
				return *d_node;
			}

			//! Post-increment operator provided by base class boost::forward_iteratable.
			Iterator &
			operator++()
			{
				// Use "IntrusiveSinglyLinkedList<ElementNodeQualifiedType,NodeTag>::Node::" to pick the correct
				// base class in case ElementNodeType inherits from more than once
				// Node bass class (ie, is in more than one list).
				d_node = d_node->IntrusiveSinglyLinkedList<ElementNodeType, NodeTag>
						::Node::get_next_node();
				return *this;
			}

			//! Inequality operator provided by base class boost::forward_iteratable.
			friend
			bool
			operator==(
					const Iterator &lhs,
					const Iterator &rhs)
			{
				return lhs.d_node == rhs.d_node;
			}

		private:
			ElementNodeQualifiedType *d_node;

			friend class Iterator<typename boost::add_const<ElementNodeType>::type>; // The const iterator.
		};

		//! Typedef for iterator.
		typedef Iterator<ElementNodeType> iterator;

		//! Typedef for const iterator.
		typedef Iterator< typename boost::add_const<ElementNodeType>::type > const_iterator;


		IntrusiveSinglyLinkedList() :
			d_list(NULL)
		{  }


		/**
		 * Copy constructor.
		 *
		 * NOTE: This shares (tail shares) the same elements as @a other_list.
		 * Subsequent pushing and popping elements from either list will not affect
		 * the other list. Although the memory management of the shared nodes is still
		 * the responsibility of the caller.
		 */
		explicit
		IntrusiveSinglyLinkedList(
				const IntrusiveSinglyLinkedList &other_list) :
			d_list(other_list.d_list)
		{  }


		/**
		 * Assignment operator.
		 *
		 * This behaves like the copy constructor in that both lists tail share elements.
		 *
		 * It is the caller's responsibility to manage the memory of any elements referenced
		 * by this list before assignment. For example, the previous elements might no longer be
		 * referenced by anyone and then be released or reused.
		 */
		IntrusiveSinglyLinkedList &
		operator=(
				const IntrusiveSinglyLinkedList &other_list)
		{
			d_list = other_list.d_list;
			return *this;
		}


		/**
		 * Clears the list.
		 *
		 * This does not destroy the objects in the list since the client owns the objects
		 * (and manages their memory).
		 */
		void
		clear()
		{
			d_list = NULL;
		}


		bool
		empty() const
		{
			return d_list == NULL;
		}


		/**
		 * Returns the element at the front of the list.
		 *
		 * NOTE: Undefined behaviour (eg, crash) will result if this list is empty.
		 */
		const ElementNodeType &
		front() const
		{
			return *d_list;
		}


		/**
		 * Returns the element at the front of the list.
		 *
		 * NOTE: Undefined behaviour (eg, crash) will result if this list is empty.
		 */
		ElementNodeType &
		front()
		{
			return *d_list;
		}


		/**
		 * Adds the specified element to the front of the list.
		 *
		 * The caller owns the element (the element node) and is responsible for managing its memory.
		 */
		void
		push_front(
				ElementNodeType *const node)
		{
			// Use "IntrusiveSinglyLinkedList<ElementNodeType,NodeTag>::Node::" to pick the correct
			// base class in case ElementNodeType inherits from more than once
			// Node bass class (ie, is in more than one list).
			node->IntrusiveSinglyLinkedList<ElementNodeType,NodeTag>::Node::d_next_node = d_list;
			d_list = node;
		}


		/**
		 * Removes the element at the front of the list.
		 *
		 * This does not destroy the object as the client owns the object (and manages its memory).
		 *
		 * NOTE: Undefined behaviour (eg, crash) will result if this list is empty.
		 */
		void
		pop_front()
		{
			// Use "IntrusiveSinglyLinkedList<ElementNodeType,NodeTag>::Node::" to pick the correct
			// base class in case ElementNodeType inherits from more than one
			// Node bass class (ie, is in more than one list).
			d_list = d_list->IntrusiveSinglyLinkedList<ElementNodeType,NodeTag>::Node::d_next_node;
		}


		iterator
		begin()
		{
			return iterator(d_list);
		}

		iterator
		end()
		{
			return iterator(NULL);
		}


		const_iterator
		begin() const
		{
			return const_iterator(d_list);
		}

		const_iterator
		end() const
		{
			return const_iterator(NULL);
		}

	private:
		ElementNodeType *d_list;
	};
}

#endif // GPLATES_UTILS_INTRUSIVESINGLYLINKEDLIST_H
