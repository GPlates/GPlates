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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTHANDLE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTHANDLE_H

#include "utils/Counter64.h"


namespace GPlatesAppLogic
{
	namespace ReconstructHandle
	{
		/**
		 * Typedef for a global handle that is stored in @a ReconstructedFeatureGeometry
		 * instances to identity them, for example, as belonging to a particular group of
		 * reconstructed feature geometries.
		 *
		 * The handle is 64-bit to ensure it doesn't wraparound (or at least takes a *very* long
		 * time to wraparound - see the comment for @a Counter64).
		 */
		typedef GPlatesUtils::Counter64 type;

		//
		// NOTE: We don't want a 'get_current_reconstruct_handle()' function because
		// then anyone can accidentally place another client's reconstruct handle into their own
		// created RFGs (thinking it's going into their own group) thus effectively adding RFGs to
		// someone else's reconstruct group.
		//

		/**
		 * Returns the next global reconstruct handle by incrementing the integer handle
		 * returned by the last call to @a get_next_reconstruct_handle.
		 *
		 * The returned global handle can be stored in @a ReconstructedFeatureGeometry
		 * instances to identity them, for example, as belonging to a particular group of
		 * reconstructed feature geometries.
		 * This is useful when searching for @a ReconstructedFeatureGeometry objects when
		 * iterating over weak observers of a particular feature.
		 * If the feature has been reconstructed several times, in different situations, then it will
		 * have several @a ReconstructedFeatureGeometry observers and the handle can then be used to
		 * identify the @a ReconstructedFeatureGeometry from the desired reconstruction situation.
		 */
		inline
		type
		get_next_reconstruct_handle()
		{
			// TODO: Will need to be thread-protected or converted to singleton if GPlates becomes multi-threaded.
			static type global_reconstruct_handle(0);

			++global_reconstruct_handle;

			return global_reconstruct_handle;
		}
	}
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTHANDLE_H
