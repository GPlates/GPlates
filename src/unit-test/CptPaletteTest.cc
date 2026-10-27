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
#include <string>

#include <QDebug>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>
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


namespace
{
	/**
	 * Writes @a contents to a file named @a file_name inside @a temp_dir, and returns its path.
	 */
	QString
	write_cpt_file(
			const QTemporaryDir &temp_dir,
			const QString &file_name,
			const QString &contents)
	{
		const QString file_path = temp_dir.filePath(file_name);

		QFile file(file_path);
		EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
		QTextStream(&file) << contents;

		return file_path;
	}


	/**
	 * A CPT colouring everything green when read as HSV (hue 120 degrees, fully saturated),
	 * preceded by @a colour_model_comment - which is what selects the colour model those three
	 * numbers are read in.
	 */
	QString
	cpt_file_contents(
			const QString &colour_model_comment)
	{
		return colour_model_comment + "\n0 120 1 1 100 120 1 1\n";
	}


	//! Returns the "#rrggbb" name of the colour @a palette gives @a key, for a legible failure.
	std::string
	colour_name_at(
			const GPlatesGui::CptPalette &palette,
			long key)
	{
		const boost::optional<GPlatesGui::Colour> colour =
				palette.get_colour(GPlatesGui::Palette::Key(key));
		if (!colour)
		{
			return "<no colour>";
		}

		return GPlatesGui::qcolor_from_colour(*colour).name().toStdString();
	}
}


TEST(CptPaletteTest, colour_model_comment_selects_hsv_whatever_its_case)
{
	using namespace GPlatesGui;

	QTemporaryDir temp_dir;
	ASSERT_TRUE(temp_dir.isValid());

	// GMT writes this comment in lower case, but CPT files exist carrying it in any case, so
	// every one of these selects the HSV colour model.
	const QString hsv_comments[] = {
			"# COLOR_MODEL = hsv",
			"# COLOR_MODEL = HSV",
			"# COLOR_MODEL = +hsv",
			"#COLOR_MODEL=HsV"
	};

	int index = 0;
	for (const QString &hsv_comment : hsv_comments)
	{
		const QString file_path = write_cpt_file(
				temp_dir,
				QString("hsv_%1.cpt").arg(index++),
				cpt_file_contents(hsv_comment));

		EXPECT_EQ("#00ff00", colour_name_at(CptPalette(file_path), 50))
				<< "for colour model comment: " << hsv_comment.toStdString();
	}
}


TEST(CptPaletteTest, colour_model_defaults_to_rgb)
{
	using namespace GPlatesGui;

	QTemporaryDir temp_dir;
	ASSERT_TRUE(temp_dir.isValid());

	// Without a COLOR_MODEL comment the same three numbers are read as RGB...
	const QString rgb_file_path = write_cpt_file(
			temp_dir, "rgb.cpt", cpt_file_contents("# no colour model stated here"));
	EXPECT_EQ("#780101", colour_name_at(CptPalette(rgb_file_path), 50));

	// ...as they are when the comment says the colour model is *not* HSV. (The CPT file in
	// 'unit-test/data/cpt' carries exactly that comment.)
	const QString not_hsv_file_path = write_cpt_file(
			temp_dir, "not_hsv.cpt", cpt_file_contents("# COLOR_MODEL != HSV"));
	EXPECT_EQ("#780101", colour_name_at(CptPalette(not_hsv_file_path), 50));
}
