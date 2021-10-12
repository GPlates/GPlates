/* $Id$ */

/**
 * \file Command-line interface version of GPlates.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>

#include "cli/CliCommandDispatcher.h"

#include "file-io/ErrorOpeningFileForReadingException.h"

#include "global/Constants.h"
#include "global/GPlatesException.h"
#include "global/NotYetImplementedException.h"

#include "utils/Profile.h"


namespace
{
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
	 * The option name used to extract the first positional command-line argument which
	 * is the GPlates command that the user wishes to execute.
	 * Each command has its own set of options.
	 * This allows the user to select different functionality using a single
	 * command-line GPlates executable.
	 */
	const char *COMMAND_OPTION_NAME = "command";

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
	 * Option name to print usage of a specific GPlates command.
	 */
	const char *HELP_COMMAND_OPTION_NAME = "help-command";


	/**
	 * Groups options into different categories.
	 */
	struct InputOptions
	{
		InputOptions() :
			generic_options("Generic options (can only appear on the command line)"),
			config_options("Configuration options (can appear on command-line or in a config file)"),
			hidden_options("Hidden options")
		{  }


		/**
		 * Adds the basic options.
		 */
		void
		add_simple_options()
		{
			generic_options.add_options()
				(HELP_COMMAND_OPTION_NAME,
						boost::program_options::value<std::string>(),
						"print options available for the specified command")
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


		// Options that will be allowed only on command line.
		// NOTE: please add new options to GPlatesCli::CommandDispatcher
		// instead of here. The basic 'help' and 'version' options are here
		// but all GPlates specific functionality and command-line argument
		// insertion/testing should go into GPlatesCli::CommandDispatcher.
		boost::program_options::options_description generic_options;

		// Options that will be allowed both on command line and in config file.
		// NOTE: please add new options to GPlatesCli::CommandDispatcher
		// instead of here.
		boost::program_options::options_description config_options;

		// Hidden options that will be allowed both on command line and
		// in config files but will not be shown to the user.
		// NOTE: please add new options to GPlatesCli::CommandDispatcher
		// instead of here.
		boost::program_options::options_description hidden_options;

		// Positional options.
		boost::program_options::positional_options_description positional_options;
	};


	boost::program_options::options_description
	get_cmdline_options(
			const InputOptions &input_options)
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
	get_config_file_options(
			const InputOptions &input_options)
	{
		// All config file options.
		boost::program_options::options_description config_file_options;
		config_file_options
				.add(input_options.config_options)
				.add(input_options.hidden_options);

		return config_file_options;
	}


	boost::program_options::options_description
	get_visible_options(
			const InputOptions &input_options)
	{
		// All options visible to the user (displayed in help/usage).
		boost::program_options::options_description visible("Allowed options");
		visible
				.add(input_options.generic_options)
				.add(input_options.config_options);

		return visible;
	}


	/**
	 * Prints program usage to @a os.
	 */
	void
	print_usage(
			std::ostream &os,
			const GPlatesCli::CommandDispatcher &command_dispatcher)
	{
		typedef std::vector<GPlatesCli::CommandDispatcher::command_name_and_description_type>
				command_name_and_desc_seq_type;

		// Add the simple options (such as help and version).
		InputOptions input_options;
		input_options.add_simple_options();

		// Get the list of commands.
		const command_name_and_desc_seq_type command_names_and_descriptions =
				command_dispatcher.get_command_names_and_descriptions();

		// Print an basic introduction about how to use the command-line interface.
		os
				<< "gplates-cli <command> <command options ...>"
				<< std::endl
				<< "            where <command> includes:"
				<< std::endl;

		// Print the list of commands.
		command_name_and_desc_seq_type::const_iterator iter = command_names_and_descriptions.begin();
		command_name_and_desc_seq_type::const_iterator end = command_names_and_descriptions.end();
		for ( ; iter != end; ++iter)
		{
			const std::string &command_name = iter->first;
			const std::string &command_desc = iter->second;
			os << " * " << command_name.c_str() << " - " << command_desc.c_str() << std::endl;
		}

		// Print the visible options.
		os
				<< std::endl
				<< get_visible_options(input_options)
				<< std::endl;
	}


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
	 * Parse specified options using @a command_line_parser and store parsed
	 * results in @a vm.
	 */
	void
	parse_options(
			boost::program_options::variables_map &vm,
			boost::program_options::command_line_parser &command_line_parser,
			const boost::program_options::options_description &cmdline_options,
			const boost::program_options::positional_options_description &positional_options)
	{
		command_line_parser.options(cmdline_options).positional(positional_options);

		// Parse options.
		const boost::program_options::parsed_options parsed = command_line_parser.run();

		// Store parsed options.
		boost::program_options::store(parsed, vm);
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
			const boost::program_options::positional_options_description &positional_options)
	{
		// Setup the parser for the command-line.
		boost::program_options::command_line_parser command_line_parser(argc, argv);
		command_line_parser.extra_parser(at_option_parser);

		// Parse options and store in 'vm'.
		parse_options(
				vm,
				command_line_parser,
				cmdline_options,
				positional_options);
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
			const boost::program_options::positional_options_description &positional_options)
	{
		const std::vector<std::string> args = read_response_file(vm);

		// Setup the parser.
		boost::program_options::command_line_parser command_line_parser(args);

		// Parse options and store in 'vm'.
		parse_options(
				vm,
				command_line_parser,
				cmdline_options,
				positional_options);
	}


	/**
	 * Parse the command-line options and also parse any response file and config files
	 * that are specified and store parsed results in @a vm.
	 */
	void
	parse_command_line_options(
			boost::program_options::variables_map &vm,
			int argc,
			char* argv[],
			const InputOptions &input_options)
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
				input_options.positional_options);

		// Parse response file if it exists.
		parse_response_file(
				vm, cmdline_options, input_options.positional_options);

		// Parse any config files the user specified on the command-line (or in response file).
		// This should be done after parsing the response file since the response file
		// could contain command-line arguments specifying config files.
		parse_config_files(vm, config_file_options);

		// Notify any callbacks we might have registered.
		boost::program_options::notify(vm);
	}


	/**
	 * Parses the command-line without expecting a command.
	 *
	 * This means only the simple options such as help and version are searched for.
	 */
	void
	parse_simple_options_only(
			GPlatesCli::CommandDispatcher &command_dispatcher,
			int argc,
			char* argv[])
	{
		// Add some simple options.
		InputOptions input_options;
		input_options.add_simple_options();

		// Parse the command-line options.
		boost::program_options::variables_map vm;
		parse_command_line_options(
				vm, argc, argv, input_options);

		// Print usage if 'help' option is specified.
		if (vm.count(HELP_OPTION_NAME))
		{
			print_usage(std::cout, command_dispatcher);
			return;
		}

		// Print GPlates version if requested.
		if (vm.count(VERSION_OPTION_NAME))
		{
			std::cout << GPlatesGlobal::VersionString << std::endl;
			return;
		}

		// Print the usage for a specific command.
		if (vm.count(HELP_COMMAND_OPTION_NAME))
		{
			// Add options for the command specified so that they become visible
			// when we print out the usage for the command.
			const std::string command = vm[HELP_COMMAND_OPTION_NAME].as<std::string>();
			if (!command_dispatcher.is_recognised_command(command))
			{
				// The command was not a recognised command.
				std::cout
						<< "Command '" << command.c_str() << "' is not a recognised command."
						<< std::endl;
				return;
			}

			// Get the command's options.
			InputOptions command_options;
			command_dispatcher.add_options_for_command(
					command,
					command_options.generic_options,
					command_options.config_options,
					command_options.hidden_options,
					command_options.positional_options);

			// Just print the options belonging to the command and nothing else.
			std::cout << get_visible_options(command_options) << std::endl;
			return;
		}

		// No simple options were encountered so just print the help usage.
		print_usage(std::cout, command_dispatcher);
	}


	/**
	 * Classifies the type of the first command-line argument.
	 */
	enum FirstArgumentType
	{
		FIRST_ARG_IS_COMMAND,
		FIRST_ARG_IS_UNRECOGNISED_COMMAND,
		FIRST_ARG_IS_OPTION,
		FIRST_ARG_IS_NONEXISTENT
	};


	/**
	 * Parses command-line assuming first argument is a recognised command and executes command.
	 */
	int
	parse_and_run_command(
			const std::string &command,
			GPlatesCli::CommandDispatcher &command_dispatcher,
			int argc,
			char* argv[])
	{
		// Add some simple options.
		InputOptions input_options;
		input_options.add_simple_options();

		// Since we have parsed a command we know that the user must specify a single
		// positional option (ie, not a regular option like "--command <cmd>" or
		// "-c <cmd>" but simply "<cmd>") to select which functionality they desire (and
		// each command has its own group of options used to configure it - these get added later).
		// This is really just letting the boost parser know that it should expect a
		// positional argument so that it parses correctly (we won't retrieve the argument's
		// value from the boost parsers though).
		// NOTE: each positional option must have an associated normal option (that's just
		// how boost positional options work) - it doesn't mean the user can use
		// "--command <cmd>" though (they must still use "<cmd>").
		input_options.positional_options.add(COMMAND_OPTION_NAME, 1);
		input_options.hidden_options.add_options()(COMMAND_OPTION_NAME, "GPlates command");

		//
		// Get the GPlates command dispatcher to add its options depending on the GPlates command.
		//
		command_dispatcher.add_options_for_command(
				command,
				input_options.generic_options,
				input_options.config_options,
				input_options.hidden_options,
				input_options.positional_options);

		// Parse the command-line options.
		boost::program_options::variables_map vm;
		parse_command_line_options(
				vm, argc, argv, input_options);

		//
		// Get the GPlates command dispatcher to look at the parsed options and run
		// whatever tasks it decides to based on them.
		//
		return command_dispatcher.run(command, vm);
	}


	/**
	 * Parses the command-line to determine the command specified by the user but
	 * doesn't parse any options specific to that command since we don't yet know the command.
	 */
	FirstArgumentType
	get_command(
			std::string &command,
			GPlatesCli::CommandDispatcher &command_dispatcher,
			int argc,
			char* argv[])
	{
		if (argc < 2)
		{
			command.clear();
			// Is there a command-line argument to test even ?
			return FIRST_ARG_IS_NONEXISTENT;
		}

		const std::string first_arg = argv[1];
		command = first_arg;

		// See if the first command-line argument is a recognised command.
		if (!command_dispatcher.is_recognised_command(first_arg))
		{
			// See if the first argument looks like an option.
			if (!first_arg.empty() && first_arg[0] == '-')
			{
				// It looks like an option since it starts with the '-' character.
				return FIRST_ARG_IS_OPTION;
			}

			// It doesn't look like an option so it's an unrecognised command.
			return FIRST_ARG_IS_UNRECOGNISED_COMMAND;
		}

		return FIRST_ARG_IS_COMMAND;
	}


	/**
	 * The main function minus any exception handling.
	 */
	int
	internal_main(
			int argc,
			char* argv[])
	{
		/*
		 * This object handles all interpretation of command-line options for different
		 * commands and executes a specified command.
		 *
		 * We create only one instance of this object because it creates all possible commands
		 * in its constructor and we might as well only do that once.
		 */
		GPlatesCli::CommandDispatcher command_dispatcher;

		// Get the user-specified command (this is the first positional argument on the
		// command-line).
		std::string command;
		const FirstArgumentType first_arg_type = get_command(
				command, command_dispatcher, argc, argv);
		switch (first_arg_type)
		{
		case FIRST_ARG_IS_NONEXISTENT:
		case FIRST_ARG_IS_OPTION:
			// First command-line argument was not a recognised command and it didn't
			// look like a command so parse the command-line to see if any
			// simple options such as help and version were specified.
			parse_simple_options_only(command_dispatcher, argc, argv);
			return 1;

		case FIRST_ARG_IS_UNRECOGNISED_COMMAND:
			// The first command-line argument was not a recognised command but it did
			// look like a command (rather than an option).
			std::cout
					<< "Command '" << command.c_str() << "' is not a recognised command."
					<< std::endl;
			return 1;

		case FIRST_ARG_IS_COMMAND:
			return parse_and_run_command(command, command_dispatcher, argc, argv);

		default:
			break;
		}

		// We only get here if the above switch statement doesn't catch all cases.
		throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
	}


	/**
	 * Handles exceptions thrown by @a internal_main.
	 */
	int
	try_catch_internal_main(
			int argc,
			char *argv[])
	{
		std::string error_message_std;
		std::string call_stack_trace_std;

		try
		{
			return internal_main(argc, argv);
		}
		catch (GPlatesGlobal::Exception &exc)
		{
			// Get exception to write its message.
			std::ostringstream ostr_stream;
			ostr_stream << exc;
			error_message_std = ostr_stream.str();

			// Extract the call stack trace to the location where the exception was thrown.
			exc.get_call_stack_trace_string(call_stack_trace_std);
		}
		catch(std::exception& exc)
		{
			error_message_std = exc.what();
		}
		catch (...)
		{
			error_message_std = "unknown exception";
		}

		//
		// If we get here then we caught an exception
		//

		QStringList error_message_stream;
		error_message_stream
				<< QObject::tr("Error: GPlates has caught an unhandled exception: ")
				<< QString::fromStdString(error_message_std);

		QString error_message = error_message_stream.join("");

		// If we have an installed message handler then this will output to a log file.
		qWarning() << error_message;

		// Output the call stack trace if we have one.
		if (!call_stack_trace_std.empty())
		{
			// If we have an installed message handler then this will output to a log file.
			// Also write out the SVN revision number so we know which source code to look
			// at when users send us back a log file.
			qWarning()
					<< QString::fromStdString(call_stack_trace_std)
					<< endl
					<< GPlatesGlobal::SourceCodeControlVersionString;
		}

		// If we have an installed message handler then this will output to a log file.
		// This is where the core dump or debugger trigger happens on debug builds.
		// On release builds this exits the application with a return value of 1.
		qFatal("Exiting due to exception caught");

		// Shouldn't get past qFatal - this just keeps compiler happy.
		return -1;
	}
}


int
main(
		int argc,
		char* argv[])
{
	// The first of two reasons to wrap 'main()' around 'internal_main()' is to
	// handle any uncaught exceptions that occur in main().
	const int return_code = try_catch_internal_main(argc, argv);

	// The second of two reasons to wrap 'main' around 'internal_main' is because
	// we want all profiles to have completed before we do profile reporting
	// and we only want to do profile reporting if no exceptions have made their
	// way back to 'main' (in other words we won't get here if 'internal_main()' threw
	// an exception).
	// NOTE: This statement is a no-op unless the current build type is "ProfileGplates"
	// in Visual Studio or you used the "-DCMAKE_BUILD_TYPE:STRING=profilegplates"
	// command-line option in "cmake" on Linux or Mac.
	PROFILE_REPORT_TO_FILE("profile.txt");

	return return_code;
}
