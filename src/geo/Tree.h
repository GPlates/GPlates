/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_TREE_H_
#define _GPLATES_GEO_TREE_H_

#include <set>

namespace GPlatesGeo
{
	/**
	 * Provides a fairly small subset of the STL methods on
	 * collections.  Should provide similar semantics to std::multiset.
	 * Namely, a collection where the elements are sorted automatically
	 * according to some sorting criterion -- i.e. @a Compare --, and 
	 * duplicates are allowed.
	 *
	 * The name is currently Tree because I have hopes that various
	 * different spatial data structures will be representable by the
	 * use of different @a Compare classes.
	 *
	 * @warning At the moment, the requirements on @a ElemType are that it 
	 *   is assignable, copyable, and comparable according to the sorting
	 *   criterion.  Also, presently the @a Compare type must define 'strict 
	 *   weak ordering'.  That is, supposing the operation is op(x,y), it 
	 *   must be:
	 *   - @em antisymmetric: If op(x,y) is true, then op(y,x) is false;
	 *   - @em transitive: If op(x,y) and op(y,z) are true, then op(x,z)
	 *     is true;
	 *   - @em irreflexive: op(x,x) is false.<br>
	 *   <b>The requirements on @a ElemType and @a Compare may change when 
	 *   the internal representation is worked out in more detail</b>.
	 * @note The idea is that eventually we'll have something along the
	 *   following lines for each tree type supported:
	 *   @code 
	 *   typedef Tree<GeologicalData*, RTreeCompare<GeologicalData*> > RTree; 
	 *   @endcode
	 *   Actually, there's a good chance that we'll have to use
	 *   inheritance instead, since the internal representation for the
	 *   different trees could vary considerably.
	 * @remark The comments for the methods of this class were based 
	 *   on the comments provided by Josuttis for the equivalent methods
	 *   of the std::multiset class (p175ff.).
	 * @todo On page 178 of Josuttis, he describes the relative merits 
	 *   of having the sorting criterion as a template parameter (such
	 *   as I have at the moment), and having it as a parameter to the
	 *   constructor.  A decision needs to be made as to which of these
	 *   to support (maybe both?).
	 */
	template<typename ElemType, typename Compare = std::less<ElemType> >
	class Tree
	{
		private:
			typedef std::multiset<ElemType, Compare> container_t;
			
		public:
			/**
			 * The type of the elements.
			 */
			typedef ElemType value_type;

			/**
			 * The type of element references.
			 */
			typedef value_type& reference;

			/**
			 * The type of constant element references.
			 */
			typedef const reference const_reference;

			/**
			 * The type of iterators.  Bidirectional, element is 
			 * constant.
			 */
			typedef typename container_t::iterator iterator;

			/**
			 * The type of constant iterators.  Bidirectional, element
			 * is constant.
			 */
			typedef typename container_t::const_iterator const_iterator;

			/**
			 * The unsigned integral type for size values.
			 */
			typedef typename container_t::size_type size_type;

			
			/**
			 * Create an empty Tree without any elements.
			 */
			Tree() {  }

			/**
			 * Create a Tree initialised by the elements in the range
			 * [begin, end).
			 */
			template<typename InputIterator>
			Tree(InputIterator begin, InputIterator end) 
				: _elements(begin, end) {  }
			
			/**
			 * Create a copy of another Tree of the same type (all the 
			 * elements are copied).
			 */
			Tree(const Tree& t) 
				: _elements(t._elements), _compare(t._compare) {  }

			/**
			 * Destroy all the elements and free the memory.
			 */
			virtual ~Tree() {  }

			/**
			 * Assigns all elements of t; that is, it replaces all existing
			 * elements with copies of the elements of t.
			 */
			Tree&
			assign(const Tree& t) { return assign(t.begin(), t.end()); }

			/**
			 * @overload
			 */
			template<typename InputIterator>
			Tree&
			assign(InputIterator begin, InputIterator end) {
				_elements.assign(begin, end);
				return *this;
			}

			/**
			 * Shorthand for assign().
			 */
			Tree&
			operator=(const Tree& t) { return assign(t); }

			/**
			 * Return an iterator for the beginning of the container (the
			 * position of the first element).  It is equivalent to end() if 
			 * the container is empty.
			 */
			iterator
			begin() { return _elements.begin(); }

			/**
			 * As above.
			 */
			const_iterator
			begin() const { return _elements.begin(); }

			/**
			 * Return an iterator for the end of the container (the
			 * position @em after the last element).  It is equivalent
			 * to begin() if the container is empty.
			 */
			iterator
			end() { return _elements.end(); }

			/**
			 * As above.
			 */
			const_iterator
			end() const { return _elements.end(); }

			/**
			 * Return if the container has no elements.
			 * Equivalent to <tt>size()==0</tt>.
			 */
			bool
			empty() const { return _elements.empty(); }

			/**
			 * Return the actual number of elements.
			 */
			size_type
			size() const { return _elements.size(); }

			/**
			 * Insert a copy of @a elem and return the position of the
			 * new element.
			 */
			iterator
			insert(const_reference elem) { return _elements.insert(elem); }

			/**
			 * Insert a copy of @a elem, using @a pos as a hint, and
			 * return the position of the new element.
			 */
			iterator
			insert(iterator pos, const_reference elem) {
				return _elements.insert(pos, elem);
			}
			
			/**
			 * Insert a copy of all elements in the range [begin, end).
			 */
			template<typename InputIterator>
			void
			insert(InputIterator begin, InputIterator end) { 
				return _elements.insert(pos, begin, end);
			}
			
			/**
			 * Remove all elements with the value @a elem and return
			 * the number of removed elements.
			 */
			size_type
			erase(const_reference elem) { 
				return _elements.erase(elem);
			}

			/**
			 * Remove the element at iterator position @a pos.
			 */
			void
			erase(iterator pos) { _elements.erase(pos); }

			/**
			 * Remove all of the elements (make the container empty).
			 */
			void
			clear() { _elements.clear(); }
			
			/**
			 * Return the position of the first element with value
			 * @a elem or end() if not found.
			 */
			iterator
			find(const_reference elem) { return _elements.find(elem); }

			/**
			 * As above.
			 */
			const_iterator
			find(const_reference elem) const { 
				return _elements.find(elem); 
			}

			/**
			 * Calls Accept on each of the elements in the container.
			 * @role ConcreteElement::Accept(Visitor) in the Visitor
			 *   design pattern (p331).
			 */
			void
			Accept(Visitor& visitor) {
				for (iterator iter = begin(); iter != end(); ++iter)
					iter->Accept(visitor);
			}
			
			/**
			 * As above.
			 */
			void
			Accept(Visitor& visitor) const {
				for (iterator iter = begin(); iter != end(); ++iter)
					iter->Accept(visitor);
			}
			
		private:
			/**
			 * The elements being contained.
			 */
			container_t _elements;
	};
}

#endif  // _GPLATES_GEO_TREE_H_
