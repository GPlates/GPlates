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

#include "scribe/Transcribe.h"
#include "scribe/TranscribeEnumProtocol.h"


namespace GPlatesAppLogic
{
	/**
	 * An enumeration of layer task types.
	 *
	 * This is useful for signalling to the user interface the type of a
	 * particular layer.
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

			NUM_BUILT_IN_TYPES, // This must be after all built-in types and before MIN_USER.

			// The following two members should be the third-last and second-last,
			// just before NUM_TYPES. Although we don't have user-defined layer
			// tasks (yet), the intention is that user-defined layer tasks are
			// identified with a value in the range [ MIN_USER, MAX_USER ].
			MIN_USER = 32768,
			MAX_USER = 65535,

			NUM_TYPES // This must be the last entry.
		};


		/**
		 * Transcribe for sessions/projects.
		 */
		inline
		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				Type &layer_task_type,
				bool transcribed_construct_data)
		{
			// WARNING: Changing the string ids will break backward/forward compatibility.
			static const GPlatesScribe::EnumValue enum_values[] =
			{
				GPlatesScribe::EnumValue("RECONSTRUCTION", RECONSTRUCTION),
				GPlatesScribe::EnumValue("RECONSTRUCT", RECONSTRUCT),
				GPlatesScribe::EnumValue("RASTER", RASTER),
				GPlatesScribe::EnumValue("SCALAR_FIELD_3D", SCALAR_FIELD_3D),
				GPlatesScribe::EnumValue("TOPOLOGY_GEOMETRY_RESOLVER", TOPOLOGY_GEOMETRY_RESOLVER),
				GPlatesScribe::EnumValue("TOPOLOGY_NETWORK_RESOLVER", TOPOLOGY_NETWORK_RESOLVER),
				GPlatesScribe::EnumValue("VELOCITY_FIELD_CALCULATOR", VELOCITY_FIELD_CALCULATOR),
				GPlatesScribe::EnumValue("CO_REGISTRATION", CO_REGISTRATION)
			};

			return GPlatesScribe::transcribe_enum_protocol(
					TRANSCRIBE_SOURCE,
					scribe,
					layer_task_type,
					enum_values,
					enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
		}
	}
}

#endif // GPLATES_APP_LOGIC_LAYERTASKTYPE_H
