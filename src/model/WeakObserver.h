/* $Id$ */

/**
 * \file 
 * Contains the definition of the class WeakObserver.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_WEAKOBSERVER_H
#define GPLATES_MODEL_WEAKOBSERVER_H

#include <algorithm>  /* std::swap */


namespace GPlatesModel
{
	// Forward declaration to avoid circularity of headers.
	template<class T> class WeakObserverVisitor;


	/**
	 * This class is the base of all weak observers of a publisher-type T.
	 *
	 * @par Weak observers
	 * The name "observer" is from the "Gang of Four" design pattern "Observer", which is also
	 * known as "Publisher-Subscriber".  Weak observers are so named because they "weakly"
	 * reference their observed publisher instances, neither incrementing nor decrementing the
	 * reference-count.
	 *
	 * @par
	 * Class WeakObserver serves as a common base for the classes RevisionAwareIterator,
	 * WeakReference and ReconstructedFeatureGeometry.
	 *
	 * @par
	 * For this class to function, it requires the definition of two non-member functions for
	 * each type-instantiation of this template.  The two functions are invoked by this class
	 * to access the list of observers of a given publisher instance, and must be defined
	 * within the same namespace as the class definition of the publisher.  This style of
	 * function mimics the functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release of
	 * the Boost intrusive_ptr smart pointer.
	 *
	 * @par
	 * Substituting T for the actual publisher type, the functions should have the prototypes:
	 *
	 * @code
	 * inline
	 * WeakObserver<T> *&
	 * weak_observer_get_last(
	 *                 T *publisher_ptr,
	 *                 const WeakObserver<T> *)
	 * @endcode
	 *
	 * @par
	 * and:
	 *
	 * @code
	 * inline
	 * WeakObserver<T> *&
	 * weak_observer_get_first(
	 *                 T *publisher_ptr,
	 *                 const WeakObserver<T> *)
	 * @endcode
	 *
	 * @par
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 *
	 * @par Exception safety
	 * Note that all manipulating operations of this class should involve builtin, nothrow
	 * operations only.
	 *
	 * @par Unsubscribing all weak observers
	 * To unsubscribe all the weak observers of a particular publisher instance, you should use
	 * the function @a weak_observer_unsubscribe_forward rather than rolling your own loop.
	 */
	template<typename T>
	class WeakObserver
	{
	public:
		/**
		 * This is a convenience typedef for this type.
		 */
		typedef WeakObserver<T> this_type;

		/**
		 * This is the type of the publisher.
		 */
		typedef T publisher_type;

		/**
		 * Default constructor.
		 *
		 * When creation is complete, this WeakObserver instance will not be subscribed to
		 * any publisher, and will not be part of any chain.  Thus, the publisher-pointer
		 * will be NULL; the "next link" and "prev link" pointers will not point to any
		 * other WeakObserver instances; and this instance will not be pointed-to by any
		 * other WeakObserver instances.
		 */
		WeakObserver():
			d_publisher_ptr(NULL),
			d_prev_link_ptr(NULL),
			d_next_link_ptr(NULL)
		{  }

		/**
		 * Constructor (note: not a copy-constructor).
		 *
		 * This newly-created WeakObserver instance will be subscribed to publisher
		 * @a publisher_.
		 */
		explicit
		WeakObserver(
				publisher_type &publisher_);

		/**
		 * Copy-constructor.
		 *
		 * This newly-created WeakObserver instance will be subscribed to the publisher to
		 * which @a other is already subscribed (if any).
		 */
		WeakObserver(
				const this_type &other);

		/**
		 * Virtual destructor.
		 *
		 * If this WeakObserver instance is subscribed to a publisher, it will be
		 * unsubscribed by this destructor.
		 *
		 * This destructor is virtual because the member function
		 * @a accept_weak_observer_visitor is virtual.
		 */
		virtual
		~WeakObserver();

		/**
		 * Return whether this WeakObserver instance is subscribed to a publisher.
		 */
		bool
		is_subscribed() const
		{
			return (d_publisher_ptr != NULL);
		}

