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
			return "assigns plate ids to regular features using topological closed plate boundaries";
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
		typedef std::vector<GPlatesFileIO::File::shared_ref> loaded_feature_collection_file_seq_type;

		GPlatesModel::ModelInterface d_model;

		/**
		 * The topological closed plate boundary features and the boundary features they reference.
		 */
		loaded_feature_collection_file_seq_type d_loaded_topological_closed_plate_boundary_files;

		/**
		 * The features that will have their plate ids (re)assigned.
		 */
		loaded_feature_collection_file_seq_type d_loaded_assign_plate_id_files;

		/**
		 * The rotation files used to rotate both the topological boundary features and
		 * the features having their plate ids (re)assigned.
		 */
		loaded_feature_collection_file_seq_type d_loaded_reconstruction_files;

		/**
		 * The reconstruction time at which to do the cookie-cutting or plate id (re)assigning.
		 * For most cases this will be present day (0Ma).
		 */
		double d_recon_time;

		GPlatesModel::integer_plate_id_type d_anchor_plate_id;


		void
		load_feature_collections(
				const boost::program_options::variables_map &vm);

		void
		load_feature_collections(
				const std::vector<std::string> &filenames,
				loaded_feature_collection_file_seq_type &files);

		void
		get_feature_collections(
				std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections,
				loaded_feature_collection_file_seq_type &files);
	};
}

#endif // GPLATES_SRC_CLI_ASSIGN_PLATE_IDS_COMMAND_H
