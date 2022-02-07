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
#include <fstream>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>

#include "CommandLineParser.h"

#include "Profile.h"

#include "file-io/ErrorOpeningFileForReadingException.h"

#include "global/Constants.h"
#include "global/GPlatesException.h"
#include "global/NotYetImplementedException.h"

namespace
{
	/**
	 * The option name used to print the program usage on the command-line.
	 */
	const char *HELP_OPTION_NAME = "help";
	//! Same as @a HELP_OPTION_NAME but with additional short option char.
	const char *HELP_OPTION_NAME_WITH_SHORT_OPTION = "help,h";

	/**
	 * The option name used to print the program version on the command-line.
	 */
	const char *VERSION_OPTION_NAME = "version";
	//! Same as @a VERSION_OPTION_NAME but with additional short option char.
	const char *VERSION_OPTION_NAME_WITH_SHORT_OPTION = "version,v";
		
	/**
	 * A response file to contain command-line options for those systems that
	 * have a small limit on the size of the command-line arguments.
	 */
	const char *RESPONSE_FILE_OPTION_NAME = "response-file";

	/**
	 * Configuration file containing options that the user wants to store in a file instead
	 * of having to type them on the command-line every time they run GPlates.
	 */
	const char *CONFIG_FILE_OPTION_NAME = "config-file";

		


	/**
	* Function for parsing options that the regular parser doesn't recognise.
	*
	* In particular we parse response filenames that look like "@filename".
	*/
	std::pair<std::string, std::string>
	at_option_parser(
			const std::string &option_name)
	{
		if ('@' == option_name[0])
		{
			return std::make_pair(RESPONSE_FILE_OPTION_NAME, option_name.substr(1));
		}

		return std::pair<std::string, std::string>();
	}


	/**
	* Parse the command-line arguments defined by @a argc and @a argv.
	*/
	void
	parse_command_line(
			boost::program_options::variables_map &vm,
			int argc,
			char* argv[],
			const boost::program_options::options_description &cmdline_options,
			const boost::program_options::positional_options_description &positional_options,
			int command_line_style)
	{
		// Setup the parser for the command-line.
		boost::program_options::command_line_parser command_line_parser(argc, argv);
		command_line_parser.extra_parser(at_option_parser);

		// Set the command-line style of processing.
		command_line_parser.style(command_line_style);

		// Setup the options to parse.
		command_line_parser.options(cmdline_options).positional(positional_options);

		// Mac OS X sometimes (when invoking from Finder or 'open' command) adds the
		// '-psn...' command-line argument to the applications argument list
		// (for example '-psn_0_548998').
		// To avoid an unknown argument exception we allow unrecognised options and explicitly
		// throw an exception ourselves if any unrecognised option does not match '-psn'.
		//
		// If the caller specifies the '-p' option (ie, it's no longer an unrecognised option)
		// then the caller will need to filter out the 'sn_*' values themselves - I tried
		// to filter it out here but there doesn't appear to be an official way to do that -
		// unofficially could do something similar to what 'boost::program_options::collect_unrecognized()'
		// does but that's iterating over options whereas removing an option is non-obvious.
		//
		// Note that we end up ignoring the '-psn...' option.
		// Also note that it doesn't actually appear in 'argv[]' for some reason.
#if defined(__APPLE__)
		command_line_parser.allow_unregistered();

		// Parse options.
		const boost::program_options::parsed_options parsed = command_line_parser.run();

		const std::vector<std::string> unrecognised_options =
				boost::program_options::collect_unrecognized(
						parsed.options,
						boost::program_options::exclude_positional);
		if (!unrecognised_options.empty())
		{
			if (unrecognised_options.size() > 1 ||
				unrecognised_options[0].substr(0, 4) != "-psn")
			{
				throw boost::program_options::unknown_option(unrecognised_options[0]);
			}
		}
#else
		// Parse options.
		// If an unrecognised option is encountered then boost will thrown an exception.
		const boost::program_options::parsed_options parsed = command_line_parser.run();
#endif

		// Store parsed options in the variables map.
		boost::program_options::store(parsed, vm);
	}

