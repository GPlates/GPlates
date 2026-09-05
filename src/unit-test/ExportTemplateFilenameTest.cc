/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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

#include <QDateTime>
#include <QString>
#include <gtest/gtest.h>

#include "file-io/ExportTemplateFilenameSequenceFormats.h"


//
// The printf-style reconstruction time format specifier ("%d", "%0.2f", ...) in an export
// template filename is recognised by a regular expression, so these tests pin the flags that
// regular expression accepts.
//
// Note that the '-' (left justify) flag really is expected to match: the regular expression
// used to spell its flag set '[ +-#0]', in which '+-#' is a character *range* rather than three
// literals, and so it never matched a '-' (and would be rejected outright by QRegularExpression).
// It is spelled '[-+ #0]' now.
//

using GPlatesFileIO::ExportTemplateFilename::ReconstructionTimePrintfFormat;


namespace
{
	//! Returns the length of the matched format specifier, or -1 if @a filename_template doesn't match.
	int
	match_length(
			const QString &filename_template)
	{
		const boost::optional<int> matched_length =
				ReconstructionTimePrintfFormat::match_format(filename_template);

		return matched_length ? matched_length.get() : -1;
	}


	//! Expands @a format_string at @a reconstruction_time (the date/time is unused by this format).
	QString
	expand(
			const QString &format_string,
			const double &reconstruction_time)
	{
		return ReconstructionTimePrintfFormat(format_string).expand_format_string(
				0/*sequence_index*/,
				reconstruction_time,
				QDateTime());
	}
}


TEST(ExportTemplateFilenameTest, match_format_accepts_each_printf_flag)
{
	// Each of the five flags (space, '+', '-', '#', '0'), and no flag at all.
	EXPECT_EQ(2, match_length("%d"));
	EXPECT_EQ(2, match_length("%f"));
	EXPECT_EQ(3, match_length("% d"));
	EXPECT_EQ(3, match_length("%+d"));
	EXPECT_EQ(4, match_length("%-5d"));
	EXPECT_EQ(3, match_length("%#f"));
	EXPECT_EQ(5, match_length("%0.2f"));
	EXPECT_EQ(6, match_length("%05.1f"));

	// Only the leading format specifier is matched - the rest of the template is left alone.
	EXPECT_EQ(2, match_length("%d_velocity"));
}


TEST(ExportTemplateFilenameTest, match_format_rejects_other_specifiers)
{
	// Only the 'd' and 'f' specifiers are supported.
	EXPECT_EQ(-1, match_length("%s"));
	EXPECT_EQ(-1, match_length("%x"));
	EXPECT_EQ(-1, match_length("%"));
	EXPECT_EQ(-1, match_length("%ld"));

	// Not anchored at the start of the (remainder of the) template.
	EXPECT_EQ(-1, match_length("velocity_%d"));
}


TEST(ExportTemplateFilenameTest, expand_format_string)
{
	// A 'd' specifier rounds the reconstruction time to the nearest integer.
	EXPECT_EQ(QString("13"), expand("%d", 12.6));
	EXPECT_EQ(QString("12"), expand("%d", 12.4));

	// An 'f' specifier keeps the reconstruction time as a floating-point value.
	EXPECT_EQ(QString("12.50"), expand("%0.2f", 12.5));
	EXPECT_EQ(QString("012.5"), expand("%05.1f", 12.5));

	// The flags reach printf.
	EXPECT_EQ(QString("+13"), expand("%+d", 12.6));
	EXPECT_EQ(QString("13   "), expand("%-5d", 12.6));
	EXPECT_EQ(QString(" 13"), expand("% 3d", 12.6));
}
