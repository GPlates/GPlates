/* $Id$ */

/**
 * \file 
 * This file contains the source code for the gplates-extract-svn-info program,
 * which is run each time GPlates is compiled.
 * 
 * This file is not compiled into any of the GPlates executables.
 *
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

#include <algorithm>
#include <iostream>
#include <boost/optional.hpp>
#include <QByteArray>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTextStream>


namespace
{
	const char *SVNVERSION_EXECUTABLE = "svnversion";
	const char *SVN_EXECUTABLE = "svn";
	const char *BRANCHES_DIRECTORY_NAME = "branches";

	// NOTE! If this gets changed, make sure you update the two variables below.
	const char *OUTPUT_TEMPLATE =
		/*  1 */ "#include \"global/SubversionInfo.h\"\n"
		/*  2 */ "const char *\n"
		/*  3 */ "GPlatesGlobal::SubversionInfo::get_working_copy_version_number() {\n"
		/*  4 */ "	return\n"
		/*  5 */ "	\"%1\"\n"
		/*  6 */ "	;\n"
		/*  7 */ "}\n"
		/*  8 */ "const char *\n"
		/*  9 */ "GPlatesGlobal::SubversionInfo::get_working_copy_branch_name() {\n"
		/* 10 */ "	return\n"
		/* 11 */ "	\"%2\"\n"
		/* 12 */ "	;\n"
		/* 13 */ "}\n";

	const int VERSION_NUMBER_LINE = 5;
	const int BRANCH_NAME_LINE = 11;


	/**
	 * Timeout for reading from a process's standard output, in milliseconds.
	 */
	const int READ_TIMEOUT = 2000;


	/**
	 * Runs `svnversion` on the @a working_directory to obtain a compact version
	 * number for that working directory.
	 *
	 * See `svnversion --help` for an explanation of the output of this program.
	 * In all cases, the output of the program is always one string on one line,
	 * without any whitespace.
	 *
	 * If an error was occurred (e.g. where the `svnversion` executable could not be
	 * found), this function will return the empty string.
	 */
	QString
	get_compact_version_number(
			const char *working_directory,
			const char *program_name)
	{
		// Start the `svnversion` program.
		QProcess process;
		QStringList args;
		args << working_directory;
		process.start(SVNVERSION_EXECUTABLE, args, QIODevice::ReadOnly | QIODevice::Text);
		if (!process.waitForStarted())
		{
			// Process couldn't start, e.g. `svnversion` not found.
			std::cerr << program_name << ": warning: svnversion could not start" << std::endl;
			return QString();
		}

		// Read from the process's standard output.
		QString result;

		// For some reason, we sometimes don't read anything from the process.
		// So let's try it a few more times and see if it works.
		static const int MAX_ATTEMPTS = 5;
		for (int i = 0; i != MAX_ATTEMPTS; ++i)
		{
			if (process.waitForReadyRead(READ_TIMEOUT))
			{
				QByteArray input_bytes = process.readAllStandardOutput();
				QTextStream input_stream(&input_bytes);
				result = input_stream.readLine();
				if (!result.isEmpty())
				{
					break;
				}
			}
		}

		if (result.isEmpty())
		{
			std::cerr << program_name << ": warning: could not read from svnversion" << std::endl;
		}

		// Close the process.
		process.waitForFinished();
		process.close();
		
		if (result == "exported")
		{
			// The directory we were given is not a working copy.
			result = "";
		}

		return result;
	}


	/**
	 * Returns true iff @a string consists of @a num_digits digits.
	 */
	bool
	is_number(
			const QString &string,
			int num_digits)
	{
		if (string.length() != num_digits)
		{
			return false;
		}
		for (int i = 0; i != num_digits; ++i)
		{
			if (!string.at(i).isDigit())
			{
				return false;
			}
		}
		return true;
	}


	/**
	 * Removes the date at the end of the branch name, if there is a date.
	 *
	 * Note: This function does not do proper validation of the date. If it sort of
	 * looks like a date, it will be treated as such.
	 */
	QString
	clean_up_branch_name(
			const QString &branch_name)
	{
		QStringList tokens = branch_name.split("-", QString::SkipEmptyParts);

		// There needs to be at least 4 tokens for a date to be found (actual branch
		// name, year, month, day).
		if (tokens.count() < 4)
		{
			return branch_name;
		}

		// The day needs to consist of 2 digits.
		const QString &day = tokens.at(tokens.count() - 1);
		if (!is_number(day, 2))
		{
			return branch_name;
		}
		tokens.removeLast();

		// We'll say it's a month if that token is 3 characters long.
		const QString &month = tokens.at(tokens.count() - 1);
		if (month.length() != 3)
		{
			return branch_name;
		}
		tokens.removeLast();

		// The year needs to consist of 4 digits.
		const QString &year = tokens.at(tokens.count() - 1);
		if (!is_number(year, 4))
		{
			return branch_name;
		}
		tokens.removeLast();

		// If we got to here, then the tokens left should represent the branch name.
		return tokens.join("-");
	}


	/**
	 * Runs `svn info` on the working directory to obtain the branch name from the
	 * working directory's URL.
	 *
	 * The output of `svn info` consists of a number of lines, with one piece of
	 * information per line. The URL line looks like this, for example:
	 *
	 *		URL: https://svn-test.gplates.org/gplates/branches/my-branch-2000-jan-01
	 *
	 * This function finds the URL line, and it breaks up the URL by '/'. The token
	 * following the token 'branches' is considered the branch name.
	 *
	 * Where the branch name has a date in the above format at the end (this is not
	 * required, but it is GPlates convention), the date is removed.
	 *
	 * If the working directory is trunk, or an error occurred while processing,
	 * the empty string is returned.
	 */
	QString
	get_branch_name(
			const char *working_directory,
			const char *program_name)
	{
		// Start the `svn info` program.
		QProcess process;
		QStringList args;
		args << "info" << working_directory;
		process.start(SVN_EXECUTABLE, args, QIODevice::ReadOnly | QIODevice::Text);
		if (!process.waitForStarted())
		{
			// Process couldn't start, e.g. `svn` not found.
			std::cerr << program_name << ": warning: svn could not start" << std::endl;
			return QString();
		}

		// Commence reading from the process's standard output.
		QString branch_name;
		while (true)
		{
			// Read as much data as it can give to us for the moment.
			if (!process.waitForReadyRead(READ_TIMEOUT))
			{
				break;
			}
			QByteArray input_bytes = process.readAllStandardOutput();
			QTextStream input_stream(&input_bytes);
			if (input_stream.atEnd())
			{
				break;
			}

			while (true)
			{
				// Read one line.
				QString line = input_stream.readLine();
				if (line.isEmpty())
				{
					break;
				}

				// See if it's the URL line.
				if (!line.startsWith("URL:"))
				{
					continue;
				}

				// Split the line up by '/', and search for the branches token.
				QStringList tokens = line.split("/", QString::SkipEmptyParts);
				QStringList::const_iterator iter = std::find(
						tokens.begin(),
						tokens.end(),
						BRANCHES_DIRECTORY_NAME);
				if (iter == tokens.end())
				{
					// BRANCHES_DIRECTORY_NAME not found; we are probably looking at trunk.
					break;
				}

				// Get the branch name, which is after BRANCHES_DIRECTORY_NAME, and get the
				// date removed from the end.
				++iter;
				if (iter == tokens.end())
				{
					// BRANCHES_DIRECTORY_NAME was somehow the last token. Bad user.
					break;
				}
				branch_name = clean_up_branch_name(*iter);
			}
		}

		// Close the process.
		process.waitForFinished();
		process.close();

		return branch_name;
	}


	/**
	 * Checks to see if @a output_filename exists, and if it exists, attempts to
	 * read the existing version number and branch name out of it.
	 *
	 * Returns false if the existing version number matches matches with
	 * @a version_number and if the existing branch number matches with @a branch_name.
	 */
	bool
	is_update_needed(
			const char *output_filename,
			const QString &version_number,
			const QString &branch_name)
	{
		// Open file for input.
		QFile file(output_filename);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			// There could be other reasons why we're here, but let's assume that we
			// couldn't read it because the file doesn't exist.
			return true;
		}

		// Read the file.
		QTextStream input_stream(&file);
		int current_line = 1;
		boost::optional<QString> existing_version_number;
		boost::optional<QString> existing_branch_name;
		while (!input_stream.atEnd())
		{
			QString line = input_stream.readLine().trimmed();

			if (line.startsWith("\"") && line.endsWith("\""))
			{
				line = line.mid(1, line.length() - 2);
				if (current_line == VERSION_NUMBER_LINE)
				{
					existing_version_number = line;
				}
				else if (current_line == BRANCH_NAME_LINE)
				{
					existing_branch_name = line;
				}
			}

			++current_line;
		}

		file.close();

		// The output file needs updating if we failed to read the existing values.
		if (!existing_version_number || !existing_branch_name)
		{
			return true;
		}

		// The output file also needs updating if the values have changed.
		return version_number != *existing_version_number ||
			branch_name != *existing_branch_name;
	}
}


