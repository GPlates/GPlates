/* $Id$ */

/**
 * \file 
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

#ifndef GPLATES_SRC_CLI_COMMAND_H
#define GPLATES_SRC_CLI_COMMAND_H

#include <string>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>


namespace GPlatesCli
{
	/**
	 * An interface for retrieving a command's name (on the command-line),
	 * adding a command's options to the command-line and executing the command
	 * once its command-line options have been parsed.
	 */
	class Command
	{
	public:
		virtual
		~Command()
		{  }


		/**
		 * Name of this command as seen on the command-line.
		 */
		virtual
		std::string
		get_command_name() const = 0;


		/**
		 * A brief description of this command.
		 *
		 * Note: the description does not include the options used by that command -
		 * that is taken care of by @a add_options_for_command since we cant print out
		 * boost::program_options::options_description.
		 */
		virtual
		std::string
		get_command_description() const = 0;


		/**
		 * Add options to be parsed by the command-line/config-file parser.
		 *
		 * @param generic_options Options that will be allowed only on the command line.
		 * @param config_options Options that will be allowed both on the command line and
		 *        in config files.
		 * @param hidden_options Options that will be allowed both on the command line and
		 *        in config files but will not be shown to the user.
		 * @param positional_options Options that are not like normal options in that they
		 *        don't look like "--name value" or "-n value" - instead they look like "value".
		 */
		virtual
		void
		add_options(
				boost::program_options::options_description &generic_options,
				boost::program_options::options_description &config_options,
				boost::program_options::options_description &hidden_options,
				boost::program_options::positional_options_description &positional_options) = 0;


		/**
		 * Interprets the parsed command-line and config file options stored in @a vm
		 * and runs this command.
		 *
		 * Throws exception on failure.
		 */
		virtual
		void
		run(
				const boost::program_options::variables_map &vm) = 0;
	};
}

#endif // GPLATES_SRC_CLI_COMMAND_H
