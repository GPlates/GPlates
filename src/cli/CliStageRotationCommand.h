/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_CLI_CLISTAGEROTATIONCOMMAND_H
#define GPLATES_CLI_CLISTAGEROTATIONCOMMAND_H

#include <string>
#include <vector>

#include "CliCommand.h"

#include "file-io/File.h"

#include "maths/FiniteRotation.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/types.h"


namespace GPlatesCli
{
	class StageRotationCommand :
			public Command
	{
	public:
		StageRotationCommand();


		//! Name of this command as seen on the command-line.
		virtual
		std::string
		get_command_name() const
		{
			return "stage-rotation";
		}


		//! A brief description of this command.
		virtual
		std::string
		get_command_description() const
		{
			return "print the stage rotation (full or half stage) between two plates and two times";
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
		typedef std::vector<GPlatesFileIO::File::non_null_ptr_type>
				loaded_feature_collection_file_seq_type;

		GPlatesModel::ModelInterface d_model;

		/**
		 * Whether each moving plate rotation sequence is extended back to the distant past such that
		 * reconstructed geometries are not snapped back to their present day positions.
		 */
		bool d_extend_total_reconstruction_poles_to_distant_past;

		double d_start_time;
		double d_end_time;
		GPlatesModel::integer_plate_id_type d_anchor_plate_id;
		GPlatesModel::integer_plate_id_type d_fixed_plate_id;
		GPlatesModel::integer_plate_id_type d_moving_plate_id;

		/**
		 * The asymmetry is in the range [-1,1] where the value 0 represents half-stage rotation
		 * and the value 1 represents full-stage rotation.
		 */
		double d_asymmetry;



		void
		output_stage_rotation(
				const GPlatesMaths::FiniteRotation &stage_rotation,
				bool output_indeterminate_for_identity_rotations);
	};
}

#endif // GPLATES_CLI_CLISTAGEROTATIONCOMMAND_H