	/**
	* Parses a file containing configuration options.
	*
	* @param config_filename the name of config file to parse
	* @param config_file_options the options to looks for in the config file
	* @param vm the place to store configuration values found
	*/
	void
	parse_config_file(
			const std::string &config_filename,
			const boost::program_options::options_description &config_file_options,
			boost::program_options::variables_map &vm)
	{
		// Load the file and tokenize it.
		std::ifstream config_file(config_filename.c_str());
		if (!config_file)
		{
			throw GPlatesFileIO::ErrorOpeningFileForReadingException(
				GPLATES_EXCEPTION_SOURCE,
				config_filename.c_str());
		}

		// Parse the file and store the options.
		boost::program_options::store(
			boost::program_options::parse_config_file(config_file, config_file_options),
			vm);
	}


	/**
	* Parses any files containing configuration options.
	* The user specifies these files with the command-line options.
	*
	* @param config_file_options the options to looks for in the config files
	* @param vm the place to store configuration values found
	*/
	void
	parse_config_files(
			boost::program_options::variables_map &vm,
			const boost::program_options::options_description &config_file_options)
	{
		// Parse any configuration files specified by the user.
		if (!vm.count(CONFIG_FILE_OPTION_NAME))
		{
			return;
		}

		// Get the config filenames.
		const std::vector<std::string> &config_filenames =
			vm[CONFIG_FILE_OPTION_NAME].as<std::vector<std::string> >();

		try
		{
			// Parse each config file.
			std::for_each(
				config_filenames.begin(),
				config_filenames.end(),
				boost::bind(&parse_config_file,
						_1/*config filename*/,
						boost::cref(config_file_options),
						boost::ref(vm)));
		}
		catch (GPlatesFileIO::ErrorOpeningFileForReadingException &exc)
		{
			// The main exception handler will print an error message but
			// it's not so easy to read so print it here also to make it obvious
			// to the program user.
			std::cerr
				<< "Error opening config file '"
				<< exc.filename().toStdString().c_str()
				<< "' for reading."
				<< std::endl;

			// Re-throw exception to force program termination via main exception handler.
			throw;
		}
	}


	/**
	* Reads response file named by @a RESPONSE_FILE_OPTION_NAME option and tokenizes
	* it into a vector of strings which is returned by this function.
	*/
	std::vector<std::string>
	read_response_file(
			const boost::program_options::variables_map &vm)
	{
		// Parse any configuration files specified by the user.
		if (!vm.count(RESPONSE_FILE_OPTION_NAME))
		{
			return std::vector<std::string>();
		}

		// Get the response filename.
		const std::string &response_filename =
			vm[RESPONSE_FILE_OPTION_NAME].as<std::string>();

		// Load the file and tokenize it.
		std::ifstream response_file(response_filename.c_str());
		if (!response_file)
		{
			throw GPlatesFileIO::ErrorOpeningFileForReadingException(
				GPLATES_EXCEPTION_SOURCE,
				response_filename.c_str());
		}

		// Read the whole file into a string.
		std::ostringstream ostr_stream;
		ostr_stream << response_file.rdbuf();
		const std::string response_file_content = ostr_stream.str();

		// Split the file content
		boost::char_separator<char> sep(" \n\r");
		boost::tokenizer<boost::char_separator<char> > tok(response_file_content, sep);
		std::vector<std::string> args;
		std::copy(tok.begin(), tok.end(), std::back_inserter(args));

		return args;
	}


	/**
	* Parses a response file containing command-line options.
	*
	* @param config_file_options the options to looks for in the response file
	* @param vm the place to store configuration values found
	*/
	void
	parse_response_file(
			boost::program_options::variables_map &vm,
			const boost::program_options::options_description &cmdline_options,
			const boost::program_options::positional_options_description &positional_options,
			int command_line_style)
	{
		const std::vector<std::string> args = read_response_file(vm);

		// Setup the parser.
		boost::program_options::command_line_parser command_line_parser(args);

		// Set the command-line style of processing.
		command_line_parser.style(command_line_style);

		// Setup the options to parse.
		command_line_parser.options(cmdline_options).positional(positional_options);

		// Parse options.
		const boost::program_options::parsed_options parsed = command_line_parser.run();

		// Store parsed options in the variables map.
		boost::program_options::store(parsed, vm);
	}
}

