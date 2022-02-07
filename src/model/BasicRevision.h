/* $Id$ */

/**
 * \file 
 * Contains the definition of the class BasicRevision.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
 * (also incorporating code from "FeatureRevision.h", "FeatureRevision.cc", "FeatureCollectionRevision.h",
 * "FeatureCollectionRevision.cc", "FeatureStoreRootRevision.h" and "FeatureStoreRootRevision.cc")
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

#ifndef GPLATES_MODEL_BASICREVISION_H
#define GPLATES_MODEL_BASICREVISION_H

#include <algorithm>
#include <functional>
#include <vector>
#include <boost/function.hpp>
#include <boost/intrusive_ptr.hpp>

#include "HandleTraits.h"
#include "types.h"

#include "global/PointerTraits.h"

namespace GPlatesModel
{
	namespace BasicRevisionInternals
	{
		// Adapter functor that wraps around a child_predicate_type to make it
		// work with intrusive_ptr not just non_null_intrusive_ptr.
		template<class PredicateType, class ChildType>
		class ChildPredicateAdapter
		{
		public:
			typedef boost::intrusive_ptr<const ChildType> argument_type;

			ChildPredicateAdapter(
					const PredicateType &predicate) :
				d_predicate(predicate)
			{
			}

			bool
			operator()(
					const boost::intrusive_ptr<const ChildType> &child_ptr) const
			{
				if (child_ptr)
				{
					return d_predicate(
							GPlatesUtils::non_null_intrusive_ptr<const ChildType>(child_ptr.get()));
				}
				else
				{
					return false;
				}
			}

		private:
			PredicateType d_predicate;
		};
	}

	/**
	 * BasicRevision contains functionality common to all Revision classes. This
	 * common functionality is brought into the Revision classes by way of
	 * inheritance. For example, FeatureRevision is derived from
	 * BasicRevision<FeatureHandle>. (Although delegation is usually preferred to
	 * inheritance, the use of inheritance in this case significantly simplifies
	 * the Revision class interfaces.)
	 */
	template<class HandleType>
	class BasicRevision
	{

	public:

		/**
		 * Typedef of the template parameter.
		 */
		typedef HandleType handle_type;

		/**
		 * The type of this class.
		 */
		typedef BasicRevision<handle_type> this_type;

		/**
		 * The revision type associated with the handle type.
		 */
		typedef typename HandleTraits<handle_type>::revision_type revision_type;

		/**
		 * The type of this type's child.
		 */
		typedef typename HandleTraits<handle_type>::child_type child_type;

		/**
		 * The type used to represent the collection of children of this revision.
		 */
		typedef std::vector<boost::intrusive_ptr<child_type> > collection_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<FeatureRevision>.
		 */
		typedef typename GPlatesGlobal::PointerTraits<revision_type>::non_null_ptr_type non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const FeatureRevision>.
		 */
		typedef typename GPlatesGlobal::PointerTraits<const revision_type>::non_null_ptr_type non_null_ptr_to_const_type;

		/**
		 * Destructor.
		 */
		virtual
		~BasicRevision();

		/**
		 * Returns the number of children-slots currently contained within this revision.
		 *
		 * Note that children-slots may be empty (ie, the pointer at that
		 * position may be NULL).  Thus, the number of children actually
		 * contained within this revision may be less than the number of
		 * children-slots.
		 * 
		 * This value is intended to be used as an upper (open range) limit on the values
		 * of the index used to access the children within this revision. 
		 * Attempting to access a child  at an index which is greater-than or
		 * equal-to the number of children-slots will always result in a NULL
		 * pointer.
		 */
		container_size_type
		container_size() const;

		/**
		 * Returns the number of children currently contained within this revision.
		 *
		 * This number does not include empty children-slots (as container_size() returns).
		 * Instead, this value represents the logical number of children in this
		 * container at this point in time.
		 */
		container_size_type
		size() const;

		/**
		 * Accesses the child at @a index in the collection. This is not revision-aware.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that children-slot is still being used or not).
		 *
		 * This is the overloading of this function for const Revision instances; it
		 * returns a pointer to a const child_type instance.
		 */
		const boost::intrusive_ptr<const child_type>
		operator[](
				container_size_type index) const;

		/**
		 * Accesses the child at @a index in the collection. This is not revision-aware.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that children-slot is still being used or not).
		 *
		 * This is the overloading of this function for non-const Revision
		 * instances; it returns a pointer to a non-const child_type instance.
		 */
		const boost::intrusive_ptr<child_type>
		operator[](
				container_size_type index);

		/**
		 * Accesses the child at @a index in the collection. This is not revision-aware.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that child-slot is still being used or not).
		 *
		 * This is the overloading of this function for const Revision instances; it
		 * returns a pointer to a const child_type instance.
		 */
		const boost::intrusive_ptr<const child_type>
		get(
				container_size_type index) const;

		/**
		 * Accesses the child at @a index in the collection. This is not revision-aware.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that child-slot is still being used or not).
		 *
		 * This is the overloading of this function for non-const Revision
		 * instances; it returns a pointer to a non-const child_type instance.
		 */
		const boost::intrusive_ptr<child_type>
		get(
				container_size_type index);

		/**
		 * Returns true if there is an element at position @a index in the underlying container.
		 */
		bool
		has_element_at(
				container_size_type index) const;

		/**
		 * Adds @a new_child to the collection.
		 * 
		 * Returns the index of @a new_child in the collection.
		 */
		container_size_type
		add(
				typename GPlatesGlobal::PointerTraits<child_type>::non_null_ptr_type new_child);

		/**
		 * Removes and returns the child at @a index in the collection.
		 *
		 * The value of @a index is expected to be valid (non-negative, less than @a size()).
		 */
		boost::intrusive_ptr<child_type>
		remove(
				container_size_type index);

		/**
		 * Changes a child at a particular @a index into @a new_child.
		 */
		void
		set(
				container_size_type index,
				typename GPlatesGlobal::PointerTraits<child_type>::non_null_ptr_type new_child);

	protected:

		/**
		 * Typedef for a function that accepts a pointer to a child_type and returns a boolean.
		 */
		typedef boost::function<bool (const typename GPlatesGlobal::PointerTraits<const child_type>::non_null_ptr_type &)>
			child_predicate_type;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		BasicRevision();

		/**
		 * This constructor should not be public, because we don't want to be allow
		 * instantiation of this type on the stack.
		 */
		BasicRevision(
				const this_type &other);

		/*
		 * This constructor should not be public, because we don't want to be allow
		 * instantiation of this type on the stack.
		 *
		 * This copy constructor (shallow) copies those children in @a other for
		 * which the predicate @a clone_children_predicate returns true.
		 */
		BasicRevision(
				const this_type &other,
				const child_predicate_type &clone_children_predicate);

	private:

		/**
		 * This should not be defined, because we don't want to be able to copy
		 * one of these objects.
		 */
		this_type &
		operator=(
				const this_type &);

		/*
		 * The collection of children possessed by this Revision.
		 *
		 * Note that any of the pointers contained as elements in this vector can be NULL. 
		 *
		 * An element which is a NULL pointer indicates that the property, which was
		 * referenced by that element, has been deleted.  The element is set to a NULL
		 * pointer rather than removed, so that the indices, which are used to reference
		 * the other elements in the vector, remain valid.
		 */
		collection_type d_children;

		/**
		 * The number of current children (i.e. the number of non-null slots in d_children).
		 */
		container_size_type d_num_children;

	};


	template<class HandleType>
	BasicRevision<HandleType>::~BasicRevision()
	{
	}


	template<class HandleType>
	container_size_type
	BasicRevision<HandleType>::container_size() const
	{
		return d_children.size();
	}


	template<class HandleType>
	container_size_type
	BasicRevision<HandleType>::size() const
	{
		return d_num_children;
	}


	template<class HandleType>
	const boost::intrusive_ptr<const typename BasicRevision<HandleType>::child_type>
	BasicRevision<HandleType>::operator[](
			container_size_type index) const
	{
		return get(index);
	}


	template<class HandleType>
	const boost::intrusive_ptr<typename BasicRevision<HandleType>::child_type>
	BasicRevision<HandleType>::operator[](
			container_size_type index)
	{
		return get(index);
	}


	template<class HandleType>
	const boost::intrusive_ptr<const typename BasicRevision<HandleType>::child_type>
	BasicRevision<HandleType>::get(
			container_size_type index) const
	{
		if (index < d_children.size())
		{
			return d_children[index];
		}
		else
		{
			return NULL;
		}
	}


	template<class HandleType>
	const boost::intrusive_ptr<typename BasicRevision<HandleType>::child_type>
	BasicRevision<HandleType>::get(
			container_size_type index)
	{
		if (index < d_children.size())
		{
			return d_children[index];
		}
		else
		{
			return NULL;
		}
	}


	template<class HandleType>
	bool
	BasicRevision<HandleType>::has_element_at(
			container_size_type index) const
	{
		if (index < d_children.size())
		{
			return d_children[index];
		}
		else
		{
			return false;
		}
	}


	template<class HandleType>
	container_size_type
	BasicRevision<HandleType>::add(
			typename GPlatesGlobal::PointerTraits<child_type>::non_null_ptr_type new_child)
	{
		d_children.push_back(new_child.get());
		++d_num_children;
		return d_children.size() - 1;
	}


	template<class HandleType>
	boost::intrusive_ptr<typename BasicRevision<HandleType>::child_type>
	BasicRevision<HandleType>::remove(
			container_size_type index)
	{
		boost::intrusive_ptr<child_type> result = d_children[index];

		if (result)
		{
			// Remove our pointer to the child.
			d_children[index] = NULL;
			--d_num_children;
		}

		return result;
	}

	
	template<class HandleType>
	void
	BasicRevision<HandleType>::set(
			container_size_type index,
			typename GPlatesGlobal::PointerTraits<child_type>::non_null_ptr_type new_child)
	{
		if (!d_children[index])
		{
			// There isn't a child at index at the moment, so this set() operation
			// actually increases the number of children by one.
			++d_num_children;
		}

		d_children[index] = new_child.get();
	}


	template<class HandleType>
	BasicRevision<HandleType>::BasicRevision() :
		d_num_children(0)
	{
	}


	template<class HandleType>
	BasicRevision<HandleType>::BasicRevision(
			const this_type &other) :
		d_children(other.d_children),
		d_num_children(other.d_num_children)
	{
	}


	template<class HandleType>
	BasicRevision<HandleType>::BasicRevision(
			const this_type &other,
			const child_predicate_type &clone_children_predicate)
	{
		using namespace std;
		typedef BasicRevisionInternals::ChildPredicateAdapter<child_predicate_type, child_type> adapter_type;
		remove_copy_if(
				other.d_children.begin(),
				other.d_children.end(),
				back_inserter(d_children),
				not1(adapter_type(clone_children_predicate)));

		// The use of ChildPredicateAdapter has a side-effect in that our d_children
		// container contains no NULL elements, which is rather convenient.
		d_num_children = d_children.size();
	}

}


#endif  // GPLATES_MODEL_REVISION_H
