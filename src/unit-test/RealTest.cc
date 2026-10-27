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

#include "maths/Real.h"

TEST(RealTest, positive_infinity)
{
	EXPECT_TRUE(GPlatesMaths::is_infinity(GPlatesMaths::positive_infinity<double>()));
	EXPECT_TRUE(GPlatesMaths::is_positive_infinity(GPlatesMaths::positive_infinity<double>()));
	EXPECT_TRUE(!GPlatesMaths::is_negative_infinity(GPlatesMaths::positive_infinity<double>()));
	EXPECT_TRUE(!GPlatesMaths::is_nan(GPlatesMaths::positive_infinity<double>()));
}


TEST(RealTest, negative_infinity)
{
	EXPECT_TRUE(GPlatesMaths::is_infinity(GPlatesMaths::negative_infinity<double>()));
	EXPECT_TRUE(!GPlatesMaths::is_positive_infinity(GPlatesMaths::negative_infinity<double>()));
	EXPECT_TRUE(GPlatesMaths::is_negative_infinity(GPlatesMaths::negative_infinity<double>()));
	EXPECT_TRUE(!GPlatesMaths::is_nan(GPlatesMaths::negative_infinity<double>()));
}

TEST(RealTest, nan)
{
	EXPECT_TRUE(!GPlatesMaths::is_infinity(GPlatesMaths::quiet_nan<double>()));
	EXPECT_TRUE(!GPlatesMaths::is_positive_infinity(GPlatesMaths::quiet_nan<double>()));
	EXPECT_TRUE(!GPlatesMaths::is_negative_infinity(GPlatesMaths::quiet_nan<double>()));
	EXPECT_TRUE(GPlatesMaths::is_nan(GPlatesMaths::quiet_nan<double>()));
}

TEST(RealTest, zero)
{
	EXPECT_TRUE(!GPlatesMaths::is_infinity(0.0));
	EXPECT_TRUE(!GPlatesMaths::is_positive_infinity(0.0));
	EXPECT_TRUE(!GPlatesMaths::is_negative_infinity(0.0));
	EXPECT_TRUE(!GPlatesMaths::is_nan(0.0));
}
