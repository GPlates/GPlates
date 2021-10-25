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
 
#ifndef GPLATES_APP_LOGIC_LAYERTASKTYPE_H
#define GPLATES_APP_LOGIC_LAYERTASKTYPE_H

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesAppLogic
{
	/**
	 * An enumeration of layer task types.
	 *
	 * This is useful for signaling to the user interface the type of a particular layer.
	 */
	namespace LayerTaskType
	{
		enum Type
		{
			RECONSTRUCTION,
			RECONSTRUCT,
			RASTER,
			SCALAR_FIELD_3D,
			TOPOLOGY_GEOMETRY_RESOLVER,
			TOPOLOGY_NETWORK_RESOLVER,
			VELOCITY_FIELD_CALCULATOR,
			CO_REGISTRATION,
			RECONSTRUCT_SCALAR_COVERAGE,

			// NOTE: Any new values should also be added to @a transcribe.

			NUM_TYPES // This must be the last entry.
		};


		/**
		 * Transcribe for sessions/projects.
		 */
		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				Type &layer_task_type,
				bool transcribed_construct_data);
	}
}

#endif // GPLATES_APP_LOGIC_LAYERTASKTYPE_H