/**
 * This program is compiled and run every time GPlates is compiled. It obtains
 * information about the working copy from which GPlates is being compiled and
 * creates, at GPlates compile time, a .cc file containing this information that
 * is then compiled into GPlates.
 *
 * Usage:
 *
 *		gplates-extract-svn-info WORKING_DIRECTORY OUTPUT_CC_FILE [CUSTOM_VERSION_NUMBER]
 *
 * WORKING_DIRECTORY is the working directory for which information is to be
 * obtained. OUTPUT_CC_FILE should be src/global/SubversionInfo.cc (with the
 * correct path prepended).
 *
 * First, this program will run `svnversion` on the working directory to obtain
 * a compact version number for that working directory.
 *
 * Secondly, this program will run `svn info` on the working directory to obtain
 * the URL, from which the branch name (or trunk) is extracted.
 *
 * `svnversion` and `svn` are invoked without a full path. Thus, both of these
 * executables must reside in a directory in the system path.
 *
 * If the optional argument CUSTOM_VERSION_NUMBER is provided, `svnversion` is
 * not invoked; instead, CUSTOM_VERSION_NUMBER will be used as the version
 * number and the branch name will be set to the empty string "".
 *
 * The arguments are provided to this program by the CMake scripts. By default,
 * the last argument is not provided. However, a custom source control version
 * number may be useful for consistency across public releases, for example.
 *
 * To set the value of CUSTOM_VERSION_NUMBER, set the CMake variable
 * GPlates_SOURCE_CODE_CONTROL_VERSION, e.g.:
 *
 *     cmake -DGPlates_SOURCE_CODE_CONTROL_VERSION:STRING=1234 ../0.9.9
 *
 * On the Windows GUI version of CMake, the variable can be added by clicking
 * the "Add Entry" button.
 *
 * Why is this not a shell script? I'd then have to write a Batch file for use
 * on Windows, and Batch is not a language one speaks in polite company.
 */
