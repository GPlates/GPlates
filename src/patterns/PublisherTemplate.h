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
	 * This implementation does not allow:
	 *  - a class to multiply-inherit from PublisherTemplate< whatever >
	 *  - a class to multiply-inherit from Subscriber
	 * Thus, a Publisher can only present one publishing interface, and a
	 * Subscriber can only observe one Publisher.  If this becomes a real
	 * limitation, we can always write a 1337er implementation involving
	 * member function pointers or something.
	 *
	 * Note that a class <em>may</em> be both a publisher and a subscriber
	 * to a different publisher.
	 *
	 * To become a publisher-type, a class X must:
	 *  - publicly inherit from GPlatesPatterns::PublisherTemplate< X >
	 *
	 * To become a subscriber-type of X, a class Y must:
	 *  - publicly inherit from
	 *     GPlatesPatterns::PublisherTemplate< X >::Subscriber
	 *  - define the member function 'void receive_notification()' (this 
	 *     definition may be virtual or non-virtual)
	 */
	template< typename T >
	class PublisherTemplate {

	 public:

		typedef T PublisherType;

		class Subscriber {

		 public:

			virtual
			~Subscriber() {  }

			virtual
			void
			receive_notification() = 0;

		};

		typedef std::list< Subscriber * > Subscribers;

		~PublisherTemplate() {  }

		void
		notify_subscribers() const {

			typename Subscribers::const_iterator
			 iter = m_subscribers.begin(),
			 end = m_subscribers.end();

			for ( ; iter != end; ++iter) {

				(*iter)->receive_notification();
			}
		}

		void
		append_subscriber(
		 Subscriber &s) {

			m_subscribers.push_back(&s);
		}

		void
		remove_subscriber(
		 Subscriber &s) {

			m_subscribers.remove(&s);
		}

	 private:

		Subscribers m_subscribers;

	};

}

#endif  // GPLATES_PATTERNS_PUBLISHERTEMPLATE_H
