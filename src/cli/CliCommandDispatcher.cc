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

#include <iostream>
#include <string>
#include <vector>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/lambda.hpp>
#include <QString>

#include "CliCommandDispatcher.h"

#include "CliCommandRegistry.h"
#include "CliInvalidOptionValue.h"
#include "CliRequiredOptionNotPresent.h"

#include "global/PreconditionViolationError.h"


GPlatesCli::CommandDispatcher::AddCommand::AddCommand(
		command_map_type &command_map) :
	d_command_map(command_map)
{
}


template <class CommandType>
void
GPlatesCli::CommandDispatcher::AddCommand::operator()(
		Wrap<CommandType>)
{
	command_ptr_type command(new CommandType());
	d_command_map[command->get_command_name()] = command;
}


GPlatesCli::CommandDispatcher::CommandDispatcher()
{
	//
	// Each new command type must be instantiated here and added to the map.
	//

	// A utility functor used to add the registered command types to the
	// command dispatcher.
	AddCommand add_command(d_command_map);

	// Iterate over the command types (actual class types stored in a boost::mpl::vector)
	// and call the functor to add them to the command dispatcher.
	boost::mpl::for_each< CommandTypes::command_types, AddCommand::Wrap<boost::mpl::_1> >(
			boost::ref(add_command));
}


std::vector<GPlatesCli::CommandDispatcher::command_name_and_description_type>
GPlatesCli::CommandDispatcher::get_command_names_and_descriptions() const
{
	std::vector<command_name_and_description_type> command_names_and_descriptions;

	// Iterate over the map and add the command names.
	command_map_type::const_iterator cmd_iter = d_command_map.begin();
	command_map_type::const_iterator cmd_end = d_command_map.end();
	for ( ; cmd_iter != cmd_end; ++cmd_iter)
	{
		// Get command name.
		const std::string &command_name = cmd_iter->first;

		// Get command description.
		Command *cmd = cmd_iter->second.get();
		const std::string command_desc = cmd->get_command_description();

		command_names_and_descriptions.push_back(
				std::make_pair(command_name, command_desc));
	}

	return command_names_and_descriptions;
}


bool
GPlatesCli::CommandDispatcher::is_recognised_command(
		const std::string &command_name) const
{
	// Search for the command in our map.
	return d_command_map.find(command_name) != d_command_map.end();
}


void
GPlatesCli::CommandDispatcher::add_options_for_command(
		const std::string &command_name,
		boost::program_options::options_description &generic_options,
		boost::program_options::options_description &config_options,
		boost::program_options::options_description &hidden_options,
		boost::program_options::positional_options_description &positional_options)
{
	// Lookup the command.
	Command *cmd = NULL;
	if (!get_command(cmd, command_name))
	{
		// The caller should have checked that 'command_name' is a recognised command.
		throw GPlatesGlobal::PreconditionViolationError(GPLATES_EXCEPTION_SOURCE);
	}

	// Get the command to add its options.
	cmd->add_options(generic_options, config_options, hidden_options, positional_options);
}


void
GPlatesCli::CommandDispatcher::run(
		const std::string &command_name,
		const boost::program_options::variables_map &vm)
{
	// Lookup the command.
	Command *cmd = NULL;
	if (!get_command(cmd, command_name))
	{
		// The caller should have checked that 'command_name' is a recognised command.
		throw GPlatesGlobal::PreconditionViolationError(GPLATES_EXCEPTION_SOURCE);
	}

	// Get the command to run.
	cmd->run(vm);
}


bool
GPlatesCli::CommandDispatcher::get_command(
		Command *&command,
		const std::string &command_name)
{
	// Search for the command in our map.
	command_map_type::iterator cmd_iter = d_command_map.find(command_name);
	if (cmd_iter == d_command_map.end())
	{
		return false;
	}

	// Get the command.
	command = cmd_iter->second.get();

	return true;
}