int main(int argc, char *argv[])
{
	// If an incorrect number of arguments is provided, print a usage help message.
	const char *program_name = argv[0];
	int num_args = argc - 1;
	if (num_args < 2 || num_args > 3)
	{
		std::cerr << program_name << ": error: expected 2 or 3 arguments, found " << num_args << std::endl;
		std::cerr << "Usage: " << program_name << " WORKING_DIRECTORY OUTPUT_CC_FILE [CUSTOM_VERSION_NUMBER]" << std::endl;
		return 1;
	}

	const char *working_directory = argv[1];
	const char *output_filename = argv[2];
	const char *custom_version_number = NULL;
	if (num_args >= 3)
	{
		custom_version_number = argv[3];
	}

	// Compute the version number and branch name if required.
	QString version_number = custom_version_number ?
			QString(custom_version_number) :
			get_compact_version_number(working_directory, program_name);
	QString branch_name = custom_version_number ?
			QString() :
			get_branch_name(working_directory, program_name);

	// Check whether we need to write the values out again or not. We don't write
	// the values out if they haven't changed because we don't want to cause
	// SubversionInfo.cc to be recompiled unnecessarily.
	if (is_update_needed(output_filename, version_number, branch_name))
	{
		// Open file for output.
		QFile file(output_filename);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			std::cerr << program_name << ": error: could not open " << output_filename << " for writing" << std::endl;
			return 2;
		}

		// Write the output file.
		QTextStream output_stream(&file);
		output_stream << QString(OUTPUT_TEMPLATE).arg(version_number).arg(branch_name);
		
		file.close();
	}
	
	return 0;
}

