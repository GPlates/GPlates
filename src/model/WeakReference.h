/* $Id$ */

/**
 * \file 
 * Contains the definition of the class WeakReference.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_WEAKREFERENCE_H
#define GPLATES_MODEL_WEAKREFERENCE_H

#include "WeakObserver.h"


namespace GPlatesModel
{
	/**
	 * This class is a weak reference to a handle.
	 *
	 * To quote the article on weak references in Wikipedia, the free encyclopedia:
	 *
	 * @par
	 * In computer programming, a weak reference is a reference that does not protect the
	 * referent object from collection by a garbage collector. An object referenced only by
	 * weak references is considered unreachable (or "weakly reachable") and so may be
	 * collected at any time. Weak references are used to prevent circular references and to
	 * avoid keeping in memory referenced but unneeded objects.
	 *
	 * To quote the section Weak References on the home-page for mxProxy: a Generic Proxy
	 * Wrapper Type for Python:
	 *
	 * @par
	 * Weak references are called weak because they don't keep the object alive by incrementing
	 * the reference count on the referenced object.
	 *
	 * GPlates uses "weak references" as "smart pointers" which enable objects to be referenced
	 * without keeping the objects alive. Specifically, weak references are used to implement
	 * references in the Presenter to objects inside the Model. 
	 */
	template<typename H, typename ConstH>
	class WeakReference:
			public WeakObserver<H, ConstH>
	{
	public:
		/**
		 * This is the type of the handle.
		 *
		 * (For example, 'FeatureCollectionHandle' or 'const FeatureCollectionHandle'.)
		 */
		typedef H handle_type;

		/**
		 * This is the const-type of the handle.
		 *
		 * (For example, 'const FeatureCollectionHandle'.)
		 */
		typedef ConstH const_handle_type;

		/**
		 * Default constructor.
		 *
		 * Weak reference instances which are initialised using the default constructor are
		 * not valid to be dereferenced.
		 *
		 * The pointer to the handle will be NULL.
		 */
		WeakReference()
		{  }

		/**
		 * Construct a reference to @a handle.
		 *
		 * This constructor will not throw.
		 *
		 * The pointer to the handle will be non-NULL, and the instance will be valid for
		 * dereferencing.
		 */
		explicit
		WeakReference(
				handle_type &handle):
			WeakObserver<H, ConstH>(handle)
		{  }

		/**
		 * Return the pointer to the handle.
		 *
		 * The pointer returned will be NULL if this instance does not reference a handle;
		 * non-NULL otherwise.
		 *
		 * This function will not throw.
		 *
		 * Note that we return a non-const instance of handle_type from a const member
		 * function:  handle_type may already be "const", in which case, the "const" would
		 * be redundant; OTOH, if handle_type *doesn't* include "const", an instance of
		 * this class should behave like an STL iterator (or a pointer) rather than an STL
		 * const-iterator.  We're simply declaring the member function "const" to ensure
		 * that it may be invoked on const instances too.
		 */
		handle_type *
		handle_ptr() const
		{
			return WeakObserver<H, ConstH>::publisher_ptr();
		}

		/**
		 * Return whether this iterator is valid to be dereferenced.
		 *
		 * This function will not throw.
		 */
		bool
		is_valid() const
		{
			return (handle_ptr() != NULL);
		}

		/**
		 * Copy-assignment operator.
		 *
		 * The effect of this operation is that this instance will reference the handle
		 * which is referenced by @a other (if any).
		 *
		 * This function will not throw.
		 */
		WeakReference &
		operator=(
				const WeakReference &other)
		{
			WeakObserver<H, ConstH>::operator=(other);
			return *this;
		}

		/**
		 * Return whether this instance is equal to @a other.
		 *
		 * This instance will be considered equal to @a other if both instances reference
		 * the same handle, or if neither instance references any handle; the instances
		 * will be considered unequal otherwise.
		 *
		 * This function will not throw.
		 */
		bool
		operator==(
				const WeakReference &other) const
		{
			return (handle_ptr() == other.handle_ptr());
		}

		/**
		 * Return whether this instance is not equal to @a other.
		 *
		 * This instance will be considered equal to @a other if both instances reference
		 * the same handle, or if neither instance references any handle; the instances
		 * will be considered unequal otherwise.
		 *
		 * This function will not throw.
		 */
		bool
		operator!=(
				const WeakReference &other) const
		{
			return (handle_ptr() != other.handle_ptr());
		}

		/**
		 * The dereference operator.
		 *
		 * This operator should only be invoked when the reference is valid (i.e., when
		 * @a is_valid would return true).
		 *
		 * Note that we return a non-const instance of handle_type from a const member
		 * function:  handle_type may already be "const", in which case, the "const" would
		 * be redundant; OTOH, if handle_type *doesn't* include "const", an instance of
		 * this class should behave like an STL iterator (or a pointer) rather than an STL
		 * const-iterator.  We're simply declaring the member function "const" to ensure
		 * that it may be invoked on const instances too.
		 */
		handle_type &
		operator*() const
		{
			return *handle_ptr();
		}

		/**
		 * The pointer-indirection-member-access operator.
		 *
		 * This operator should only be invoked when the reference is valid (i.e., when
		 * @a is_valid would return true).
		 *
		 * Note that we return a non-const instance of handle_type from a const member
		 * function:  handle_type may already be "const", in which case, the "const" would
		 * be redundant; OTOH, if handle_type *doesn't* include "const", an instance of
		 * this class should behave like an STL iterator (or a pointer) rather than an STL
		 * const-iterator.  We're simply declaring the member function "const" to ensure
		 * that it may be invoked on const instances too.
		 */
		handle_type *
		operator->() const
		{
			return handle_ptr();
		}
	};

}

#endif  // GPLATES_MODEL_WEAKREFERENCE_H
