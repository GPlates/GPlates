/* $Id$ */

/**
 * \file
 * This file contains code to test the template class
 * @a GPlatesPatterns::PublisherTemplate.
 *
 * Since @a GPlatesPatterns::PublisherTemplate is a template, it's not
 * instantiated until it's used.  (The same goes for its member functions.)
 * And since you can't check the correctness of code which hasn't been
 * instantiated, the only way to check the correctness of this code is to use
 * it.  Hence: this class.
 *
 * It also tests that all the operations behave as they are meant to, and as an
 * added bonus, it acts as demo code for @a GPlatesPatterns::PublisherTemplate.
 *
 * The contents of this file will never be compiled into the GPlates
 * executable, and as such, it will not be compiled automatically.  That's
 * right, kids, you have to compile it @b yourselves!  I recommend something
 * like <code>g++ -Wall -ansi -pedantic PublisherTemplate_test.cc</code>, but
 * as always, YMMV.  (Don't forget to execute it after all that.)
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

#include <iostream>
#include "PublisherTemplate.h"


class TestPublisher: public
 GPlatesPatterns::PublisherTemplate< TestPublisher > {

};

class TestSubscriber: public
 GPlatesPatterns::PublisherTemplate< TestPublisher >::Subscriber {

 public:

	void
	receive_notification() {

		std::cout
		 << "- Subscriber "
		 << this
		 << " (subscribed to "
		 << publisher()
		 << ") received notification."
		 << std::endl;
	}

};


int
main() {

	TestSubscriber ts3;

	std::cout
	 << "- Subscriber "
	 << (&ts3)
	 << " is currently subscribed to: "
	 << ts3.publisher()
	 << std::endl;

	{
		std::cout << "* Beginning nested block..." << std::endl;

		TestPublisher tp1;
		TestSubscriber ts1, ts2;

		std::cout
		 << "- Number of subscribers: "
		 << tp1.num_subscribers()
		 << std::endl;
		std::cout
		 << "- Subscriber "
		 << (&ts3)
		 << " is currently subscribed to: "
		 << ts3.publisher()
		 << std::endl;
		std::cout
		 << "- Subscriber "
		 << (&ts1)
		 << " is currently subscribed to: "
		 << ts1.publisher()
		 << std::endl;
		std::cout
		 << "- Subscriber "
		 << (&ts2)
		 << " is currently subscribed to: "
		 << ts2.publisher()
		 << std::endl;

		std::cout << "* Subscribing three subscribers..." << std::endl;

		ts3.subscribe(tp1);
		tp1.append_subscriber(ts1);
		ts2.subscribe(tp1);

		std::cout
		 << "- Number of subscribers: "
		 << tp1.num_subscribers()
		 << std::endl;
		std::cout
		 << "- Subscriber "
		 << (&ts3)
		 << " is currently subscribed to: "
		 << ts3.publisher()
		 << std::endl;
		std::cout
		 << "- Subscriber "
		 << (&ts1)
		 << " is currently subscribed to: "
		 << ts1.publisher()
		 << std::endl;
		std::cout
		 << "- Subscriber "
		 << (&ts2)
		 << " is currently subscribed to: "
		 << ts2.publisher()
		 << std::endl;

		std::cout << "* Notifying subscribers." << std::endl;
		tp1.notify_subscribers();

		std::cout << "* Unsubscribing two subscribers..." << std::endl;

		ts1.unsubscribe();
		tp1.remove_subscriber(ts2);

		std::cout
		 << "- Number of subscribers: "
		 << tp1.num_subscribers()
		 << std::endl;
		std::cout
		 << "- Subscriber "
		 << (&ts3)
		 << " is currently subscribed to: "
		 << ts3.publisher()
		 << std::endl;
		std::cout
		 << "- Subscriber "
		 << (&ts1)
		 << " is currently subscribed to: "
		 << ts1.publisher()
		 << std::endl;
		std::cout
		 << "- Subscriber "
		 << (&ts2)
		 << " is currently subscribed to: "
		 << ts2.publisher()
		 << std::endl;

		std::cout
		 << "* Ending nested block.  "
		 << "(End of lifetime of publisher.)"
		 << std::endl;
	}

	std::cout
	 << "- Subscriber "
	 << (&ts3)
	 << " is currently subscribed to: "
	 << ts3.publisher()
	 << std::endl;

	return 0;
}
