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

#include <cstring> // for memcmp, memcpy
#include <iostream>

#include <QDebug>

#include "MipmapperTest.h"

#include "gui/Colour.h"
#include "gui/Mipmapper.h"

#include "property-values/RawRaster.h"


using GPlatesGui::rgba8_t;
using namespace GPlatesPropertyValues;


GPlatesUnitTest::MipmapperTestSuite::MipmapperTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"MipmapperTestSuite")
{
	init(level);
}


void
GPlatesUnitTest::MipmapperTestSuite::construct_maps()
{
	boost::shared_ptr<MipmapperTest> instance(
		new MipmapperTest());

	ADD_TESTCASE(MipmapperTest, test_extend_raster1);
	ADD_TESTCASE(MipmapperTest, test_extend_raster2);
	ADD_TESTCASE(MipmapperTest, test_extend_raster3);
	ADD_TESTCASE(MipmapperTest, test_extend_raster4);
	ADD_TESTCASE(MipmapperTest, test_rgba_mipmapper);
}


void
GPlatesUnitTest::MipmapperTest::test_extend_raster1()
{
	// Test 1: even width, even height.
	// Because the raster already has even width/height, extending it should do nothing.
	static const int SIZE = 2;
	Int32RawRaster::non_null_ptr_type raster = Int32RawRaster::create(SIZE, SIZE);
	boost::int32_t *buf = raster->data();
	for (int i = 0; i != SIZE * SIZE; ++i)
	{
		*(buf + i) = i;
	}

	Int32RawRaster::non_null_ptr_to_const_type result = GPlatesGui::MipmapperInternals::extend_raster(*raster);

	BOOST_CHECK(std::memcmp(raster->data(), result->data(), SIZE * SIZE * sizeof(boost::int32_t)) == 0);
}


void
GPlatesUnitTest::MipmapperTest::test_extend_raster2()
{
	// Test 2: odd width, even height.
	// After extending, it should be one pixel wider.
	Rgba8RawRaster::non_null_ptr_type raster = Rgba8RawRaster::create(3, 2);
	rgba8_t raster_data[] = {
		/* 1st row */ rgba8_t(0, 10, 20, 30), rgba8_t(1, 11, 21, 31), rgba8_t(2, 12, 22, 32),
		/* 2nd row */ rgba8_t(40, 50, 60, 70), rgba8_t(41, 51, 61, 71), rgba8_t(42, 52, 62, 72)
	};
	std::memcpy(raster->data(), raster_data, sizeof(raster_data));
	
	rgba8_t expected_result[] = {
		/* 1st row */ rgba8_t(0, 10, 20, 30), rgba8_t(1, 11, 21, 31), rgba8_t(2, 12, 22, 32), rgba8_t(2, 12, 22, 32),
		/* 2nd row */ rgba8_t(40, 50, 60, 70), rgba8_t(41, 51, 61, 71), rgba8_t(42, 52, 62, 72), rgba8_t(42, 52, 62, 72)
	};

	Rgba8RawRaster::non_null_ptr_to_const_type result = GPlatesGui::MipmapperInternals::extend_raster(*raster);

	BOOST_CHECK(std::memcmp(expected_result, result->data(), 8 * sizeof(rgba8_t)) == 0);
}


void
GPlatesUnitTest::MipmapperTest::test_extend_raster3()
{
	// Test 3: even width, odd height.
	// After extending, it should be one pixel higher.
	Rgba8RawRaster::non_null_ptr_type raster = Rgba8RawRaster::create(2, 3);
	rgba8_t raster_data[] = {
		/* 1st row */ rgba8_t(0, 10, 20, 30), rgba8_t(1, 11, 21, 31),
		/* 2nd row */ rgba8_t(40, 50, 60, 70), rgba8_t(41, 51, 61, 71),
		/* 3rd row */ rgba8_t(80, 90, 100, 110), rgba8_t(81, 91, 101, 111)
	};
	std::memcpy(raster->data(), raster_data, sizeof(raster_data));

	rgba8_t expected_result[] = {
		/* 1st row */ rgba8_t(0, 10, 20, 30), rgba8_t(1, 11, 21, 31),
		/* 2nd row */ rgba8_t(40, 50, 60, 70), rgba8_t(41, 51, 61, 71),
		/* 3rd row */ rgba8_t(80, 90, 100, 110), rgba8_t(81, 91, 101, 111),
		/* 4th row */ rgba8_t(80, 90, 100, 110), rgba8_t(81, 91, 101, 111)
	};

	Rgba8RawRaster::non_null_ptr_to_const_type result = GPlatesGui::MipmapperInternals::extend_raster(*raster);

	BOOST_CHECK(std::memcmp(expected_result, result->data(), 8 * sizeof(rgba8_t)) == 0);
}


