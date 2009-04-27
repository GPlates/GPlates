/* $Id$ */

/**
 * @file
 * An external test program for class SmartNodeLinkedList.
 *
 * Compile with:
 * @code
 * g++ -Wall -ansi -pedantic SmartNodeLinkedList_test.cc
 * @endcode
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "SmartNodeLinkedList.h"
#include <memory>  // std::auto_ptr
#include <iostream>


template<typename T>
void
print_list(
		GPlatesUtils::SmartNodeLinkedList<T> &list)
{
	typename GPlatesUtils::SmartNodeLinkedList<T>::iterator iter = list.begin();
	typename GPlatesUtils::SmartNodeLinkedList<T>::iterator end = list.end();
	std::cout << "list is [ ";
	for ( ; iter != end; ++iter) {
		std::cout << *iter << ", ";
	}
	std::cout << "]" << std::endl;
}


std::auto_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node>
add_node_3(
		GPlatesUtils::SmartNodeLinkedList<int> &list)
{
	typedef GPlatesUtils::SmartNodeLinkedList<int>::Node Node;

	std::cout << "Have entered function 'add_node_3': ";
	print_list(list);

	std::auto_ptr<Node> node_3_ptr(new Node(3));
	list.append(*node_3_ptr);
	std::cout << "Have appended '*node_3_ptr': ";
	print_list(list);

	return node_3_ptr;
}


std::auto_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node>
add_node_2(
		GPlatesUtils::SmartNodeLinkedList<int> &list)
{
	std::cout << "Have entered function 'add_node_2': ";
	print_list(list);

	GPlatesUtils::SmartNodeLinkedList<int>::Node node_2(2);
	list.append(node_2);
	std::cout << "Have appended 'node_2': ";
	print_list(list);

	std::auto_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node> node_3_ptr = add_node_3(list);
	std::cout << "Have returned from function 'add_node_3': ";
	print_list(list);

	return node_3_ptr;
}


void
invoke_add_node_2(
		GPlatesUtils::SmartNodeLinkedList<int> &list)
{
	std::cout << "Have entered function 'invoke_add_node_2': ";
	print_list(list);

	std::auto_ptr<GPlatesUtils::SmartNodeLinkedList<int>::Node> node_3_ptr = add_node_2(list);
	std::cout << "Have returned from function 'add_node_2': ";
	print_list(list);
}


void
add_node_1(
		GPlatesUtils::SmartNodeLinkedList<int> &list)
{
	std::cout << "Have entered function 'add_node_1': ";
	print_list(list);

	GPlatesUtils::SmartNodeLinkedList<int>::Node node_1(1);
	list.append(node_1);
	std::cout << "Have appended 'node_1': ";
	print_list(list);

	invoke_add_node_2(list);
	std::cout << "Have returned from function 'invoke_add_node_2': ";
	print_list(list);
}


void
test_list_scoping()
{
	std::cout << "\nTesting list node scoping...\n";

	GPlatesUtils::SmartNodeLinkedList<int> empty_list(-1);
	std::cout << "Empty list: ";
	print_list(empty_list);

	add_node_1(empty_list);
	std::cout << "Have returned from function 'add_node_1': ";
	print_list(empty_list);
}


struct A
{
	int i;
	int j;
};


std::ostream &
operator<<(
		std::ostream &os,
		const A &a)
{
	os << "A(" << a.i << ", " << a.j << ")";
	return os;
}


void
test_increment_decrement_and_operator_arrow()
{
	std::cout << "\nTesting increment, decrement and operator->...\n";

	A a12 = {1, 2};
	GPlatesUtils::SmartNodeLinkedList<A> list(a12);
	std::cout << "Empty list: ";
	print_list(list);

	A a34 = {3, 4};
	GPlatesUtils::SmartNodeLinkedList<A>::Node node(a34);
	list.append(node);
	std::cout << "Appending a node: ";
	print_list(list);

	list.begin()->i = 5;
	std::cout << "list.begin()->i = 5; ";
	print_list(list);

	GPlatesUtils::SmartNodeLinkedList<A>::iterator iter = list.end();
	(--iter)->j = 6;
	std::cout << "iter = list.end(); (--iter)->j = 6; ";
	print_list(list);

	----iter;
	iter->i = 7;
	std::cout << "----iter; iter->i = 7; ";
	print_list(list);

	iter++;
	iter->i = 8;
	std::cout << "iter++;  /* iter points at sentinel */  iter->i = 8; ";
	print_list(list);

	iter++;
	iter->i = 9;
	std::cout << "iter++;  /* iter points at node */  iter->i = 9; ";
	print_list(list);
}


int
main()
{
	test_list_scoping();
	test_increment_decrement_and_operator_arrow();

	return 0;
}
