/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
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
 */

#ifndef GPLATES_PATTERNS_PUBLISHERTEMPLATE_H
#define GPLATES_PATTERNS_PUBLISHERTEMPLATE_H

#include <list>
#include <algorithm>
#include "global/InternalInconsistencyException.h"


namespace GPlatesPatterns {

	/**
	 * These base classes are intended to simplify the implementation of
	 * the "Gang of Four" Observer pattern (aka the Publisher-Subscriber
	 * pattern).
	 *
	 * @section Overview
	 *
	 * - The publisher/subscriber relationship is made typesafe by the use
	 *    of "mixin-style" base classes (see p.31 of Meyers98).
	 * - The subscription/unsubscription operations may be performed upon
	 *    either the publisher or subscriber (the effect will be
	 *    equivalent).
	 * - When a subscriber is being destroyed, it automatically
	 *    unsubscribes itself: when a publisher is being destroyed, it
	 *    automatically unsubscribes all its subscribers.
	 * - All operations defined in these classes are strongly
	 *    exception-safe (if any operation terminates due to an exception,
	 *    program state will remain unchanged) and exception-neutral (any
	 *    exceptions are propagated to the caller).
	 *
	 * @section Usage
	 *
	 * To become a publisher-type, a class X must:
	 *  - publicly inherit from GPlatesPatterns::PublisherTemplate< X >
	 *
	 * To become a subscriber-type of X, a class Y must:
	 *  - publicly inherit from
	 *     GPlatesPatterns::PublisherTemplate< X >::Subscriber
	 *  - define the member function @a receive_notification (this
	 *     definition may be virtual or non-virtual)
	 *
	 * This implementation does not allow a class to inherit from multiple
	 * publishers or multiple subscribers.  Thus, a publisher can only
	 * present one publishing interface, and a subscriber can only observe
	 * one publisher.  If this becomes a real limitation, we can always
	 * write a 1337er implementation involving member function pointers or
	 * something.
	 *
	 * @attention Note that a class may be both a publisher @em and a
	 * subscriber to a @em different publisher.
	 *
	 * @section Details
	 *
	 *  - Publishers do not advertise an explicit ordering of their
	 *     subscribers.
	 *  - There are no automatic actions triggered when a subscriber is
	 *     subscribed or unsubscribed.
	 *  - Attempting to subscribe a subscriber to a publisher to which it
	 *     is already subscribed is a no-op (see
	 *     @a PublisherTemplate< T >::Subscriber::subscribe_to for more
	 *     details).
	 *  - Attempting to unsubscribe a subscriber which is not subscribed to
	 *     anything is a no-op (see
	 *     @a PublisherTemplate< T >::Subscriber::unsubscribe for more
	 *     details).
	 *
	 * @section References and Thanks
	 *
	 *  - Scott Meyers, <i>Effective C++: 50 Specific Ways to Improve Your
	 *     Programs and Designs</i>, 2nd ed., Addison-Wesley, 1998.
	 *  - Nicolai M. Josuttis, <i>The C++ Standard Library: A Tutorial and
	 *     Reference</i>, Addison-Wesley, 1999.
	 *  - Herb Sutter, <i>Exceptional C++: 47 Engineering Puzzles,
	 *     Programming Problems, and Solutions</i>, Addison Wesley Longman,
	 *     2000.
	 *
	 * The author would also like to thank the inventors of coffee for
	 * their invaluable contribution to the development of this code.
	 */
	template< typename T >
	class PublisherTemplate {

	 public:

		typedef T PublisherType;
		typedef PublisherTemplate< T > PublisherBaseType;

		/**
		 * This class is the abstract base class of all subscribers to
		 * publishers of type PublisherTemplate< X >.
		 */
		class Subscriber {

		 public:

			/**
			 * This function will not throw.
			 */
			Subscriber() :
			 m_publisher(NULL) {  }

			/**
			 * This function will not throw.
			 */
			virtual
			~Subscriber() {

				/*
				 * Unsubscribe, before our publisher tries to
				 * notify a dangling pointer.
				 */
				unsubscribe();  // This won't throw.
			}

			/**
			 * The publisher will invoke this function to notify
			 * this subscriber that an event has occurred.
			 *
			 * Classes which derive from this base class must
			 * provide a definition of this function.
			 *
			 * For obvious reasons, no guarantees can be made about
			 * the exception safety of this function.
			 */
			virtual
			void
			receive_notification() = 0;

