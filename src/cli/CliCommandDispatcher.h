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

#ifndef GPLATES_SRC_CLI_COMMAND_DISPATCHER_H
#define GPLATES_SRC_CLI_COMMAND_DISPATCHER_H

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/shared_ptr.hpp>


namespace GPlatesCli
{
	class Command;

	/**
	 * The GPlates command-line allows a single command (with its own command-line options)
	 * from a group of possible commands - this class keeps track of those commands
	 * and provides an interface for getting a specific command to add its command-line
	 * options and for executing that command once its command-line options have been parsed.
	 */
	class CommandDispatcher
	{
	public:
		CommandDispatcher();


		/**
		 * Typedef for a command name and description.
		 *
		 * The first value is the command name and the second the description.
		 */
		typedef std::pair<std::string, std::string> command_name_and_description_type;


		/**
		 * Returns a list of the names of all commands (as they appear on the command-line)
		 * and a brief description for each (note: the description does not include the
		 * options used by that command - that is taken care of by @a add_options_for_command
		 * since we cant print out boost::program_options::options_description).
		 */
		std::vector<command_name_and_description_type>
		get_command_names_and_descriptions() const;


		/**
		 * Returns true if @a command_name is a recognised command.
		 */
		bool
		is_recognised_command(
				const std::string &command_name) const;


		/**
		 * Add options to be parsed by the command-line/config-file parser.
		 *
		 * Precondition: check that @a command_name is a recognised command with
		 * @a is_recognised_command.
		 *
		 * @param generic_options Options that will be allowed only on the command line.
		 * @param config_options Options that will be allowed both on the command line and
		 *        in config files.
		 * @param hidden_options Options that will be allowed both on the command line and
		 *        in config files but will not be shown to the user.
		 * @param positional_options Options that are not like normal options in that they
		 *        don't look like "--name value" or "-n value" - instead they look like "value".
		 * @throws PreconditionViolationError if @a command_name is not a recognised command.
		 */
		void
		add_options_for_command(
				const std::string &command_name,
				boost::program_options::options_description &generic_options,
				boost::program_options::options_description &config_options,
				boost::program_options::options_description &hidden_options,
				boost::program_options::positional_options_description &positional_options);


		/**
		 * Interprets the parsed command-line and config file options stored in @a vm
		 * and runs the command specified by @a command_name.
		 *
		 * Precondition: check that @a command_name is a recognised command with
		 * @a is_recognised_command.
		 *
		 * Returns 0 on success otherwise non-zero.
		 * @throws PreconditionViolationError if @a command_name is not a recognised command.
		 */
		int
		run(
				const std::string &command_name,
				const boost::program_options::variables_map &vm);

	private:
		typedef boost::shared_ptr<Command> command_ptr_type;
		typedef std::map<std::string, command_ptr_type > command_map_type;

		command_map_type d_command_map;


		//! Looks up @a command using @a command_name.
		bool
		get_command(
				Command *&command,
				const std::string &command_name);
	};
}

#endif // GPLATES_SRC_CLI_COMMAND_DISPATCHER_H
