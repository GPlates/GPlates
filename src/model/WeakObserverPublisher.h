/* $Id$ */

/**
 * \file 
 * Contains the definition of the class WeakObserverPublisher.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_MODEL_WEAKOBSERVERPUBLISHER_H
#define GPLATES_MODEL_WEAKOBSERVERPUBLISHER_H

#include "WeakObserverVisitor.h"

namespace GPlatesModel
{
	/**
	 * A WeakObserverPublisher corresponds to the publisher component of the
	 * observer design pattern. The observers are linked together in a linked list
	 * and the publisher holds a pointer to the first and last observers.
	 *
	 * There are separate lists for const and non-const weak observers.
	 */
	template<class H>
	class WeakObserverPublisher
	{

	public:

		/**
		 * The base type of all const weak observers of instances of this class.
		 */
		typedef WeakObserver<const H> const_weak_observer_type;

		/**
		 * The base type of all (non-const) weak observers of instances of this class.
		 */
		typedef WeakObserver<H> weak_observer_type;

		/**
		 * Constructor.
		 */
		WeakObserverPublisher();

		/**
		 * Destructor.
		 *
		 * Unsubscribes all weak observers.
		 */
		~WeakObserverPublisher();

		/**
		 * Apply the supplied WeakObserverVisitor to all weak observers of this instance.
		 */
		void
		apply_weak_observer_visitor(
				WeakObserverVisitor<H> &visitor);

		/**
		 * Access the first const weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		const_weak_observer_type *&
		first_const_weak_observer() const;

		/**
		 * Access the first weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		weak_observer_type *&
		first_weak_observer();

		/**
		 * Access the last const weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		const_weak_observer_type *&
		last_const_weak_observer() const;

		/**
		 * Access the last weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		weak_observer_type *&
		last_weak_observer();

	private:

		/**
		 * The first const weak observer of this instance.
		 */
		mutable const_weak_observer_type *d_first_const_weak_observer;

		/**
		 * The first weak observer of this instance.
		 */
		mutable weak_observer_type *d_first_weak_observer;

		/**
		 * The last const weak observer of this instance.
		 */
		mutable const_weak_observer_type *d_last_const_weak_observer;

		/**
		 * The last weak observer of this instance.
		 */
		mutable weak_observer_type *d_last_weak_observer;

	};

	template<class H>
	WeakObserverPublisher<H>::WeakObserverPublisher() :
		d_first_const_weak_observer(NULL),
		d_first_weak_observer(NULL),
		d_last_const_weak_observer(NULL),
		d_last_weak_observer(NULL)
	{
	}

	template<class H>
	WeakObserverPublisher<H>::~WeakObserverPublisher()
	{
		weak_observer_unsubscribe_forward(d_first_const_weak_observer);
		weak_observer_unsubscribe_forward(d_first_weak_observer);
	}

	template<class H>
	void
	WeakObserverPublisher<H>::apply_weak_observer_visitor(
			WeakObserverVisitor<H> &visitor)
	{
		weak_observer_type *curr = first_weak_observer();
		while (curr != NULL)
		{
			curr->accept_weak_observer_visitor(visitor);
			curr = curr->next_link_ptr();
		}
	}

	template<class H>
	typename WeakObserverPublisher<H>::const_weak_observer_type *&
	WeakObserverPublisher<H>::first_const_weak_observer() const
	{
		return d_first_const_weak_observer;
	}

	template<class H>
	typename WeakObserverPublisher<H>::weak_observer_type *&
	WeakObserverPublisher<H>::first_weak_observer()
	{
		return d_first_weak_observer;
	}

	template<class H>
	typename WeakObserverPublisher<H>::const_weak_observer_type *&
	WeakObserverPublisher<H>::last_const_weak_observer() const
	{
		return d_last_const_weak_observer;
	}

	template<class H>
	typename WeakObserverPublisher<H>::weak_observer_type *&
	WeakObserverPublisher<H>::last_weak_observer()
	{
		return d_last_weak_observer;
	}

	/**
	 * Get the first weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This function mimics the Boost
	 * intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	template<class H>
	WeakObserver<const H> *&
	weak_observer_get_first(
			const H *publisher_ptr,
			const WeakObserver<const H> *)
	{
		return publisher_ptr->first_const_weak_observer();
	}

	/**
	 * Get the last weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This style of function mimics the
	 * Boost intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	template<class H>
	WeakObserver<const H> *&
	weak_observer_get_last(
			const H *publisher_ptr,
			const WeakObserver<const H> *)
	{
		return publisher_ptr->last_const_weak_observer();
	}


	/**
	 * Get the first weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This function mimics the Boost
	 * intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	template<class H>
	WeakObserver<H> *&
	weak_observer_get_first(
			H *publisher_ptr,
			const WeakObserver<H> *)
	{
		return publisher_ptr->first_weak_observer();
	}


	/**
	 * Get the last weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This style of function mimics the
	 * Boost intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	template<class H>
	WeakObserver<H> *&
	weak_observer_get_last(
			H *publisher_ptr,
			const WeakObserver<H> *)
	{
		return publisher_ptr->last_weak_observer();
	}

}

#endif  // GPLATES_MODEL_WEAKOBSERVERPUBLISHER_H