			/**
			 * Return a pointer to the publisher to which this
			 * subscriber is subscribed.
			 *
			 * If this subscriber is not subscribed to any
			 * publisher, NULL will be returned.
			 *
			 * This function will not throw.
			 */
			const PublisherBaseType *
			publisher() const {

				return m_publisher;
			}

			/**
			 * Invoke this function to subscribe this subscriber to
			 * @a publisher.
			 *
			 * If this subscriber is already subscribed to
			 * @a publisher, this operation will be a no-op (but is
			 * otherwise considered valid).  If this subscriber is
			 * already subscribed to a @em different publisher, it
			 * will be unsubscribed from that publisher first.
			 *
			 * This function is strongly exception safe and
			 * exception neutral.
			 */
			void
			subscribe_to(
			 PublisherBaseType &publisher);

			/**
			 * Invoke this function to unsubscribe this subscriber
			 * from its publisher.
			 *
			 * If it is not subscribed to any publisher, this
			 * operation will be a no-op (but is otherwise
			 * considered valid).
			 *
			 * This function will not throw.
			 */
			void
			unsubscribe();

		 protected:

			/**
			 * If @a other is subscribed to a publisher, the new
			 * instance will be subscribed to that same publisher.
			 *
			 * This copy-constructor is protected (rather than
			 * public) because Subscriber is a virtual base class
			 * (even though, as an abstract class, it can't ever be
			 * explicitly instantiated anyway).
			 *
			 * This function is strongly exception safe and
			 * exception neutral.
			 *
			 * Classes which derive from this base class, and
			 * explicitly define their own copy-constructor, must
			 * explicitly invoke this copy-constructor within the
			 * initialisation list (see Meyers98, Item 16, for more
			 * details).
			 */
			Subscriber(
			 const Subscriber &other);

			/**
			 * If @a other is subscribed to a publisher, this
			 * instance will be subscribed to that same publisher.
			 * If @a other is not subscribed to any publisher, this
			 * instance will be likewise unsubscribed from any
			 * subscriber to which it may currently be subscribed.
			 *
			 * This assignment operator is protected (rather than
			 * public) because Subscriber is a virtual base class.
			 * This is intended to remove the ability to invoke
			 * this function upon an instance of a subclass, which
			 * would result in partial ("slicing") assignment.
			 *
			 * This function is strongly exception safe and
			 * exception neutral.
			 *
			 * Classes which derive from this base class, and
			 * explicitly define their own assignment operator,
			 * must explicitly invoke this assignment operator
			 * within the derived class' assignment operator (see
			 * Meyers98, Item 16, for more details).
			 */
			Subscriber &
			operator=(
			 const Subscriber &other);

		 private:

			/**
			 * The publisher to which this subscriber is currently
			 * subscribed.
			 *
			 * If this subscriber is not subscribed to any
			 * publisher, this will be NULL.
			 */
			PublisherBaseType *m_publisher;

		};

		typedef std::list< Subscriber * > Subscribers;
		typedef typename Subscribers::size_type size_type;

		/**
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		PublisherTemplate() {  }

		/**
		 * This function will not throw.
		 */
		~PublisherTemplate() {

			remove_all_subscribers();  // This won't throw.
		}

		/**
		 * Return the current number of subscribers.
		 *
		 * This function will not throw.
		 */
		size_type
		num_subscribers() const {

			return m_subscribers.size();
		}

		/**
		 * Notify all subscribers that an event has occurred.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		void
		notify_subscribers();

		/**
		 * Subscribe the subscriber @a s to this publisher.
		 *
		 * If @a s is already subscribed to this publisher, this
		 * operation will be a no-op (but is otherwise considered
		 * valid).
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		void
		append_subscriber(
		 Subscriber &s);

		/**
		 * Unsubscribe the subscriber @a s from this publisher.
		 *
		 * If @a s is not subscribed to this publisher, this operation
		 * will be a no-op (but is otherwise considered valid).
		 *
		 * This function will not throw.
		 */
		void
		remove_subscriber(
		 Subscriber &s);

		/**
		 * Unsubscribe all subscribers from this publisher.
		 *
		 * This function will not throw.
		 */
		void
		remove_all_subscribers();

	 private:

		/**
		 * The current subscribers to this publisher.
		 */
		Subscribers m_subscribers;

