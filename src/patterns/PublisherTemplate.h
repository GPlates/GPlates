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

namespace GPlatesPatterns {

	/**
	 * These base classes are intended to simplify the implementation of
	 * the "Gang of Four" Observer pattern (aka the Publisher-Subscriber
	 * pattern).
	 *
	 * The publisher/subscriber relationship is made typesafe by the use of
	 * "mixin-style" base classes (see p.31 of Meyers, <i>Effective
	 * C++</i>, 2nd ed., Addison-Wesley, 1997).  The
	 * subscription/unsubscription operations may be performed upon either
	 * the publisher or subscriber (the effect will be equivalent).  When a
	 * subscriber is being destroyed, it automatically unsubscribes itself:
	 * when a publisher is being destroyed, it automatically unsubscribes
	 * all its subscribers.
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
	 * This implementation does not allow:
	 *  - a class to multiply-inherit from PublisherTemplate< whatever >
	 *  - a class to multiply-inherit from Subscriber
	 *
	 * Thus, a Publisher can only present one publishing interface, and a
	 * Subscriber can only observe one Publisher.  If this becomes a real
	 * limitation, we can always write a 1337er implementation involving
	 * member function pointers or something.
	 *
	 * Note that a class may be both a publisher <em>and</em> a subscriber
	 * to a <em>different</em> publisher.
	 *
	 * The author would like to thank the inventors of coffee for their
	 * assistance in the development of this code.
	 */
	template< typename T >
	class PublisherTemplate {

	 public:

		typedef T PublisherType;
		typedef PublisherTemplate< T > PublisherBaseType;

		class Subscriber {

		 public:

			Subscriber() :
			 m_publisher(NULL),
			 m_is_locked(false) {  }

			virtual
			~Subscriber() {

				/*
				 * Unsubscribe, before our publisher tries to
				 * notify a dangling pointer.
				 */
				unsubscribe();
			}

			/**
			 * The publisher will invoke this function to notify
			 * this subscriber that an event has occurred.
			 *
			 * Classes which derive from this base class must
			 * provide a definition of this function.
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
			 */
			const PublisherBaseType *
			publisher() const {

				return m_publisher;
			}

			/**
			 * Invoke this function to subscribe this subscriber to
			 * the publisher @a publisher.
			 *
			 * If this subscriber is already subscribed to
			 * @a publisher, this operation will be a no-op.  If
			 * this subscriber is already subscribed to a
			 * <em>different</em> publisher, it will be
			 * unsubscribed from that publisher first.
			 */
			void
			subscribe(
			 PublisherBaseType &publisher);

			/**
			 * Invoke this function to unsubscribe this subscriber
			 * from its publisher.
			 *
			 * If it is not subscribed to any publisher, this
			 * operation will be a no-op.
			 */
			void
			unsubscribe();

		 private:

			/**
			 * The publisher to which this subscriber is currently
			 * subscribed.
			 *
			 * If this subscriber is not subscribed to any
			 * publisher, this will be NULL.
			 */
			PublisherBaseType *m_publisher;

			/**
			 * The instance is locked while it subscribes or
			 * unsubscribes itself, since these operations require
			 * otherwise-identical recursive function calls.
			 */
			bool m_is_locked;

		};

		typedef std::list< Subscriber * > Subscribers;
		typedef typename std::list< Subscriber * >::size_type
		 size_type;

		PublisherTemplate() :
		 m_is_removing_all(false),
		 m_is_locked(false) {  }

		~PublisherTemplate() {

			remove_all_subscribers();
		}

		/**
		 * Return the current number of subscribers.
		 */
		size_type
		num_subscribers() const {

			return m_subscribers.size();
		}

		/**
		 * Notify all subscribers that an event has occurred.
		 */
		void
		notify_subscribers();

		/**
		 * Subscribe the subscriber @a s to this publisher.
		 *
		 * If @a s is already subscribed to this publisher, this
		 * operation will be a no-op.
		 */
		void
		append_subscriber(
		 Subscriber &s);