void
GPlatesUtils::CommandLineParser::parse_command_line_options(
		boost::program_options::variables_map &vm,
		int argc,
		char* argv[],
		const GPlatesUtils::CommandLineParser::InputOptions &input_options,
		int command_line_style)
{
	// All command-line options.
	boost::program_options::options_description cmdline_options =
		get_cmdline_options(input_options);

	// All config file options.
	boost::program_options::options_description config_file_options =
		get_config_file_options(input_options);

	// All options visible to the user (displayed in help/usage).
	boost::program_options::options_description visible =
		get_visible_options(input_options);


	//
	// We parse the command-line before the config file.
	// This has implications if parameters exist in both.
	//   For some parameters this means the command-line version overrides
	//   the config file version (eg, an integer or string).
	//   For other parameters that call 'composing()' this means the values
	//   from both sources are merged together.
	//

	// Parse the command-line.
	parse_command_line(
		vm,
		argc,
		argv,
		cmdline_options,
		input_options.positional_options,
		command_line_style);

	// Parse response file if it exists.
	parse_response_file(
		vm,
		cmdline_options,
		input_options.positional_options,
		command_line_style);

	// Parse any config files the user specified on the command-line (or in response file).
	// This should be done after parsing the response file since the response file
	// could contain command-line arguments specifying config files.
	parse_config_files(vm, config_file_options);

	// Notify any callbacks we might have registered.
	boost::program_options::notify(vm);
}

void 
GPlatesUtils::CommandLineParser::InputOptions::add_simple_options()
{
	generic_options.add_options()
		(HELP_OPTION_NAME_WITH_SHORT_OPTION, "produce help message")
		(VERSION_OPTION_NAME_WITH_SHORT_OPTION, "print version string")
		(RESPONSE_FILE_OPTION_NAME,
		boost::program_options::value<std::string>(),
		"read command-line options from a response file "
		"(note: can use @filename instead)")
		(CONFIG_FILE_OPTION_NAME,
		boost::program_options::value<std::vector<std::string> >(),
		"read configuration options from file (multiple files are allowed - "
		"use multiple options, one for each config file)");
}

boost::program_options::options_description
GPlatesUtils::CommandLineParser::get_cmdline_options(
		const GPlatesUtils::CommandLineParser::InputOptions &input_options)
{
	// All command-line options.
	boost::program_options::options_description cmdline_options;
	cmdline_options
		.add(input_options.generic_options)
		.add(input_options.config_options)
		.add(input_options.hidden_options);

	return cmdline_options;
}


boost::program_options::options_description
GPlatesUtils::CommandLineParser::get_config_file_options(
		const GPlatesUtils::CommandLineParser::InputOptions &input_options)
{
	// All config file options.
	boost::program_options::options_description config_file_options;
	config_file_options
		.add(input_options.config_options)
		.add(input_options.hidden_options);

	return config_file_options;
}


boost::program_options::options_description
GPlatesUtils::CommandLineParser::get_visible_options(
		const GPlatesUtils::CommandLineParser::InputOptions &input_options)
{
	// All options visible to the user (displayed in help/usage).
	boost::program_options::options_description visible("Allowed options");
	visible
		.add(input_options.generic_options)
		.add(input_options.config_options);

	return visible;
}


bool
GPlatesUtils::CommandLineParser::is_help_requested(
	const boost::program_options::variables_map &vm)
{
	return vm.count(HELP_OPTION_NAME) != 0;
}


bool
GPlatesUtils::CommandLineParser::is_version_requested(
	const boost::program_options::variables_map &vm)
{
	return vm.count(VERSION_OPTION_NAME) != 0;
}
