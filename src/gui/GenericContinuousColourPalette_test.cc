/* $Id$ */

/**
 * \file 
 * Contains (rather ad hoc) testing for the GenericContinuousColourPalette class.
 *
 * Compile with:
 * g++ -o colour_test -I . -I /usr/include/qt4/QtGui -I /usr/include/qt4/QtCore -I /usr/include/qt4/ -l QtGui gui/GenericContinuousColourPalette_test.cc gui/Colour.cc gui/GenericContinuousColourPalette.cc maths/Real.cc global/AssertionFailureException.cc global/GPlatesException.cc utils/CallStackTracker.cc
 * in the src/ directory.
 *
 * $Revision$
 * $Date$ 
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

#include "GenericContinuousColourPalette.h"
#include <iostream>

using namespace GPlatesGui;
using namespace std;

void
test1()
{
	// 1 control point
	map<GPlatesMaths::Real, Colour> control_points;
	control_points.insert(make_pair(0, Colour::get_red()));
	GenericContinuousColourPalette palette(control_points);
	cout << "Test 1" << endl;
	cout << " -1: " << *palette.get_colour(-1) << endl;
	cout << "  0: " << *palette.get_colour(0) << endl;
	cout << "  1: " << *palette.get_colour(1) << endl;
	cout << endl;
}

void
test2()
{
	// 2 control points
	map<GPlatesMaths::Real, Colour> control_points;
	control_points.insert(make_pair(0, Colour::get_red()));
	control_points.insert(make_pair(1, Colour::get_green()));
	GenericContinuousColourPalette palette(control_points);
	cout << "Test 2" << endl;
	cout << " -1: " << *palette.get_colour(-1) << endl; 
	cout << "  0: " << *palette.get_colour(0) << endl;
	cout << "0.5: " << *palette.get_colour(0.5) << endl;
	cout << "  1: " << *palette.get_colour(1) << endl;
	cout << "  2: " << *palette.get_colour(2) << endl;
	cout << endl;
}

void
test3()
{
	// 3 control points
	map<GPlatesMaths::Real, Colour> control_points;
	control_points.insert(make_pair(0, Colour::get_red()));
	control_points.insert(make_pair(1, Colour::get_green()));
	control_points.insert(make_pair(3, Colour::get_blue()));
	GenericContinuousColourPalette palette(control_points);
	cout << "Test 3" << endl;
	cout << " -1: " << *palette.get_colour(-1) << endl; 
	cout << "  0: " << *palette.get_colour(0) << endl;
	cout << "0.5: " << *palette.get_colour(0.5) << endl;
	cout << "  1: " << *palette.get_colour(1) << endl;
	cout << "  2: " << *palette.get_colour(2) << endl;
	cout << "  3: " << *palette.get_colour(3) << endl;
	cout << "  4: " << *palette.get_colour(4) << endl;
	cout << endl;
}

int
main()
{
	test1();
	test2();
	test3();

	return 0;
}

