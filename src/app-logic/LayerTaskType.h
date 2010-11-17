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
			AGE_GRID,
			TOPOLOGY_BOUNDARY_RESOLVER,
			TOPOLOGY_NETWORK_RESOLVER,
			VELOCITY_FIELD_CALCULATOR,
			CO_REGISTRATION,
			// Entries for new built-in layer task types should go here.

			// The following two members should be the third-last and second-last,
			// just before NUM_TYPES. Although we don't have user-defined layer
			// tasks (yet), the intention is that user-defined layer tasks are
			// identified with a value in the range [ MIN_USER, MAX_USER ].
			MIN_USER = 32768,
			MAX_USER = 65535,

			NUM_TYPES // This must be the last entry.
		};
	}
}

#endif // GPLATES_APP_LOGIC_LAYERTASKTYPE_H
