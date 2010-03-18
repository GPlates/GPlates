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
#include <QString>

#include "CliCommandDispatcher.h"

#include "CliAssignPlateIdsCommand.h"
#include "CliInvalidOptionValue.h"
#include "CliReconstructCommand.h"
#include "CliRequiredOptionNotPresent.h"
#include "CliConvertFileFormatCommand.h"

#include "global/PreconditionViolationError.h"


GPlatesCli::CommandDispatcher::CommandDispatcher()
{
	//
	// Each new command type must be instantiated here and added to the map.
	//

	// Add ReconstructCommand.
	command_ptr_type reconstruct_cmd(new ReconstructCommand());
	d_command_map[reconstruct_cmd->get_command_name()] = reconstruct_cmd;

	// Add ConvertFileFormatCommand.
	command_ptr_type convert_to_gpml_cmd(new ConvertFileFormatCommand());
	d_command_map[convert_to_gpml_cmd->get_command_name()] = convert_to_gpml_cmd;

	// Add AssignPlateIdsCommand.
	command_ptr_type assign_plate_ids_command(new AssignPlateIdsCommand());
	d_command_map[assign_plate_ids_command->get_command_name()] = assign_plate_ids_command;
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


int
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

	try
	{
		// Get the command to run.
		return cmd->run(vm);
	}
	catch(RequiredOptionNotPresent &exc)
	{
		std::cerr << exc << std::endl;
		return 1; // Zero is success
	}
	catch(InvalidOptionValue &exc)
	{
		std::cerr << exc << std::endl;
		return 1; // Zero is success
	}

	return 0; // Zero is success
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