		/**
		 * Unsubscribe the subscriber @a s from this publisher.
		 *
		 * If @a s is not subscribed to this publisher, this operation
		 * will be a no-op.
		 */
		void
		remove_subscriber(
		 Subscriber &s);

		/**
		 * Unsubscribe all subscribers from this publisher.
		 */
		void
		remove_all_subscribers();

	 private:

		/**
		 * The current subscribers to this publisher.
		 */
		Subscribers m_subscribers;

		/**
		 * Whether or not this publisher instance is currently
		 * removing all subscribers.
		 *
		 * Initialised by the ctor, accessed by @a remove_subscriber,
		 * set and unset by @a remove_all_subscribers.
		 */
		bool m_is_removing_all;

		/**
		 * The instance is locked while it subscribes or unsubscribes
		 * subscribers, since these operations require
		 * otherwise-identical recursive function calls.
		 */
		bool m_is_locked;

	};

}


template< typename T >
void
GPlatesPatterns::PublisherTemplate< T >::Subscriber::subscribe(
 PublisherBaseType &publisher) {

	if (m_is_locked){

		/*
		 * This subscriber is currently in the middle of the process of
		 * subscribing.
		 *
		 * Do nothing.
		 */
		return;
	}

	if (&publisher == m_publisher) {

		/*
		 * This subscriber is already subscribed to this publisher.
		 * Do nothing.
		 */
		return;
	}
	unsubscribe();

	m_is_locked = true;
	publisher.append_subscriber(*this);
	m_is_locked = false;

	m_publisher = &publisher;
}


template< typename T >
void
GPlatesPatterns::PublisherTemplate< T >::Subscriber::unsubscribe() {

	if (m_is_locked){

		/*
		 * This subscriber is currently in the middle of the process of
		 * unsubscribing.
		 *
		 * Do nothing.
		 */
		return;
	}

	if (m_publisher == NULL) {

		/* 
		 * This subscriber is not subscribed to any publisher.
		 * Do nothing.
		 */
		return;
	}

	m_is_locked = true;
	m_publisher->remove_subscriber(*this);
	m_is_locked = false;

	m_publisher = NULL;
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
void
GPlatesPatterns::PublisherTemplate< T >::append_subscriber(
 Subscriber &s) {

	if (m_is_locked){

		/*
		 * This publisher is currently in the middle of the process of
		 * subscribing a subscriber.
		 *
		 * Do nothing.
		 */
		return;
	}

	if (s.publisher() == this) {

		/*
		 * This subscriber is already subscribed.
		 * Do nothing.
		 */
		return;
	}
	m_subscribers.push_back(&s);

	m_is_locked = true;
	s.subscribe(*this);
	m_is_locked = false;
}


template< typename T >
void
GPlatesPatterns::PublisherTemplate< T >::remove_subscriber(
 Subscriber &s) {

	if (m_is_locked){

		/*
		 * This publisher is currently in the middle of the process of
		 * unsubscribing a subscriber.
		 *
		 * Do nothing.
		 */
		return;
	}

	if (m_is_removing_all) {

		/*
		 * This publisher instance is currently removing all
		 * subscribers, which means that it is currently iterating
		 * through the list of subscribers, unsubscribing them
		 * one-by-one.  In fact, the iterator is almost certainly
		 * pointing at the node which contains this very subscriber,
		 * having just told it to unsubscribe.
		 *
		 * Let's not confuse the iterator by deleting its node out from
		 * under it, ok?
		 */
		return;
	}

	if (s.publisher() != this) {

		/*
		 * This subscriber is not subscribed.
		 * Do nothing.
		 */
		return;
	}
	m_subscribers.remove(&s);

	m_is_locked = true;
	s.unsubscribe();
	m_is_locked = false;
}


template< typename T >
void
GPlatesPatterns::PublisherTemplate< T >::remove_all_subscribers() {

	m_is_removing_all = true;

	typename Subscribers::iterator
	 iter = m_subscribers.begin(),
	 end = m_subscribers.end();

	for ( ; iter != end; ++iter) (*iter)->unsubscribe();
	m_subscribers.clear();

	m_is_removing_all = false;
}

#endif  // GPLATES_PATTERNS_PUBLISHERTEMPLATE_H
