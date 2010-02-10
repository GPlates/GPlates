/* $Id$ */

/**
* \file
* File specific comments.
*
* Most recent change:
*   $Date$
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

#ifndef GPLATES_UTILS_COMMAND_LINE_PARSER_H
#define GPLATES_UTILS_COMMAND_LINE_PARSER_H

#include <boost/program_options.hpp>

namespace GPlatesUtils
{
	namespace CommandLineParser
	{
		/**
		 * This is where all options to be parsed on the command-line are stored.
		 *
		 * And it groups options into different categories.
		 */
		struct InputOptions
		{
			InputOptions() :
				generic_options("Generic options (can only appear on the command line)"),
				config_options("Configuration options (can appear on command-line or in a config file)"),
				hidden_options("Hidden options")
			{  }


			/**
			 * Adds the basic options such as help describing how to use
			 * response/configuration files and the version of GPlates.
			 *
			 * These are general options that can be used by any executable or
			 * application that needs to parse the command-line.
			 */
			void
			add_simple_options();
			
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
				const GPlatesUtils::CommandLineParser::InputOptions &input_options);

		boost::program_options::options_description
		get_config_file_options(
				const GPlatesUtils::CommandLineParser::InputOptions &input_options);
		
		boost::program_options::options_description
		get_visible_options(
				const GPlatesUtils::CommandLineParser::InputOptions &input_options);

		/**
		* Parse the command-line options and also parse any response file and config files
		* that are specified and store parsed results in @a vm.
		*
		* Can throw std::exception for some invalid command-lines.
		*/
		void
		parse_command_line_options(
				boost::program_options::variables_map &vm,
				int argc,
				char* argv[],
				const InputOptions &input_options);

		/**
		 * Returns true if help was requested in the parsed command-line arguments.
		 */
		bool
		is_help_requested(
				const boost::program_options::variables_map &vm);
		
		/**
		 * Returns true if the GPlates version was requested in the
		 * parsed command-line arguments.
		 */
		bool
		is_version_requested(
				const boost::program_options::variables_map &vm);
	}
}
#endif    //GPLATES_UTILS_COMMAND_LINE_PARSER_H
