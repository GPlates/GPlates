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
#include <gtest/gtest.h>

#include "gui/ColourQt.h"
#include "gui/Palette.h"

TEST(CptPaletteTest, cpt_palette)
{
	using namespace GPlatesGui;
	std::cout << "testing cpt palette" << std::endl;

	CptPalette cpt_palette(GPLATES_UNIT_TEST_DATA_DIR "/cpt/cpt_unit_test.txt");

	boost::optional<Colour> c = cpt_palette.get_colour(Palette::Key(125));
	EXPECT_TRUE(GPlatesGui::qcolor_from_colour(*c).name().toStdString()=="#ffa500");
	std::cout << GPlatesGui::qcolor_from_colour(*c).name().toStdString() << std::endl;

	c = cpt_palette.get_colour(Palette::Key(801));
	EXPECT_TRUE(GPlatesGui::qcolor_from_colour(*c).name().toStdString()=="#00ff00");
	std::cout << GPlatesGui::qcolor_from_colour(*c).name().toStdString() << std::endl;

	c = cpt_palette.get_colour(Palette::Key(600));
	EXPECT_TRUE(GPlatesGui::qcolor_from_colour(*c).name().toStdString()=="#568a4d");
	std::cout << GPlatesGui::qcolor_from_colour(*c).name().toStdString() << std::endl;

	c = cpt_palette.get_colour(Palette::Key(450));
	EXPECT_TRUE(GPlatesGui::qcolor_from_colour(*c).name().toStdString()=="#bfadaf");
	std::cout << GPlatesGui::qcolor_from_colour(*c).name().toStdString() << std::endl;

	c = cpt_palette.get_colour(Palette::Key(800));
	EXPECT_TRUE(GPlatesGui::qcolor_from_colour(*c).name().toStdString()=="#d38d7c");
	std::cout << GPlatesGui::qcolor_from_colour(*c).name().toStdString() << std::endl;

	Colour b, f, n;
	boost::tie(b,f,n) = cpt_palette.get_BFN_colour();
	std::cout << GPlatesGui::qcolor_from_colour(b).name().toStdString() << std::endl;
	EXPECT_TRUE(GPlatesGui::qcolor_from_colour(b).name().toStdString()=="#000000");
	std::cout << GPlatesGui::qcolor_from_colour(f).name().toStdString() << std::endl;
	EXPECT_TRUE(GPlatesGui::qcolor_from_colour(f).name().toStdString()=="#ffffff");
	std::cout << GPlatesGui::qcolor_from_colour(n).name().toStdString() << std::endl;
	EXPECT_TRUE(GPlatesGui::qcolor_from_colour(n).name().toStdString()=="#808080");
}
