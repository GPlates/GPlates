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

#include <QString>

#include "model/FeatureType.h"

#include "unit-test/StringSetTest.h"

#include "utils/StringSet.h"


GPlatesUnitTest::StringSetTestSuite::StringSetTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"StringSetTestSuite")
{
	init(level);
}


void
GPlatesUnitTest::StringSetTestSuite::construct_maps()
{
	boost::shared_ptr<StringSetTest> instance(new StringSetTest());

	ADD_TESTCASE(StringSetTest, equality_test);
}


void
GPlatesUnitTest::StringSetTest::equality_test()
{
	using GPlatesUtils::StringSet;
	typedef StringSet::SharedIterator SharedIterator;

	StringSet string_set;

	SharedIterator a = string_set.insert("a");
	SharedIterator b = string_set.insert("a");
	BOOST_CHECK(a == b);

	using GPlatesModel::FeatureType;

	FeatureType foo(UnicodeString("gpml"), UnicodeString("Foo"));
	FeatureType foo2(QString("gpml"), QString("Foo"));
	BOOST_CHECK(foo == foo2);
}