		/**
		 * Remove subscriber @a s from @a m_subscribers.
		 *
		 * This function is private because it should only ever be
		 * invoked by @a Subscriber::unsubscribe.
		 *
		 * This function will not throw.
		 */
		void
		remove(
		 Subscriber &s);

		/**
		 * Splice subscriber @a s out of @a m_subscribers, into the
		 * list of subscribers @a into_this.
		 *
		 * This function is private because it should only ever be
		 * invoked by @a Subscriber::subscribe_to.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		void
		splice_out(
		 const Subscriber &s,
		 Subscribers &into_this);

		/**
		 * Splice the contents of the list of subscribers @a from_this
		 * into @a m_subscribers.
		 *
		 * This function is private because it should only ever be
		 * invoked by @a Subscriber::subscribe_to.
		 *
		 * This function will not throw.
		 */
		void
		splice_in(
		 Subscribers &from_this);

		/**
		 * Declare the copy-constructor private, because
		 * copy-construction doesn't make sense for publishers.
		 *
		 * If a publisher were copy-constructed from another, the new
		 * publisher instance should be a duplicate of the original,
		 * which would mean an identical copy of @a m_subscribers.  But
		 * the new instance would be in an invalid state unless all the
		 * subscribers referred-to in @a m_subscribers were actually
		 * subscribed to it.  But since a subscriber can only be
		 * subscribed to @em one publisher at any time, this would mean
		 * all the subscribers would need to be unsubscribed from the
		 * original publisher.
		 *
		 * Clearly, this violates the principle of copy-construction,
		 * so copy-construction is disabled.
		 *
		 * This copy-constructor should @em not be defined.
		 */
		PublisherTemplate(
		 const PublisherTemplate &);

		/**
		 * Declare the assignment operator private, because assignment
		 * doesn't make sense for publishers.
		 *
		 * If a publisher were assigned from another, the l-value
		 * (assigned-to) publisher instance should be a duplicate of
		 * the r-value instance, which would mean an identical copy of
		 * @a m_subscribers.  But the l-value instance would be left in
		 * an invalid state unless all the subscribers referred-to in
		 * @a m_subscribers were actually subscribed to it.  But since
		 * a subscriber can only be subscribed to @em one publisher at
		 * any time, this would mean all the subscribers would need to
		 * be unsubscribed from the r-value instance.
		 *
		 * Clearly, this violates the principle of assignment, so
		 * assignment is disabled.
		 *
		 * This assignment operator should @em not be defined.
		 */
		PublisherTemplate &
		operator=(
		 const PublisherTemplate &);

	};

}


template< typename T >
void
GPlatesPatterns::PublisherTemplate< T >::Subscriber::subscribe_to(
 PublisherBaseType &publisher) {

	if (&publisher == m_publisher) {

		/*
		 * This instance is already subscribed to this publisher.
		 * Do nothing.
		 */
		return;
	}

	Subscribers tmp;

	/*
	 * Ensure 'tmp' contains a pointer to this instance.  Also, if this
	 * instance is currently subscribed to a publisher, unsubscribe it.
	 * This is the only part of this function which might throw.
	 *
	 * If we make it to the end of this if/else statement, the following
	 * statements will be true:
	 *  - 'tmp' will hold a pointer to this instance.
	 *  - this instance will not be subscribed to any publisher.
	 *  - no exceptions will be thrown for the rest of the function.
	 */
	if (m_publisher != NULL) {

		// Remove this instance from m_publisher's list of subscribers.
		m_publisher->splice_out(*this, tmp);  // This won't throw.
		if (tmp.empty()) {

			/*
			 * Nothing was spliced into 'tmp'.  This means that
			 * this instance was not found in m_publisher's list of
			 * subscribers, despite the fact that this instance
			 * thinks it's subscribed...
			 */
			throw GPlatesGlobal::InternalInconsistencyException(
			 __FILE__, __LINE__,
			 "This Subscriber instance was not found in the list "
			 "of subscribers to the Publisher to which this "
			 "Subscriber currently holds a reference.");
		}

	} else {

		tmp.push_back(this);  // This may throw.
	}

	/*
	 * Now splice the contents of 'tmp' into the new publisher's list of
	 * subscribers, update the data member, and we're done.
	 */
	publisher.splice_in(tmp);  // This won't throw.
	m_publisher = &publisher;
}


