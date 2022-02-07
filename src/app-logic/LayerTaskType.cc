/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include "LayerTaskType.h"

#include "scribe/TranscribeEnumProtocol.h"


GPlatesScribe::TranscribeResult
GPlatesAppLogic::LayerTaskType::transcribe(
		GPlatesScribe::Scribe &scribe,
		Type &layer_task_type,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("RECONSTRUCTION", RECONSTRUCTION),
		GPlatesScribe::EnumValue("RECONSTRUCT", RECONSTRUCT),
		GPlatesScribe::EnumValue("RASTER", RASTER),
		GPlatesScribe::EnumValue("SCALAR_FIELD_3D", SCALAR_FIELD_3D),
		GPlatesScribe::EnumValue("TOPOLOGY_GEOMETRY_RESOLVER", TOPOLOGY_GEOMETRY_RESOLVER),
		GPlatesScribe::EnumValue("TOPOLOGY_NETWORK_RESOLVER", TOPOLOGY_NETWORK_RESOLVER),
		GPlatesScribe::EnumValue("VELOCITY_FIELD_CALCULATOR", VELOCITY_FIELD_CALCULATOR),
		GPlatesScribe::EnumValue("CO_REGISTRATION", CO_REGISTRATION),
		GPlatesScribe::EnumValue("RECONSTRUCT_SCALAR_COVERAGE", RECONSTRUCT_SCALAR_COVERAGE)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			layer_task_type,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
