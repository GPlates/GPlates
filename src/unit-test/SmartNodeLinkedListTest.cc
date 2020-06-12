/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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
#include <iostream>

#include <QDebug>

#include "unit-test/SmartNodeLinkedListTest.h"

GPlatesUnitTest::SmartNodeLinkedListTestSuite::SmartNodeLinkedListTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"SmartNodeLinkedListTestSuite")
{
	init(level);
}


void
GPlatesUnitTest::SmartNodeLinkedListTestSuite::construct_maps()
{
	boost::shared_ptr<SmartNodeLinkedListTest> instance(
		new SmartNodeLinkedListTest());

	ADD_TESTCASE(SmartNodeLinkedListTest,test_increment_decrement_and_operator_arrow);
	ADD_TESTCASE(SmartNodeLinkedListTest,test_list_scoping);
}

namespace{
	
	struct A
	{
		int i;
		int j;
	};
	
	std::unique_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node>
	add_node_3(
			GPlatesUtils::SmartNodeLinkedList<int> &list)
	{
		typedef GPlatesUtils::SmartNodeLinkedList<int>::Node Node;

		GPlatesUtils::SmartNodeLinkedList<int>::iterator begin = list.begin();
		GPlatesUtils::SmartNodeLinkedList<int>::iterator end = list.end();
		BOOST_CHECK(*begin == 1);
		begin++;
		BOOST_CHECK(*begin == 2);
		begin++;
		BOOST_CHECK(begin == end);;

		std::unique_ptr<Node> node_3_ptr(new Node(3));
		list.append(*node_3_ptr);

		begin = list.begin();
		end = list.end();
		BOOST_CHECK(*begin == 1);
		begin++;
		BOOST_CHECK(*begin == 2);
		begin++;
		BOOST_CHECK(*begin == 3);
		begin++;
		BOOST_CHECK(begin == end);

		return node_3_ptr;
	}


	std::unique_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node>
	add_node_2(
			GPlatesUtils::SmartNodeLinkedList<int> &list)
	{
		GPlatesUtils::SmartNodeLinkedList<int>::iterator begin = list.begin();
		GPlatesUtils::SmartNodeLinkedList<int>::iterator end = list.end();
		BOOST_CHECK(*begin == 1);
		begin++;
		BOOST_CHECK(begin == end);


		GPlatesUtils::SmartNodeLinkedList<int>::Node node_2(2);
		list.append(node_2);

		begin = list.begin();
		end = list.end();
		BOOST_CHECK(*begin == 1);
		begin++;
		BOOST_CHECK(*begin == 2);
		begin++;
		BOOST_CHECK(begin == end);

		std::unique_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node> node_3_ptr = add_node_3(list);
		
		begin = list.begin();
		end = list.end();
		BOOST_CHECK(*begin == 1);
		begin++;
		BOOST_CHECK(*begin == 2);
		begin++;
		BOOST_CHECK(*begin == 3);
		begin++;
		BOOST_CHECK(begin == end);

		return node_3_ptr;
	}


	void
	invoke_add_node_2(
			GPlatesUtils::SmartNodeLinkedList<int> &list)
	{
		GPlatesUtils::SmartNodeLinkedList<int>::iterator begin = list.begin();
		GPlatesUtils::SmartNodeLinkedList<int>::iterator end = list.end();
		BOOST_CHECK(*begin == 1);
		begin++;
		BOOST_CHECK(begin == end);

		std::unique_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node> node_3_ptr = add_node_2(list);
		
		begin = list.begin();
		end = list.end();
		BOOST_CHECK(*begin == 1);
		begin++;
		BOOST_CHECK(*begin == 3);
		begin++;
		BOOST_CHECK(begin == end);
	}

	void
	add_node_1(
			GPlatesUtils::SmartNodeLinkedList<int> &list)
	{
		GPlatesUtils::SmartNodeLinkedList<int>::iterator begin = list.begin();
		GPlatesUtils::SmartNodeLinkedList<int>::iterator end = list.end();
		BOOST_CHECK(begin == end);

		GPlatesUtils::SmartNodeLinkedList<int>::Node node_1(1);
		list.append(node_1);
		
		begin = list.begin();
		end = list.end();
		BOOST_CHECK(*begin == 1);
		begin++;
		BOOST_CHECK(begin == end);
		
		invoke_add_node_2(list);
		
		begin = list.begin();
		end = list.end();
		BOOST_CHECK(*begin == 1);
		begin++;
		BOOST_CHECK(begin == end);
	}
}

void
GPlatesUnitTest::SmartNodeLinkedListTest::test_list_scoping()
{
	GPlatesUtils::SmartNodeLinkedList<int> empty_list(-1);
	GPlatesUtils::SmartNodeLinkedList<int>::iterator begin = empty_list.begin();
	GPlatesUtils::SmartNodeLinkedList<int>::iterator end = empty_list.end();
	BOOST_CHECK(begin == end);
	
	add_node_1(empty_list);
	
	//after returned form add_node_1, the list is still empty
	begin = empty_list.begin();
	end = empty_list.end();
	BOOST_CHECK(begin == end);
	
}
void
GPlatesUnitTest::SmartNodeLinkedListTest::test_increment_decrement_and_operator_arrow()
{
	A a12 = {1, 2};
	GPlatesUtils::SmartNodeLinkedList<A> list(a12);//empty list
	GPlatesUtils::SmartNodeLinkedList<A>::iterator begin = list.begin();
	GPlatesUtils::SmartNodeLinkedList<A>::iterator end = list.end();
	BOOST_CHECK(begin == end);
	
	A a34 = {3, 4};
	GPlatesUtils::SmartNodeLinkedList<A>::Node node(a34);
	list.append(node);
	BOOST_CHECK(list.begin()->i == 3);
	BOOST_CHECK(list.begin()->j == 4);

	list.begin()->i = 5;
	BOOST_CHECK(list.begin()->i == 5);
	BOOST_CHECK(list.begin()->j == 4);

	GPlatesUtils::SmartNodeLinkedList<A>::iterator iter = list.end();
	(--iter)->j = 6;
	BOOST_CHECK(list.begin()->i == 5);
	BOOST_CHECK(list.begin()->j == 6);
	

	----iter;
	iter->i = 7;
	BOOST_CHECK(list.begin()->i == 7);
	BOOST_CHECK(list.begin()->j == 6);

	iter++;
	iter->i = 8;
	BOOST_CHECK(list.begin()->i == 7);
	BOOST_CHECK(list.begin()->j == 6);
	BOOST_CHECK(iter->i == 8);
	
	iter++;
	iter->i = 9;
	BOOST_CHECK(list.begin()->i == 9);
	BOOST_CHECK(list.begin()->j == 6);
}



