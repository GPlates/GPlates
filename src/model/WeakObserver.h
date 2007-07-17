/* $Id$ */

/**
 * \file 
 * Contains the definition of the class WeakObserver.
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

#ifndef GPLATES_MODEL_WEAKOBSERVER_H
#define GPLATES_MODEL_WEAKOBSERVER_H

#include <algorithm>  /* std::swap */


namespace GPlatesModel
{
	/**
	 * This class is the base of all weak observers of a particular publisher-type T (ignoring
	 * the constness of T).
	 *
	 * @par Weak observers
	 * The name "observer" is from the "Gang of Four" design pattern "Observer", which is also
	 * known as "Publisher-Subscriber".  Weak observers are so named because they "weakly"
	 * reference their observed publisher instances, neither incrementing nor decrementing the
	 * reference-count.
	 *
	 * @par
	 * Class WeakObserver serves as a common base for both class HandleContainerIterator and
	 * class WeakReference.
	 *
	 * @par Why class WeakObserverBase?
	 * This base class is necessary so that, for example, an instance of weak observer of
	 * FeatureHandle can have pointers which point to either weak observer of FeatureHandle or
	 * weak observer of const FeatureHandle; and similarly, an instance of weak observer of
	 * @em const FeatureHandle can have pointers which point to either weak observer of
	 * FeatureHandle or weak observer of const FeatureHandle.
	 *
	 * @par
	 * Achieving this is not as straight-forward as it might seem:  A template instantiation
	 * 'X@<T@>' is considered by the compiler to be completely unrelated to 'X@<const T@>', so
	 * it's not possible (without using base classes or unions) to have a pointer which can
	 * point to both types.  The most obvious "solution", simply casting a pointer of one type
	 * to a pointer of the other type, is not an appropriate solution, as it will discard
	 * information about what the @em actual type of the pointer is (which causes problems
	 * later on, when using the pointer), in addition to compromising const-correctness.
	 *
	 * @par
	 * Thus, as a solution, a weak observer is implemented in two parts: this base class
	 * (WeakObserverBase) and WeakObserver, which derives from this base class.
	 *
	 * @par
	 * This base class, which is templatised by type 'const T', contains pointers to other
	 * instances of this same base type, and knows nothing about the actual publisher-type (ie,
	 * whether the publisher-type is 'T' or 'const T').  The knowing of the specific
	 * publisher-type and the containing of pointers to the publisher will be performed by
	 * the WeakObserver derivations of this base class.
	 *
	 * @par Exception safety
	 * Note that all manipulating operations of this class should involve builtin, nothrow
	 * operations only.
	 */
	template<typename ConstT>
	class WeakObserverBase
	{
	public:
		/**
		 * This is a convenience typedef for this type.
		 */
		typedef WeakObserverBase<ConstT> this_type;

		/**
		 * This is the const-type of the publisher.
		 */
		typedef ConstT const_publisher_type;

		/**
		 * Default constructor.
		 *
		 * When a WeakObserverBase instance is created, it will not point to (nor be
		 * pointed-to by) any other WeakObserverBase instances.  (Although that may very
		 * well have changed by the end of the body of the WeakObserver constructor which
		 * will run after this.)
		 */
		WeakObserverBase():
			d_prev_link_ptr(NULL),
			d_next_link_ptr(NULL)
		{  }

		/**
		 * Virtual destructor.
		 *
		 * The WeakObserverBase constructor does nothing.
		 *
		 * This destructor is virtual because the member function @a unsubscribe is
		 * virtual.
		 */
		virtual
		~WeakObserverBase()
		{  }

		/**
		 * Return a pointer to the "next" weak observer instance in the chain.
		 */
		this_type *
		next_link_ptr() const
		{
			return d_next_link_ptr;
		}

		/**
		 * Unsubscribe this weak observer instance from its publisher.
		 *
		 * This function is declared here because, even though WeakObserverBase neither
		 * knows about the publisher nor contains a pointer to the publisher, the
		 * instruction to a weak observer to unsubscribe from its publisher will be
		 * directed to the links in the chain, which are only known to be WeakObserverBase
		 * instances by the client code.
		 *
		 * This function is defined as a pure virtual member function, so that it can be
		 * implemented in class WeakObserver, which will have knowledge of the publisher.
		 */
		virtual
		void
		unsubscribe() = 0;
	protected:
		/**
		 * Remove this weak observer from the list of subscribers to the publisher.
		 *
		 * When this function has completed, the "previous" weak observer instance (if any)
		 * in the chain will be connected to the "next" weak observer instance (if any) in
		 * the chain; the "prev link" and "next link" pointers of this instance will both
		 * be NULL.
		 */
		void
		remove_from_subscriber_list(
				const_publisher_type &publisher_);

