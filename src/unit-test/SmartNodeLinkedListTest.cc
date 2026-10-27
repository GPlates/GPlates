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
#include <gtest/gtest.h>

#include "utils/SmartNodeLinkedList.h"

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
		EXPECT_TRUE(*begin == 1);
		begin++;
		EXPECT_TRUE(*begin == 2);
		begin++;
		EXPECT_TRUE(begin == end);;

		std::unique_ptr<Node> node_3_ptr(new Node(3));
		list.append(*node_3_ptr);

		begin = list.begin();
		end = list.end();
		EXPECT_TRUE(*begin == 1);
		begin++;
		EXPECT_TRUE(*begin == 2);
		begin++;
		EXPECT_TRUE(*begin == 3);
		begin++;
		EXPECT_TRUE(begin == end);

		return node_3_ptr;
	}


	std::unique_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node>
	add_node_2(
			GPlatesUtils::SmartNodeLinkedList<int> &list)
	{
		GPlatesUtils::SmartNodeLinkedList<int>::iterator begin = list.begin();
		GPlatesUtils::SmartNodeLinkedList<int>::iterator end = list.end();
		EXPECT_TRUE(*begin == 1);
		begin++;
		EXPECT_TRUE(begin == end);


		GPlatesUtils::SmartNodeLinkedList<int>::Node node_2(2);
		list.append(node_2);

		begin = list.begin();
		end = list.end();
		EXPECT_TRUE(*begin == 1);
		begin++;
		EXPECT_TRUE(*begin == 2);
		begin++;
		EXPECT_TRUE(begin == end);

		std::unique_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node> node_3_ptr = add_node_3(list);

		begin = list.begin();
		end = list.end();
		EXPECT_TRUE(*begin == 1);
		begin++;
		EXPECT_TRUE(*begin == 2);
		begin++;
		EXPECT_TRUE(*begin == 3);
		begin++;
		EXPECT_TRUE(begin == end);

		return node_3_ptr;
	}


	void
	invoke_add_node_2(
			GPlatesUtils::SmartNodeLinkedList<int> &list)
	{
		GPlatesUtils::SmartNodeLinkedList<int>::iterator begin = list.begin();
		GPlatesUtils::SmartNodeLinkedList<int>::iterator end = list.end();
		EXPECT_TRUE(*begin == 1);
		begin++;
		EXPECT_TRUE(begin == end);

		std::unique_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node> node_3_ptr = add_node_2(list);

		begin = list.begin();
		end = list.end();
		EXPECT_TRUE(*begin == 1);
		begin++;
		EXPECT_TRUE(*begin == 3);
		begin++;
		EXPECT_TRUE(begin == end);
	}

	void
	add_node_1(
			GPlatesUtils::SmartNodeLinkedList<int> &list)
	{
		GPlatesUtils::SmartNodeLinkedList<int>::iterator begin = list.begin();
		GPlatesUtils::SmartNodeLinkedList<int>::iterator end = list.end();
		EXPECT_TRUE(begin == end);

		GPlatesUtils::SmartNodeLinkedList<int>::Node node_1(1);
		list.append(node_1);

		begin = list.begin();
		end = list.end();
		EXPECT_TRUE(*begin == 1);
		begin++;
		EXPECT_TRUE(begin == end);

		invoke_add_node_2(list);

		begin = list.begin();
		end = list.end();
		EXPECT_TRUE(*begin == 1);
		begin++;
		EXPECT_TRUE(begin == end);
	}
}

TEST(SmartNodeLinkedListTest, list_scoping)
{
	GPlatesUtils::SmartNodeLinkedList<int> empty_list(-1);
	GPlatesUtils::SmartNodeLinkedList<int>::iterator begin = empty_list.begin();
	GPlatesUtils::SmartNodeLinkedList<int>::iterator end = empty_list.end();
	EXPECT_TRUE(begin == end);

	add_node_1(empty_list);

	//after returned form add_node_1, the list is still empty
	begin = empty_list.begin();
	end = empty_list.end();
	EXPECT_TRUE(begin == end);

}
TEST(SmartNodeLinkedListTest, increment_decrement_and_operator_arrow)
{
	A a12 = {1, 2};
	GPlatesUtils::SmartNodeLinkedList<A> list(a12);//empty list
	GPlatesUtils::SmartNodeLinkedList<A>::iterator begin = list.begin();
	GPlatesUtils::SmartNodeLinkedList<A>::iterator end = list.end();
	EXPECT_TRUE(begin == end);

	A a34 = {3, 4};
	GPlatesUtils::SmartNodeLinkedList<A>::Node node(a34);
	list.append(node);
	EXPECT_TRUE(list.begin()->i == 3);
	EXPECT_TRUE(list.begin()->j == 4);

	list.begin()->i = 5;
	EXPECT_TRUE(list.begin()->i == 5);
	EXPECT_TRUE(list.begin()->j == 4);

	GPlatesUtils::SmartNodeLinkedList<A>::iterator iter = list.end();
	(--iter)->j = 6;
	EXPECT_TRUE(list.begin()->i == 5);
	EXPECT_TRUE(list.begin()->j == 6);


	----iter;
	iter->i = 7;
	EXPECT_TRUE(list.begin()->i == 7);
	EXPECT_TRUE(list.begin()->j == 6);

	iter++;
	iter->i = 8;
	EXPECT_TRUE(list.begin()->i == 7);
	EXPECT_TRUE(list.begin()->j == 6);
	EXPECT_TRUE(iter->i == 8);

	iter++;
	iter->i = 9;
	EXPECT_TRUE(list.begin()->i == 9);
	EXPECT_TRUE(list.begin()->j == 6);
}