void
GPlatesUnitTest::MipmapperTest::test_extend_raster4()
{
	// Test 4: odd width, odd height.
	// After extending, it should be one pixel wider and higher.
	Rgba8RawRaster::non_null_ptr_type raster = Rgba8RawRaster::create(3, 3);
	rgba8_t raster_data[] = {
		/* 1st row */ rgba8_t(0, 10, 20, 30), rgba8_t(1, 11, 21, 31), rgba8_t(2, 12, 22, 32),
		/* 2nd row */ rgba8_t(40, 50, 60, 70), rgba8_t(41, 51, 61, 71), rgba8_t(42, 52, 62, 72),
		/* 3rd row */ rgba8_t(80, 90, 100, 110), rgba8_t(81, 91, 101, 111), rgba8_t(82, 92, 102, 112)
	};
	std::memcpy(raster->data(), raster_data, sizeof(raster_data));

	rgba8_t expected_result[] = {
		/* 1st row */ rgba8_t(0, 10, 20, 30), rgba8_t(1, 11, 21, 31), rgba8_t(2, 12, 22, 32), rgba8_t(2, 12, 22, 32),
		/* 2nd row */ rgba8_t(40, 50, 60, 70), rgba8_t(41, 51, 61, 71), rgba8_t(42, 52, 62, 72), rgba8_t(42, 52, 62, 72),
		/* 3rd row */ rgba8_t(80, 90, 100, 110), rgba8_t(81, 91, 101, 111), rgba8_t(82, 92, 102, 112), rgba8_t(82, 92, 102, 112),
		/* 4th row */ rgba8_t(80, 90, 100, 110), rgba8_t(81, 91, 101, 111), rgba8_t(82, 92, 102, 112), rgba8_t(82, 92, 102, 112)
	};

	Rgba8RawRaster::non_null_ptr_to_const_type result = GPlatesGui::MipmapperInternals::extend_raster(*raster);

	BOOST_CHECK(std::memcmp(expected_result, result->data(), 16 * sizeof(rgba8_t)) == 0);
}


void
GPlatesUnitTest::MipmapperTest::test_rgba_mipmapper()
{
	// Let's mipmap a 3x5 raster.
	Rgba8RawRaster::non_null_ptr_type raster = Rgba8RawRaster::create(5, 3);
	rgba8_t raster_data[] = {
		/* 1st row */ rgba8_t(0, 10, 20, 30), rgba8_t(1, 11, 21, 31), rgba8_t(2, 12, 22, 32), rgba8_t(3, 13, 23, 33), rgba8_t(4, 14, 24, 34),
		/* 2nd row */ rgba8_t(40, 50, 60, 70), rgba8_t(41, 51, 61, 71), rgba8_t(42, 52, 62, 72), rgba8_t(43, 53, 63, 73), rgba8_t(44, 54, 64, 74),
		/* 3rd row */ rgba8_t(80, 90, 100, 110), rgba8_t(81, 91, 101, 111), rgba8_t(82, 92, 102, 112), rgba8_t(83, 93, 103, 113), rgba8_t(84, 94, 104, 114)
	};
	std::memcpy(raster->data(), raster_data, sizeof(raster_data));

	GPlatesGui::Mipmapper<Rgba8RawRaster> mipmapper(raster);

	// Level 1 should be 2x3.
	BOOST_CHECK(
			mipmapper.generate_next() &&
			mipmapper.get_current_mipmap()->height() == 2 &&
			mipmapper.get_current_mipmap()->width() == 3);

#if 0
	const rgba8_t *bits = mipmapper.get_current_mipmap()->data();
	for (int i = 0; i != 6; ++i)
	{
		std::cout << *(bits + i) << std::endl;
	}
#endif

	// Level 2 should be 1x2.
	BOOST_CHECK(
			mipmapper.generate_next() &&
			mipmapper.get_current_mipmap()->height() == 1 &&
			mipmapper.get_current_mipmap()->width() == 2);

	// Level 3 should be 1x1.
	BOOST_CHECK(
			mipmapper.generate_next() &&
			mipmapper.get_current_mipmap()->height() == 1 &&
			mipmapper.get_current_mipmap()->width() == 1);

	// And there should be no more levels after that.
	BOOST_CHECK(!mipmapper.generate_next());
}

