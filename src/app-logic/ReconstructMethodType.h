/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTMETHODTYPE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTMETHODTYPE_H

namespace GPlatesAppLogic
{
	namespace ReconstructMethod
	{
		/**
		 * An enumeration of different ways to generate ReconstructedFeatureGeometry objects from features.
		 *
		 * NOTE: The order of enumerations is from less specialised to more specialised.
		 * For example, FLOWLINE can be handled by HALF_STAGE_ROTATION but not the other way around.
		 * So FLOWLINE should be placed after HALF_STAGE_ROTATION so that it will get matched first.
		 * BY_PLATE_ID is almost a catch-all when the others are not suitable (and hence is listed first).
		 *
		 * NOTE: The methods only apply to a subset of features - only those that generate
		 * ReconstructedFeatureGeometry objects (or derived objects).
		 * Topological features, for example, are not included here.
		 */
		enum Type
		{
			BY_PLATE_ID,
			HALF_STAGE_ROTATION,
			SMALL_CIRCLE,
			VIRTUAL_GEOMAGNETIC_POLE,
			FLOWLINE,
			MOTION_PATH,

			NUM_TYPES
		};
	}
}
	
#endif  // GPLATES_APP_LOGIC_RECONSTRUCTMETHODTYPE_H
