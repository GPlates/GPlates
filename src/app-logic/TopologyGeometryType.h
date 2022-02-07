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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYTYPE_H
#define GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYTYPE_H


namespace GPlatesAppLogic
{
	namespace TopologyGeometry
	{
		/**
		 * The type of topological geometry represented in a feature.
		 *
		 * This distinguishes how the topological geometries are resolved.
		 *
		 * NOTE: This does *not* distinguish the different *feature types* that may have a
		 * topological geometry contained within them.
		 */
		enum Type
		{
			// Resolves to form a polyline.
			LINE,

			// Resolves to form a polygon (with optional interior holes/boundaries).
			BOUNDARY,

			// Resolves to form a triangulation within a polygon (and its optional interior constraints).
			NETWORK,

			UNKNOWN
		};
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYTYPE_H
