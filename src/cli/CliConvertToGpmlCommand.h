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

#ifndef GPLATES_CLI_CLICONVERTTOGPML_H
#define GPLATES_CLI_CLICONVERTTOGPML_H

#include <string>
#include <vector>

#include "CliCommand.h"

#include "model/ModelInterface.h"


namespace GPlatesCli
{
	class ConvertToGpmlCommand :
			public Command
	{
	public:
		ConvertToGpmlCommand();


		//! Name of this command as seen on the command-line.
		virtual
		std::string
		get_command_name() const
		{
			return "convert-to-gpml";
		}


		//! A brief description of this command.
		virtual
		std::string
		get_command_description() const
		{
			return "converts loaded feature collection(s) to GPML format";
		}


		//! Add options to be parsed by the command-line/config-file parser.
		virtual
		void
		add_options(
				boost::program_options::options_description &generic_options,
				boost::program_options::options_description &config_options,
				boost::program_options::options_description &hidden_options,
				boost::program_options::positional_options_description &positional_options);


		//! Interprets the parsed command-line and config file options stored in @a vm and runs this command.
		virtual
		int
		run(
				const boost::program_options::variables_map &vm);

	private:
		GPlatesModel::ModelInterface d_model;
		std::string d_output_basename_suffix;
	};
}

#endif // GPLATES_CLI_CLICONVERTTOGPML_H