		/**
		 * Access the "prev link" pointer which points to the previous weak observer
		 * instance in the chain.
		 */
		this_type *&
		prev_link_ptr()
		{
			return d_prev_link_ptr;
		}

		/**
		 * Set the "next link" pointer of the instance, which is referenced by the "prev
		 * link" pointer, to @a ptr.
		 *
		 * This function assumes that the "prev link" pointer is non-NULL.
		 */
		void
		set_next_link_ptr_of_prev_link(
				this_type *ptr)
		{
			d_prev_link_ptr->d_next_link_ptr = ptr;
		}
	private:
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
	};


	template<typename ConstT>
	void
	WeakObserverBase<ConstT>::remove_from_subscriber_list(
			const_publisher_type &publisher_)
	{
		if (d_prev_link_ptr != NULL) {
			// Tell the previous link to skip over this instance and point to the next
			// instance (if any).
			d_prev_link_ptr->d_next_link_ptr = d_next_link_ptr;
		} else {
			// Since there was no previous link, this instance must be the first weak
			// observer of the publisher.
			publisher_.first_weak_observer() = d_next_link_ptr;
		}
		if (d_next_link_ptr != NULL) {
			// Tell the next link to skip over this instance and point to the previous
			// instance (if any).
			d_next_link_ptr->d_prev_link_ptr = d_prev_link_ptr;
		} else {
			// Since there was no next link, this instance must be the last weak
			// observer of the publisher.
			publisher_.last_weak_observer() = d_prev_link_ptr;
		}

		d_prev_link_ptr = NULL;
		d_next_link_ptr = NULL;
	}


	/**
	 * This class is a weak observer of publisher-type T.
	 *
	 * This class serves as a common base for both class HandleContainerIterator and class
	 * WeakReference.
	 *
	 * @par Exception safety
	 * Note that all manipulating operations of this class should involve builtin, nothrow
	 * operations only.
	 */
	template<typename T, typename ConstT>
	class WeakObserver: public WeakObserverBase<ConstT>
	{
	public:
		/**
		 * This is the type of the publisher.
		 */
		typedef T publisher_type;

		/**
		 * This is a convenience typedef for this type.
		 */
		typedef WeakObserver<T, ConstT> this_type;

		/**
		 * Default constructor.
		 *
		 * This weak observer instance will not be subscribed to any publisher, and will
		 * not be part of any chain.
		 *
		 * The publisher-pointer will be NULL.
		 */
		WeakObserver():
			d_publisher_ptr(NULL)
		{  }

		/**
		 * Constructor.
		 *
		 * This newly-created instance will be subscribed to publisher @a publisher_.
		 */
		explicit
		WeakObserver(
				publisher_type &publisher_);

		/**
		 * Copy-constructor.
		 *
		 * This newly-created instance will be subscribed to the publisher to which
		 * @a other is subscribed (if any).
		 */
		WeakObserver(
				const this_type &other);

		/**
		 * Destructor.
		 *
		 * If this WeakObserver instance is subscribed to a publisher, it will be
		 * unsubscribed by this destructor.
		 */
		~WeakObserver();

		/**
		 * Return whether this instance is subscribed to a publisher.
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
		 * Copy-assign the value of @a other to this instance.
		 *
		 * The effect of this operation is that this instance will be subscribed to the
		 * publisher to which @a other is subscribed (if any).
		 */
		WeakObserver<T, ConstT> &
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
	private:
		/**
		 * If non-NULL, this points to the publisher instance to which this weak observer
		 * is subscribed.
		 */
		publisher_type *d_publisher_ptr;

		/**
		 * Subscribe this weak observer to @a publisher_.
		 *
		 * This function should be invoked in the general case that it is not known whether
		 * @a publisher_ has any other subscribers or not.
		 *
		 * It is a pre-condition of this function that this instance is not subscribed to
		 * any publisher.
		 */
		void
		subscribe_to_publisher_unknown_other_subscribers(
				publisher_type &publisher_);

		/**
		 * Subscribe this weak observer to the publisher to which @a other is subscribed.
		 *
		 * This function is able to be slightly more efficient than the function
		 * @a subscribe_to_publisher_unknown_other_subscribers, since it knows that the
		 * publisher must already have at least one other subscriber, namely @a other.
		 *
		 * It is a pre-condition of this function that this instance is not subscribed to
		 * any publisher.
		 */
		void
		subscribe_to_same_publisher_as_other(
				const this_type &other);
	};


