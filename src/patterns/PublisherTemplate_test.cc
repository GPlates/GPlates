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
 * it.  Hence: this file.
 *
 * It also tests that all the operations behave as they are meant to, and as an
 * added bonus, it acts as demo code for @a GPlatesPatterns::PublisherTemplate.
 *
 * The contents of this file will never be compiled into the GPlates
 * executable, and as such, it will not be compiled automatically.  That's
 * right, kids, you have to compile it @b yourselves!  I recommend something
 * like <code>g++ -Wall -ansi -pedantic PublisherTemplate_test.cc -I ..
 * ../global/InternalInconsistencyException.cc ../global/Exception.cc</code>,
 * but as always, YMMV.  (Don't forget to execute it after all that.)
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
		 << " (subscribed to ";

		if (publisher() == NULL) std::cout << "no publisher";
		else std::cout << publisher();

		std::cout
		 << ") received notification."
		 << std::endl;
	}

};


void
describe_subscriber(
 const TestSubscriber &ts);

void
describe_publisher(
 const TestPublisher &tp);


int
main() {

	std::cout
	 << "\n* Default-ctoring 1 subscriber."
	 << std::endl;
	TestSubscriber ts3;

	describe_subscriber(ts3);

	{
		std::cout
		 << "\n* Beginning nested block."
		 << "\n* Default-ctoring 1 publisher and 2 subscribers."
		 << std::endl;
		TestPublisher tp1;
		TestSubscriber ts1, ts2;

		describe_publisher(tp1);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);

		std::cout
		 << "\n* Subscribing 3 subscribers to publisher "
		 << (&tp1)
		 << "."
		 << std::endl;
		ts3.subscribe_to(tp1);
		tp1.append_subscriber(ts1);
		ts2.subscribe_to(tp1);

		describe_publisher(tp1);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);

		std::cout
		 << "\n* Notifying subscribers of publisher "
		 << (&tp1)
		 << "."
		 << std::endl;
		tp1.notify_subscribers();

		std::cout
		 << "\n* Copy-ctoring a new subscriber from subscriber "
		 << (&ts2)
		 << "."
		 << std::endl;
		TestSubscriber ts4(ts2);

		describe_publisher(tp1);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);

		std::cout
		 << "\n* Notifying subscribers of publisher "
		 << (&tp1)
		 << "."
		 << std::endl;
		tp1.notify_subscribers();

		std::cout
		 << "\n* Default-ctoring 1 subscriber."
		 << std::endl;
		TestSubscriber ts5;

		describe_publisher(tp1);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);
		describe_subscriber(ts5);

		std::cout
		 << "\n* Assigning subscriber "
		 << (&ts5)
		 << " to subscriber "
		 << (&ts2)
		 << "."
		 << std::endl;
		ts2 = ts5;

		describe_publisher(tp1);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);
		describe_subscriber(ts5);

		std::cout
		 << "\n* Unsubscribing two subscribers ("
		 << (&ts1)
		 << " and "
		 << (&ts2)
		 << ") from publisher\n "
		 << (&tp1)
		 << "."
		 << std::endl;

		ts1.unsubscribe();
		tp1.remove_subscriber(ts2);

		describe_publisher(tp1);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);
		describe_subscriber(ts5);

		std::cout
		 << "\n* Subscribing subscriber "
		 << (&ts3)
		 << " to publisher "
		 << (&tp1)
		 << "."
		 << std::endl;
		ts3.subscribe_to(tp1);

		describe_publisher(tp1);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);
		describe_subscriber(ts5);

		std::cout
		 << "\n* Notifying subscribers of publisher "
		 << (&tp1)
		 << "."
		 << std::endl;
		tp1.notify_subscribers();

		std::cout << "\n* Default-ctoring 1 publisher." << std::endl;
		TestPublisher tp2;

		describe_publisher(tp1);
		describe_publisher(tp2);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);
		describe_subscriber(ts5);

		std::cout
		 << "\n* Notifying subscribers of publisher "
		 << (&tp1)
		 << "."
		 << std::endl;
		tp1.notify_subscribers();

		std::cout
		 << "\n* Notifying subscribers of publisher "
		 << (&tp2)
		 << "."
		 << std::endl;
		tp2.notify_subscribers();

		std::cout
		 << "\n* Subscribing 1 subscriber to publisher "
		 << (&tp2)
		 << "."
		 << std::endl;
		ts3.subscribe_to(tp2);

		describe_publisher(tp1);
		describe_publisher(tp2);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);
		describe_subscriber(ts5);

		std::cout
		 << "\n* Subscribing 1 subscriber to publisher "
		 << (&tp2)
		 << "."
		 << std::endl;
		ts2.subscribe_to(tp2);

		describe_publisher(tp1);
		describe_publisher(tp2);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);
		describe_subscriber(ts5);

		std::cout
		 << "\n* Subscribing 1 subscriber to publisher "
		 << (&tp1)
		 << "."
		 << std::endl;
		ts5.subscribe_to(tp1);

		describe_publisher(tp1);
		describe_publisher(tp2);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);
		describe_subscriber(ts5);

		std::cout
		 << "\n* Removing all subscribers of publisher "
		 << (&tp1)
		 << "."
		 << std::endl;
		tp1.remove_all_subscribers();

		describe_publisher(tp1);
		describe_publisher(tp2);
		describe_subscriber(ts3);
		describe_subscriber(ts1);
		describe_subscriber(ts2);
		describe_subscriber(ts4);
		describe_subscriber(ts5);

		std::cout
		 << "\n* Notifying subscribers of publisher "
		 << (&tp1)
		 << "."
		 << std::endl;
		tp1.notify_subscribers();

		std::cout
		 << "\n* Notifying subscribers of publisher "
		 << (&tp2)
		 << "."
		 << std::endl;
		tp2.notify_subscribers();

		std::cout
		 << "\n* Ending nested block.  "
		 << "(End of lifetime of 2 publishers and 4 subscribers.)"
		 << std::endl;
	}

	describe_subscriber(ts3);

	return 0;
}


void
describe_subscriber(
 const TestSubscriber &ts) {

	std::cout
	 << "- Subscriber "
	 << (&ts)
	 << " is currently subscribed to ";

	if (ts.publisher() == NULL) std::cout << "no publisher";
	else std::cout << "publisher " << ts.publisher();

	std::cout << std::endl;
}


void
describe_publisher(
 const TestPublisher &tp) {

	std::cout
	 << "- Publisher "
	 << (&tp)
	 << " currently has "
	 << tp.num_subscribers()
	 << " subscribers"
	 << std::endl;
}
