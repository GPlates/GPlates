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

#ifndef GPLATES_SRC_CLI_ASSIGN_PLATE_IDS_COMMAND_H
#define GPLATES_SRC_CLI_ASSIGN_PLATE_IDS_COMMAND_H

#include <string>
#include <vector>

#include "CliCommand.h"

#include "file-io/File.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/types.h"


namespace GPlatesCli
{
	class AssignPlateIdsCommand :
			public Command
	{
	public:
		AssignPlateIdsCommand();


		//! Name of this command as seen on the command-line.
		virtual
		std::string
		get_command_name() const
		{
			return "assign-plate-ids";
		}


		//! A brief description of this command.
		virtual
		std::string
		get_command_description() const
		{
			return "assign plate ids to regular features using dynamic or static polygons";
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
		void
		run(
				const boost::program_options::variables_map &vm);

	private:
		GPlatesModel::ModelInterface d_model;

		/**
		 * Whether each moving plate rotation sequence is extended back to the distant past such that
		 * reconstructed geometries are not snapped back to their present day positions.
		 */
		bool d_extend_total_reconstruction_poles_to_distant_past;

		/**
		 * The reconstruction time at which to do the cookie-cutting or plate id (re)assigning.
		 * For most cases this will be present day (0Ma).
		 */
		double d_recon_time;

		/**
		 * Assign plate ids (from the partitioning features).
		 */
		bool d_assign_plate_id;

		/**
		 * Assign time period (from the partitioning features).
		 */
		bool d_assign_time_period;

		/**
		 * Only partition features that exist at the reconstruction time.
		 */
		bool d_respect_time_period;

		GPlatesModel::integer_plate_id_type d_anchor_plate_id;

		std::string d_save_file_prefix;
		std::string d_save_file_suffix;
	};
}

#endif // GPLATES_SRC_CLI_ASSIGN_PLATE_IDS_COMMAND_H
