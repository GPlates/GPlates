/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_CLI_CLICOMMANDREGISTRY_H
#define GPLATES_CLI_CLICOMMANDREGISTRY_H

#include <boost/mpl/vector.hpp>

#include "CliAssignPlateIdsCommand.h"
#include "CliConvertFileFormatCommand.h"
#include "CliEquivalentTotalRotation.h"
#include "CliReconstructCommand.h"
#include "CliRelativeTotalRotation.h"
#include "CliStageRotationCommand.h"


namespace GPlatesCli
{
	namespace CommandTypes
	{
		/**
		 * Add any new command classes you have created here.
		 */
		typedef boost::mpl::vector<
				AssignPlateIdsCommand,
				ConvertFileFormatCommand,
				EquivalentTotalRotationCommand,
				ReconstructCommand,
				RelativeTotalRotationCommand,
				StageRotationCommand
		> command_types;
	}
}

#endif // GPLATES_CLI_CLICOMMANDREGISTRY_H