		/**
		 * Return a pointer to the publisher-type.
		 *
		 * Note that we return a non-const instance of publisher_type from a const member
		 * function:  publisher_type may already be "const", in which case, the "const"
		 * would be redundant; OTOH, if publisher_type *doesn't* include "const", an
		 * instance of this class should behave like an STL iterator (or a pointer) rather
		 * than an STL const-iterator.  We're simply declaring the member function "const"
		 * to ensure that it may be invoked on const instances too.
		 */
		publisher_type *
		publisher_ptr() const
		{
			return d_publisher_ptr;
		}

		/**
		 * Return a pointer to the "next" weak observer instance in the chain.
		 *
		 * This member function is public because it is the only means available to the
		 * publisher of traversing its list of weak observers.
		 */
		this_type *
		next_link_ptr() const
		{
			return d_next_link_ptr;
		}

		/**
		 * Subscribe this WeakObserver instance to publisher @a publisher_.
		 *
		 * If this WeakObserver instance is @em already subscribed to a publisher, it will
		 * be unsubscribed from that publisher before being subscribed to @a publisher_.
		 */
		void
		subscribe(
				publisher_type &publisher_);

		/**
		 * Unsubscribe this WeakObserver instance from the publisher to which it is
		 * subscribed (if any).
		 *
		 * After this operation, the publisher-pointer will be NULL.
		 */
		void
		unsubscribe();

		/**
		 * Accept a WeakObserverVisitor instance.
		 *
		 * Note that this is called 'accept_weak_observer_visitor', rather than the usual
		 * 'accept_visitor', because there is already an 'accept_visitor' member function
		 * in ReconstructedFeatureGeometry (which derives from both ReconstructionGeometry
		 * and WeakObserver).  The existing 'accept_visitor' member function, which takes a
		 * ReconstructionGeometryVisitor as a parameter, would hide this member function if
		 * it had the same name.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				WeakObserverVisitor<T> &visitor) = 0;

	protected:
		/**
		 * Copy-assign the value of @a other to this instance.
		 *
		 * The effect of this operation is that this instance will be subscribed to the
		 * publisher to which @a other is subscribed (if any).
		 */
		WeakObserver<T> &
		operator=(
				const this_type &other);

		/**
		 * Swap the value of this instance with the value of @a other.
		 *
		 * The effect of this operation is that this instance will be subscribed to the
		 * publisher to which @a other was subscribed (if any), and @a other will be
		 * subscribed to the publisher to which this instance was subscribed (if any).
		 */
		void
		swap(
				this_type &other);

		/**
		 * Remove this WeakObserver from the list of subscribers to the publisher.
		 *
		 * When this function has completed, the "previous" WeakObserver instance (if any)
		 * in the chain will be connected to the "next" WeakObserver instance (if any) in
		 * the chain; the "prev link" and "next link" pointers of this instance will both
		 * be NULL.
		 */
		void
		remove_from_subscriber_list_of_publisher(
				publisher_type &publisher_);

		/**
		 * Access the "prev link" pointer which points to the previous weak observer
		 * instance in the chain.
		 */
		this_type *&
		prev_link_ptr()
		{
			return d_prev_link_ptr;
		}

	private:
		/**
		 * If non-NULL, this points to the publisher instance to which this WeakObserver
		 * instance is subscribed.
		 */
		publisher_type *d_publisher_ptr;

		/**
		 * This points to the previous link in the doubly-linked list of weak observers of
		 * a particular publisher instance.
		 *
		 * The weak observers themselves are the links in the list, so that they can be
		 * subscribed and unsubscribed to the publisher using only nothrow list-splicing
		 * operations.
		 *
		 * Note that no ordering of the elements in the list is guaranteed:  Elements may
		 * be moved around arbitrarily to facilitate more complex operations.
		 */
		this_type *d_prev_link_ptr;

