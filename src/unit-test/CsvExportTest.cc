/**
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

#include <vector>
#include <gtest/gtest.h>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>

#include "gui/CsvExport.h"


namespace
{
	/**
	 * Non-ASCII text, as explicit UTF-8 bytes rather than as characters in this source file.
	 *
	 * The build does not tell MSVC that its sources are UTF-8 (there is no '/utf-8'), so a
	 * character written literally here would be decoded using the compiler's own code page.
	 */
	const QString
	non_ascii_text()
	{
		// 测试-Ünïcode
		// Note: The literal is split because a hex escape is greedy - "\xAFcode" would
		//       otherwise continue into the 'c', which is also a hex digit.
		return QString::fromUtf8("\xE6\xB5\x8B\xE8\xAF\x95-\xC3\x9Cn\xC3\xAF" "code");
	}

	/**
	 * The line ending that QIODevice::Text writes, which is what the std::ofstream text mode
	 * that CsvExport used to open its files with wrote.
	 */
	const QByteArray
	native_line_ending()
	{
#if defined(Q_OS_WIN)
		return QByteArray("\r\n");
#else
		return QByteArray("\n");
#endif
	}

	/**
	 * Returns the contents of @a filename as the bytes it actually contains.
	 *
	 * Note: The file is *not* opened with QIODevice::Text, so that the line endings written
	 * are visible to the caller rather than translated back.
	 */
	QByteArray
	read_file(
			const QString &filename)
	{
		QFile file(filename);
		EXPECT_TRUE(file.open(QIODevice::ReadOnly));
		return file.readAll();
	}
}


TEST(CsvExportTest, exports_to_a_non_ascii_path)
{
	// A directory and a filename that both contain characters outside any single-byte code page.
	//
	// Note: The non-ASCII names are created here rather than committed to the repository, since
	// a filename is not stored identically by every filesystem (macOS decomposes it, for one).
	QTemporaryDir temp_dir;
	ASSERT_TRUE(temp_dir.isValid());
	const QString directory = temp_dir.filePath(non_ascii_text());
	ASSERT_TRUE(QDir().mkpath(directory));
	const QString filename = directory + "/" + non_ascii_text() + ".csv";

	GPlatesGui::CsvExport::ExportOptions options;
	options.delimiter = ',';

	std::vector<GPlatesGui::CsvExport::LineDataType> data;

	GPlatesGui::CsvExport::LineDataType header;
	header.push_back("plate id");
	header.push_back(non_ascii_text());
	data.push_back(header);

	GPlatesGui::CsvExport::LineDataType row;
	row.push_back("801");
	row.push_back(non_ascii_text());
	data.push_back(row);

	// Note: export_data() reports a failure to the user with a message box rather than by
	// throwing, since it is called from the GUI. So if this ever regresses on a platform where
	// the path cannot be opened, the test blocks in that modal dialog and fails by exceeding the
	// timeout that 'gtest_discover_tests' sets, rather than failing on the checks below.
	GPlatesGui::CsvExport::export_data(filename, options, data);

	// The file is named by the path asked for...
	ASSERT_TRUE(QFile::exists(filename));

	// ...and contains the data encoded as UTF-8, with the platform line ending.
	//
	// Note: This is compared as bytes because the encoding is part of what is being tested: a
	// QTextStream writes the locale encoding unless told otherwise, so exporting the same data
	// on two machines would otherwise not produce the same file.
	QByteArray expected;
	expected += QByteArray("plate id,") + non_ascii_text().toUtf8() + native_line_ending();
	expected += QByteArray("801,") + non_ascii_text().toUtf8() + native_line_ending();

	EXPECT_EQ(expected, read_file(filename));
}


TEST(CsvExportTest, quotes_only_the_fields_that_need_quoting)
{
	// CsvExport::export_line() writes to a QTextStream, which need not be a file.
	QString line;
	QTextStream os(&line);

	GPlatesGui::CsvExport::ExportOptions options;
	options.delimiter = ',';

	GPlatesGui::CsvExport::LineDataType line_data;
	line_data.push_back("801");						// Nothing to quote.
	line_data.push_back(non_ascii_text());			// Non-ASCII is not a reason to quote.
	line_data.push_back("a,b");						// Contains the delimiter.
	line_data.push_back("say \"hello\"");			// Contains quotes, which are doubled.
	line_data.push_back(" leading space");			// Would otherwise be trimmed by a reader.

	GPlatesGui::CsvExport::export_line(os, options, line_data);

	const QString expected =
			"801," +
			non_ascii_text() + "," +
			"\"a,b\"," +
			"\"say \"\"hello\"\"\"," +
			"\" leading space\"\n";

	EXPECT_EQ(expected, line);
}
