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

#include "unit-test/GenericContinuousColourPaletteTest.h"
#include "maths/MathsUtils.h"
#include "maths/Real.h"
#include "gui/Colour.h"

using GPlatesMaths::are_almost_exactly_equal;

GPlatesUnitTest::GenericContinuousColourPaletteTestSuite::GenericContinuousColourPaletteTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"GenericContinuousColourPaletteTestSuite")
{
	init(level);
}

void
GPlatesUnitTest::GenericContinuousColourPaletteTest::test_control_points_1()
{
#if 0
	std::map<GPlatesMaths::Real, GPlatesGui::Colour> control_points;
	control_points.insert(std::make_pair(0, GPlatesGui::Colour::get_red()));
	
	GPlatesGui::GenericContinuousColourPalette palette(control_points);
	
	boost::optional<GPlatesGui::Colour> colour;
	
	colour = palette.get_colour(-1);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red(), 1));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(0);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 1));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(1);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 1));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));
#endif
}

void
GPlatesUnitTest::GenericContinuousColourPaletteTest::test_control_points_2()
{
#if 0
	std::map<GPlatesMaths::Real, GPlatesGui::Colour> control_points;
	
	control_points.insert(std::make_pair(0, GPlatesGui::Colour::get_red()));
	control_points.insert(std::make_pair(1, GPlatesGui::Colour::get_green()));
	
	GPlatesGui::GenericContinuousColourPalette palette(control_points);
	
	boost::optional<GPlatesGui::Colour> colour;

	colour = palette.get_colour(-1);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 1));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(0);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 1));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(0.5);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 0.5));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0.25));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(1);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0.5));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(2);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0.5));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));
#endif
}

void
GPlatesUnitTest::GenericContinuousColourPaletteTest::test_control_points_3()
{
#if 0
	std::map<GPlatesMaths::Real, GPlatesGui::Colour> control_points;
	
	control_points.insert(std::make_pair(0, GPlatesGui::Colour::get_red()));
	control_points.insert(std::make_pair(1, GPlatesGui::Colour::get_green()));
	control_points.insert(std::make_pair(3, GPlatesGui::Colour::get_blue()));
	
	GPlatesGui::GenericContinuousColourPalette palette(control_points);
	
	
	boost::optional<GPlatesGui::Colour> colour;

	colour = palette.get_colour(-1);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 1));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(0);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 1));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(0.5);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 0.5));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0.25));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(1);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0.5));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(2);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0.25));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 0.5));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(3);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 1));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));

	colour = palette.get_colour(4);
	BOOST_CHECK(colour);
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->red() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->green() , 0));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->blue() , 1));
	BOOST_CHECK(are_almost_exactly_equal<float>(colour->alpha() , 1));
#endif
}

void
GPlatesUnitTest::GenericContinuousColourPaletteTestSuite::construct_maps()
{
#if 0
	boost::shared_ptr<GenericContinuousColourPaletteTest> instance(
		new GenericContinuousColourPaletteTest());

	ADD_TESTCASE(GenericContinuousColourPaletteTest,test_control_points_1);
	ADD_TESTCASE(GenericContinuousColourPaletteTest,test_control_points_2);
	ADD_TESTCASE(GenericContinuousColourPaletteTest,test_control_points_3);
#endif
}