		/**
		 * This points to the next link in the doubly-linked list of weak observers of a
		 * particular publisher instance.
		 *
		 * The weak observers themselves are the links in the list, so that they can be
		 * subscribed and unsubscribed to the publisher using only nothrow list-splicing
		 * operations.
		 *
		 * Note that no ordering of the elements in the list is guaranteed:  Elements may
		 * be moved around arbitrarily to facilitate more complex operations.
		 */
		this_type *d_next_link_ptr;

		/**
		 * Subscribe this weak observer to @a publisher_.
		 *
		 * This function should be invoked in the general case that it is not known whether
		 * @a publisher_ has any other subscribers or not.
		 *
		 * It is a pre-condition of this function that this WeakObserver instance is not
		 * subscribed to any publisher.
		 */
		void
		subscribe_to_publisher_unknown_whether_other_subscribers(
				publisher_type &publisher_);

		/**
		 * Subscribe this weak observer to the publisher to which @a other is subscribed.
		 *
		 * This function is able to be slightly more efficient than the function
		 * @a subscribe_to_publisher_unknown_whether_other_subscribers, since it knows that the
		 * publisher must already have at least one other subscriber, namely @a other.
		 *
		 * It is a pre-condition of this function that this WeakObserver instance is not
		 * subscribed to any publisher.
		 */
		void
		subscribe_to_same_publisher_as_other_observer(
				const this_type &other);

	};


	template<typename T>
	inline
	WeakObserver<T>::WeakObserver(
			publisher_type &publisher_):
		d_publisher_ptr(NULL),
		d_prev_link_ptr(NULL),
		d_next_link_ptr(NULL)
	{
		subscribe_to_publisher_unknown_whether_other_subscribers(publisher_);
	}


	template<typename T>
	inline
	WeakObserver<T>::WeakObserver(
			const WeakObserver<T> &other):
		d_publisher_ptr(NULL),
		d_prev_link_ptr(NULL),
		d_next_link_ptr(NULL)
	{
		subscribe_to_same_publisher_as_other_observer(other);
	}


	template<typename T>
	WeakObserver<T>::~WeakObserver()
	{
		if (is_subscribed()) {
			remove_from_subscriber_list_of_publisher(*d_publisher_ptr);
		}
	}


	template<typename T>
	inline
	void
	WeakObserver<T>::subscribe(
			publisher_type &publisher_)
	{
		if (is_subscribed()) {
			remove_from_subscriber_list_of_publisher(*d_publisher_ptr);
		}
		subscribe_to_publisher_unknown_whether_other_subscribers(publisher_);
	}


	template<typename T>
	void
	WeakObserver<T>::unsubscribe()
	{
		if (is_subscribed()) {
			remove_from_subscriber_list_of_publisher(*d_publisher_ptr);
			d_publisher_ptr = NULL;
		}
	}


	template<typename T>
	WeakObserver<T> &
	WeakObserver<T>::operator=(
			const WeakObserver<T> &other)
	{
		if (&other != this) {
			// This instance must unsubscribe itself from its current publisher, and
			// subscribe to the publisher observed by 'other'.
			if (is_subscribed()) {
				remove_from_subscriber_list_of_publisher(*d_publisher_ptr);
			}
			subscribe_to_same_publisher_as_other_observer(other);
		}
		return *this;
	}


	template<typename T>
	inline
	void
	WeakObserver<T>::swap(
			WeakObserver<T> &other)
	{
		// We're OK with this copy-construct, copy-assign, copy-assign implementation of
		// swap, since all of these operations will consist of builtin operations only.
		WeakObserver<T> tmp(*this);
		*this = other;
		other = tmp;
	}