template< typename T >
inline
void
GPlatesPatterns::PublisherTemplate< T >::Subscriber::unsubscribe() {

	if (m_publisher == NULL) {

		/* 
		 * This subscriber is not subscribed to any publisher.
		 * Do nothing.
		 */
		return;
	}
	m_publisher->remove(*this);  // This won't throw.
	m_publisher = NULL;
}


template< typename T >
inline
GPlatesPatterns::PublisherTemplate< T >::Subscriber::Subscriber(
 const Subscriber &other) :
 m_publisher(NULL) {

	if (other.m_publisher != NULL) subscribe_to(*other.m_publisher);
}


template< typename T >
typename GPlatesPatterns::PublisherTemplate< T >::Subscriber &
GPlatesPatterns::PublisherTemplate< T >::Subscriber::operator=(
 const Subscriber &other) {

	if (&other != this) {

		if (other.m_publisher == NULL) {

			/*
			 * 'other' is not subscribed to any publisher, so if we
			 * want to be like 'other', we only need to unsubscribe
			 * ourselves from our current publisher (if any).
			 */
			unsubscribe();  // This won't throw.

		} else {

			/*
			 * If we want to be like 'other', we need to subscribe
			 * to 'other''s publisher.
			 */
			subscribe_to(*other.m_publisher);  // This may throw.
		}
	}
	return *this;
}


template< typename T >
void
GPlatesPatterns::PublisherTemplate< T >::notify_subscribers() {

	typename Subscribers::iterator
	 iter = m_subscribers.begin(),
	 end = m_subscribers.end();

	for ( ; iter != end; ++iter) (*iter)->receive_notification();
}


template< typename T >
inline
void
GPlatesPatterns::PublisherTemplate< T >::append_subscriber(
 Subscriber &s) {

	if (s.publisher() == this) {

		/*
		 * This subscriber is already subscribed to this publisher.
		 * Do nothing.
		 */
		return;
	}
	s.subscribe_to(*this);
}


template< typename T >
inline
void
GPlatesPatterns::PublisherTemplate< T >::remove_subscriber(
 Subscriber &s) {

	if (s.publisher() != this) {

		/*
		 * This subscriber is not subscribed to this publisher.
		 * Do nothing.
		 */
		return;
	}
	s.unsubscribe();
}


template< typename T >
void
GPlatesPatterns::PublisherTemplate< T >::remove_all_subscribers() {

	/*
	 * This needs to be a little trickier than the standard iterator-based
	 * for-loop, because the call to 'Subscriber::unsubscribe' will result
	 * in the removal of the list-node pointed-to by 'iter', which will
	 * leave 'iter' in an undefined state.  Hence, 'next' is always a step
	 * ahead.
	 */
	typename Subscribers::iterator
	 iter,  // not yet
	 next = m_subscribers.begin(),
	 end = m_subscribers.end();

	for (iter = next; iter != end; iter = next) {

		++next;  // Only do this after we are certain (iter != end).
		(*iter)->unsubscribe();
	}

	// Josuttis99 says this won't throw (section 6.10.7).
	m_subscribers.clear();
}


template< typename T >
inline
void
GPlatesPatterns::PublisherTemplate< T >::remove(
 Subscriber &s) {

	/*
	 * Josuttis99 says this won't throw unless the comparison of the
	 * elements can throw (section 6.10.7).  Since the elements are
	 * pointers, I think it unlikely that a comparison will throw.
	 * Hence, assume this won't throw.
	 */
	m_subscribers.remove(&s);
}


template< typename T >
inline
void
GPlatesPatterns::PublisherTemplate< T >::splice_out(
 const Subscriber &s,
 Subscribers &into_this) {

	// 'find' should not throw because pointer comparisons don't throw.
	typename Subscribers::iterator found =
	 std::find(m_subscribers.begin(), m_subscribers.end(), &s);

	// Check that the subscriber 's' is actually contained in
	// 'm_subscribers'.  If it's not, no splicing will occur.
	if (found != m_subscribers.end()) {

		// We found the subscriber 's' in 'm_subscribers'.
		// Now splice it out.

		// Josuttis99 (section 6.10.8) says this won't throw.
		into_this.splice(into_this.end(), m_subscribers, found);
	}
}


template< typename T >
inline
void
GPlatesPatterns::PublisherTemplate< T >::splice_in(
 Subscribers &from_this) {

	// Josuttis99 (section 6.10.8) says this won't throw.
	m_subscribers.splice(m_subscribers.end(), from_this);
}

#endif  // GPLATES_PATTERNS_PUBLISHERTEMPLATE_H
