/**
 * @file 
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_FINITEROTATIONSNAPSHOTTABLE_H
#define GPLATES_MATHS_FINITEROTATIONSNAPSHOTTABLE_H

#include "types.h"
#include "FiniteRotation.h"

namespace GPlatesMaths {

	/**
	 * "Snapshot" of the rotation hierarchy at a particular time.
	 */
	class FiniteRotationSnapshotTable {

		public:

			/**
			 * @todo XXX Implement me.
			 */
			FiniteRotationSnapshotTable() {  }

			/**
			 * @return NULL when there is no rotation defined for 
			 *  @a rot_id at the time of the snapshot.
			 *
			 * @todo XXX Implement me.
			 */
			const FiniteRotation *
			operator[](rot_id_t) {

				// Find the FiniteRotation for rot_id.

				return NULL;
			}
	};
}

#endif