	template<typename T>
	void
	WeakObserver<T>::remove_from_subscriber_list_of_publisher(
			publisher_type &publisher_)
	{
		if (d_prev_link_ptr != NULL) {
			// Tell the previous link to skip over this instance and point to the next
			// instance (if any).
			d_prev_link_ptr->d_next_link_ptr = d_next_link_ptr;
		} else {
			// Since there was no previous link, this instance must be the first weak
			// observer of the publisher.  So, tell the publisher to skip over this
			// instance and point to the next instance (if any).
			weak_observer_get_first(&publisher_, this) = d_next_link_ptr;
		}
		if (d_next_link_ptr != NULL) {
			// Tell the next link to skip over this instance and point to the previous
			// instance (if any).
			d_next_link_ptr->d_prev_link_ptr = d_prev_link_ptr;
		} else {
			// Since there was no next link, this instance must be the last weak
			// observer of the publisher.  So, tell the publisher to skip over this
			// instance and point to the previous instance (if any).
			weak_observer_get_last(&publisher_, this) = d_prev_link_ptr;
		}

		d_prev_link_ptr = NULL;
		d_next_link_ptr = NULL;
	}


	template<typename T>
	void
	WeakObserver<T>::subscribe_to_publisher_unknown_whether_other_subscribers(
			publisher_type &publisher_)
	{
		d_publisher_ptr = &publisher_;
		prev_link_ptr() = weak_observer_get_last(d_publisher_ptr, this);

		// We don't know whether the value of 'publisher_.last_weak_observer()', which has
		// been assigned to 'd_prev_link_ptr', is NULL or not.  (It will be NULL if there
		// were no other weak observers of this publisher; non-NULL otherwise.)
		if (prev_link_ptr() != NULL) {
			// Tell the previous link, which was previously the last observer of the
			// instance of T, to point to this instance as its next.
			d_prev_link_ptr->d_next_link_ptr = this;
		} else {
			// Since there was no "last weak observer" of the instance of T, this
			// instance must be the first.
			weak_observer_get_first(d_publisher_ptr, this) = this;
		}
		weak_observer_get_last(d_publisher_ptr, this) = this;
	}


	template<typename T>
	void
	WeakObserver<T>::subscribe_to_same_publisher_as_other_observer(
			const WeakObserver<T> &other)
	{
		d_publisher_ptr = other.d_publisher_ptr;
		if (d_publisher_ptr != NULL) {
			// 'other' was subscribed to a publisher.  Let's subscribe also.
			prev_link_ptr() = weak_observer_get_last(d_publisher_ptr, this);

			// We know that 'd_prev_link_ptr' cannot be NULL, since 'other' is already
			// subscribed to the publisher, so there must be at least one subscriber
			// before this instance.  Tell the previous link, which was previously the
			// last observer of the instance of T, to point to this instance as its
			// next.
			d_prev_link_ptr->d_next_link_ptr = this;
			weak_observer_get_last(d_publisher_ptr, this) = this;
		}
	}


	/**
	 * Unsubscribe all weak observers from @a curr onwards (inclusive).
	 *
	 * This is the function you should use to unsubscribe all the weak observers of a publisher
	 * instance (rather than rolling your own loop).  (This function was created after the same
	 * error was found in several loops.)
	 *
	 * Note that @a curr is allowed to be NULL; this function will behave correctly.
	 */
	template<typename WeakObserverType>
	inline
	void
	weak_observer_unsubscribe_forward(
			WeakObserverType *curr)
	{
		for (WeakObserverType *next; curr != NULL; curr = next) {
			// Note that we need to grab a pointer to the "next" link NOW, *before* we
			// unsubscribe the current link from its publisher, because unsubscribing
			// a link will set its "next" and "prev" links to NULL.
			//
			// (Since this loop iterates until a NULL "next"-link pointer is
			// encountered, if we didn't grab the "next"-link pointer in advance, the
			// loop would only unsubscribe the first weak observer.)
			next = curr->next_link_ptr();
			curr->unsubscribe();
		}
	}
}


#if 0  // This function would be more useful for derivations of WeakObserver
namespace std
{
	/**
	 * This is a template specialisation of the standard function @a swap.
	 *
	 * See Josuttis, section 4.4.2, "Swapping Two Values" for more information.
	 */
	template<typename T>
	inline
	void
	swap(
			GPlatesModel::WeakObserver<T> &w1,
			GPlatesModel::WeakObserver<T> &w2)
	{
		w1.swap(w2);
	}
}
#endif

#endif  // GPLATES_MODEL_WEAKOBSERVER_H
