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

#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <gtest/gtest.h>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

#include "utils/CommandLineParser.h"


namespace
{
	const char *FILE_OPTION_NAME = "file";

	/**
	 * Non-ASCII text, as explicit UTF-8 bytes rather than as characters in this source file.
	 *
	 * The build does not tell MSVC that its sources are UTF-8 (there is no '/utf-8'), so a
	 * character written literally here would be decoded using the compiler's own code page.
	 *
	 * These characters are outside code page 1252, so on Windows they cannot survive the
	 * conversion of the command-line that the C runtime does before 'main()' - which is why
	 * CommandLineParser works with a QStringList rather than with 'argv'.
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
	 * A filename (with no directory, so that it contains no spaces - a response file is
	 * tokenized on whitespace and cannot quote a value).
	 */
	const QString
	non_ascii_filename()
	{
		return non_ascii_text() + ".gpml";
	}

	/**
	 * The options accepted by these tests: the simple options (which is where the
	 * '--response-file' and '--config-file' options come from) and a '--file' option.
	 *
	 * Note: '--file' is a config option, so that it can be given on the command-line, in a
	 * config file, or as a positional argument.
	 */
	void
	add_options(
			GPlatesUtils::CommandLineParser::InputOptions &input_options)
	{
		input_options.add_simple_options();

		input_options.config_options.add_options()
			(FILE_OPTION_NAME,
			boost::program_options::value< std::vector<std::string> >(),
			"files to load");

		input_options.positional_options.add(FILE_OPTION_NAME, -1);
	}

	/**
	 * Returns the values of the '--file' option as QStrings.
	 *
	 * Option values are UTF-8 encoded, which is what QString::fromStdString() decodes.
	 */
	QStringList
	get_filenames(
			const boost::program_options::variables_map &vm)
	{
		QStringList filenames;

		if (vm.count(FILE_OPTION_NAME))
		{
			const std::vector<std::string> values =
					vm[FILE_OPTION_NAME].as< std::vector<std::string> >();
			for (unsigned int n = 0; n < values.size(); ++n)
			{
				filenames.append(QString::fromStdString(values[n]));
			}
		}

		return filenames;
	}

	/**
	 * Writes @a contents to a file named @a filename, which the caller names non-ASCII-ly.
	 */
	void
	write_file(
			const QString &filename,
			const QString &contents)
	{
		QFile file(filename);
		ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
		ASSERT_NE(-1, file.write(contents.toUtf8()));
		file.close();
	}
}


TEST(CommandLineParserTest, non_ascii_option_values_survive_the_parse)
{
	// The arguments as GPlates gets them from CommandLineParser::get_command_line_arguments(),
	// the first of which is the program name.
	QStringList command_line_arguments;
	command_line_arguments
			<< "gplates"
			<< "--file" << non_ascii_filename()
			<< non_ascii_text() + "-positional.gpml";

	GPlatesUtils::CommandLineParser::InputOptions input_options;
	add_options(input_options);

	boost::program_options::variables_map vm;
	GPlatesUtils::CommandLineParser::parse_command_line_options(vm, command_line_arguments, input_options);

	// The values are encoded as UTF-8 for boost::program_options, which works with narrow
	// strings, and decoded again on the way out - so they should be unchanged.
	QStringList expected_filenames;
	expected_filenames << non_ascii_filename() << non_ascii_text() + "-positional.gpml";

	EXPECT_EQ(expected_filenames, get_filenames(vm));
}


TEST(CommandLineParserTest, reads_a_response_file_with_a_non_ascii_name)
{
	QTemporaryDir temp_dir;
	ASSERT_TRUE(temp_dir.isValid());
	const QString directory = temp_dir.filePath(non_ascii_text());
	ASSERT_TRUE(QDir().mkpath(directory));
	const QString response_filename = directory + "/" + non_ascii_text() + ".rsp";

	write_file(response_filename, QString("--file ") + non_ascii_filename());

	QStringList expected_filenames;
	expected_filenames << non_ascii_filename();

	// The response file is opened with QFile, so its name can contain anything a filename can.
	{
		QStringList command_line_arguments;
		command_line_arguments << "gplates" << "--response-file" << response_filename;

		GPlatesUtils::CommandLineParser::InputOptions input_options;
		add_options(input_options);

		boost::program_options::variables_map vm;
		GPlatesUtils::CommandLineParser::parse_command_line_options(vm, command_line_arguments, input_options);

		EXPECT_EQ(expected_filenames, get_filenames(vm));
	}

	// "@filename" names a response file too.
	{
		QStringList command_line_arguments;
		command_line_arguments << "gplates" << "@" + response_filename;

		GPlatesUtils::CommandLineParser::InputOptions input_options;
		add_options(input_options);

		boost::program_options::variables_map vm;
		GPlatesUtils::CommandLineParser::parse_command_line_options(vm, command_line_arguments, input_options);

		EXPECT_EQ(expected_filenames, get_filenames(vm));
	}
}


TEST(CommandLineParserTest, reads_a_config_file_with_a_non_ascii_name)
{
	QTemporaryDir temp_dir;
	ASSERT_TRUE(temp_dir.isValid());
	const QString directory = temp_dir.filePath(non_ascii_text());
	ASSERT_TRUE(QDir().mkpath(directory));
	const QString config_filename = directory + "/" + non_ascii_text() + ".cfg";

	write_file(config_filename, QString(FILE_OPTION_NAME) + "=" + non_ascii_filename() + "\n");

	QStringList command_line_arguments;
	command_line_arguments << "gplates" << "--config-file" << config_filename;

	GPlatesUtils::CommandLineParser::InputOptions input_options;
	add_options(input_options);

	boost::program_options::variables_map vm;
	GPlatesUtils::CommandLineParser::parse_command_line_options(vm, command_line_arguments, input_options);

	QStringList expected_filenames;
	expected_filenames << non_ascii_filename();

	EXPECT_EQ(expected_filenames, get_filenames(vm));
}


TEST(CommandLineParserTest, does_not_parse_the_program_name_as_an_option)
{
	// The program name is whatever the program was invoked as, which is not necessarily a name
	// that looks like a filename.
	//
	// Note: The arguments are parsed by the std::vector<std::string> overload of
	// boost::program_options::command_line_parser, which - unlike the (argc, argv) overload -
	// does not skip a leading program name, so parse_command_line_options() drops it itself.
	QStringList command_line_arguments;
	command_line_arguments << "--help" << "--file" << non_ascii_filename();

	GPlatesUtils::CommandLineParser::InputOptions input_options;
	add_options(input_options);

	boost::program_options::variables_map vm;
	GPlatesUtils::CommandLineParser::parse_command_line_options(vm, command_line_arguments, input_options);

	EXPECT_FALSE(GPlatesUtils::CommandLineParser::is_help_requested(vm));

	QStringList expected_filenames;
	expected_filenames << non_ascii_filename();

	EXPECT_EQ(expected_filenames, get_filenames(vm));
}
