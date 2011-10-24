/* $Id:  $ */

/**
 * \file 
 * $Revision: 7584 $
 * $Date: 2010-02-10 19:29:36 +1100 (Wed, 10 Feb 2010) $
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
#include "gui/Palette.h"
#include "unit-test/CptPaletteTest.h"

//copy the following code into directory level test suite file
//for example, if the test class is in data-mining directory, the following code will
//be copied to DataMiningTestSuite.cc
//#include "unit-test/CptPaletteTest.h"
//ADD_TESTSUITE(CptPalette);

//./gplates-unit-test --detect_memory_leaks=0 --G_test_to_run=*/Coreg

GPlatesUnitTest::CptPaletteTestSuite::CptPaletteTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"CptPaletteTestSuite")
{
	init(level);
} 

void 
GPlatesUnitTest::CptPaletteTest::test_case_1()
{
	using namespace GPlatesGui;
	std::cout << "testing cpt palette" << std::endl;

	Palette* cpt_palette = 
		new CptPalette("C:/gplates_src/symbology-2011-Jun-03/sample-data/unit-test-data/cpt_unit_test.txt");
	
	boost::optional<Colour> c = cpt_palette->get_colour(Palette::Key(125));
	BOOST_CHECK(QColor(*c).name().toStdString()=="#ffa500");
	std::cout << QColor(*c).name().toStdString() << std::endl;

	c = cpt_palette->get_colour(Palette::Key(801));
	BOOST_CHECK(QColor(*c).name().toStdString()=="#00ff00");
	std::cout << QColor(*c).name().toStdString() << std::endl;

	c = cpt_palette->get_colour(Palette::Key(600));
	BOOST_CHECK(QColor(*c).name().toStdString()=="#568a4d");
	std::cout << QColor(*c).name().toStdString() << std::endl;

	c = cpt_palette->get_colour(Palette::Key(450));
	BOOST_CHECK(QColor(*c).name().toStdString()=="#bfadaf");
	std::cout << QColor(*c).name().toStdString() << std::endl;

	c = cpt_palette->get_colour(Palette::Key(800));
	BOOST_CHECK(QColor(*c).name().toStdString()=="#d38d7c");
	std::cout << QColor(*c).name().toStdString() << std::endl;

	Colour b, f, n;
	boost::tie(b,f,n) = cpt_palette->get_BFN_colour();
	std::cout << QColor(b).name().toStdString() << std::endl;
	BOOST_CHECK(QColor(b).name().toStdString()=="#000000");
	std::cout << QColor(f).name().toStdString() << std::endl;
	BOOST_CHECK(QColor(f).name().toStdString()=="#ffffff");
	std::cout << QColor(n).name().toStdString() << std::endl;
	BOOST_CHECK(QColor(n).name().toStdString()=="#808080");
	
	delete cpt_palette;
	return;
}

void 
GPlatesUnitTest::CptPaletteTest::test_case_2()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CptPaletteTest::test_case_3()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CptPaletteTest::test_case_4()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CptPaletteTest::test_case_5()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CptPaletteTest::test_case_6()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CptPaletteTest::test_case_7()
{
	//Add you test code here
	return;
}

void
GPlatesUnitTest::CptPaletteTestSuite::construct_maps()
{
	boost::shared_ptr<CptPaletteTest> instance(
		new CptPaletteTest());

	ADD_TESTCASE(CptPaletteTest,test_case_1);
	ADD_TESTCASE(CptPaletteTest,test_case_2);
	ADD_TESTCASE(CptPaletteTest,test_case_3);
	ADD_TESTCASE(CptPaletteTest,test_case_4);
	ADD_TESTCASE(CptPaletteTest,test_case_5);
	ADD_TESTCASE(CptPaletteTest,test_case_6);
	ADD_TESTCASE(CptPaletteTest,test_case_7);
}