	template<typename T, typename ConstT>
	inline
	WeakObserver<T, ConstT>::WeakObserver(
			publisher_type &publisher_):
		d_publisher_ptr(NULL)
	{
		subscribe_to_publisher_unknown_other_subscribers(publisher_);
	}


	template<typename T, typename ConstT>
	inline
	WeakObserver<T, ConstT>::WeakObserver(
			const WeakObserver<T, ConstT> &other):
		WeakObserverBase<ConstT>(),
		d_publisher_ptr(NULL)
	{
		subscribe_to_same_publisher_as_other(other);
	}


	template<typename T, typename ConstT>
	WeakObserver<T, ConstT>::~WeakObserver()
	{
		if (is_subscribed()) {
			remove_from_subscriber_list(*d_publisher_ptr);
		}
	}


	template<typename T, typename ConstT>
	inline
	void
	WeakObserver<T, ConstT>::subscribe(
			publisher_type &publisher_)
	{
		if (is_subscribed()) {
			remove_from_subscriber_list(*d_publisher_ptr);
		}
		subscribe_to_publisher_unknown_other_subscribers(publisher_);
	}


	template<typename T, typename ConstT>
	void
	WeakObserver<T, ConstT>::unsubscribe()
	{
		if (is_subscribed()) {
			remove_from_subscriber_list(*d_publisher_ptr);
			d_publisher_ptr = NULL;
		}
	}


	template<typename T, typename ConstT>
	WeakObserver<T, ConstT> &
	WeakObserver<T, ConstT>::operator=(
			const WeakObserver<T, ConstT> &other)
	{
		if (&other != this) {
			// This instance must unsubscribe itself from its current publisher, and
			// subscribe to the publisher observed by 'other'.
			if (is_subscribed()) {
				remove_from_subscriber_list(*d_publisher_ptr);
			}
			subscribe_to_same_publisher_as_other(other);
		}
		return *this;
	}


	template<typename T, typename ConstT>
	inline
	void
	WeakObserver<T, ConstT>::swap(
			WeakObserver<T, ConstT> &other)
	{
		// We're OK with this copy-construct, copy-assign, copy-assign implementation of
		// swap, since all of these operations will consist of builtin operations only.
		WeakObserver<T, ConstT> tmp(*this);
		*this = other;
		other = tmp;
	}


	template<typename T, typename ConstT>
	void
	WeakObserver<T, ConstT>::subscribe_to_publisher_unknown_other_subscribers(
			publisher_type &publisher_)
	{
		d_publisher_ptr = &publisher_;
		WeakObserverBase<ConstT>::prev_link_ptr() = publisher_.last_weak_observer();

		// We don't know whether the value of 'publisher_.last_weak_observer()', which has
		// been assigned to 'd_prev_link_ptr', is NULL or not.  (It will be NULL if there
		// were no other weak observers of this publisher; non-NULL otherwise.)
		if (WeakObserverBase<ConstT>::prev_link_ptr() != NULL) {
			// Tell the previous link, which was previously the last observer of the
			// instance of T, to point to this instance as its next.
			set_next_link_ptr_of_prev_link(this);
		} else {
			// Since there was no "last weak observer" of the instance of T, this
			// instance must be the first.
			d_publisher_ptr->first_weak_observer() = this;
		}
		d_publisher_ptr->last_weak_observer() = this;
	}


	template<typename T, typename ConstT>
	void
	WeakObserver<T, ConstT>::subscribe_to_same_publisher_as_other(
			const WeakObserver<T, ConstT> &other)
	{
		d_publisher_ptr = other.d_publisher_ptr;
		if (d_publisher_ptr != NULL) {
			// 'other' was subscribed to a publisher.  Let's subscribe also.
			WeakObserverBase<ConstT>::prev_link_ptr() = d_publisher_ptr->last_weak_observer();

			// We know that 'd_prev_link_ptr' cannot be NULL, since 'other' is already
			// subscribed to the publisher, so there must be at least one subscriber
			// before this instance.  Tell the previous link, which was previously the
			// last observer of the instance of T, to point to this instance as its
			// next.
			set_next_link_ptr_of_prev_link(this);
			d_publisher_ptr->last_weak_observer() = this;
		}
	}
}

namespace std
{
	/**
	 * This is a template specialisation of the standard function @a swap.
	 *
	 * See Josuttis, section 4.4.2, "Swapping Two Values" for more information.
	 */
	template<typename T, typename ConstT>
	inline
	void
	swap(
			GPlatesModel::WeakObserver<T, ConstT> &w1,
			GPlatesModel::WeakObserver<T, ConstT> &w2)
	{
		w1.swap(w2);
	}
}

#endif  // GPLATES_MODEL_WEAKOBSERVER_H
