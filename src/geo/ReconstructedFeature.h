/**
 * @file 
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_GEO_RECONSTRUCTEDFEATURE_H
#define GPLATES_GEO_RECONSTRUCTEDFEATURE_H

#include "maths/FiniteRotationSnapshotTable.h"

namespace GPlatesGeo {

	/**
	 * Our representation of a Feature that has been reconstructed to
	 * a particular point in time -- namely the time determined by
	 * @a m_finite_rotation_table.
	 */
	class ReconstructedFeature {

		public:

			ReconstructedFeature(
			 const GPlatesMaths::FiniteRotationSnapshotTable &table)
			 : m_finite_rotation_table(&table) {  }

			virtual
			~ReconstructedFeature() {  }

		private:

			/**
			 * FIXME: Jimmy will have to explain this monstrosity.
			 */
			const GPlatesMaths::FiniteRotationSnapshotTable *
			 m_finite_rotation_table;
	};
}

#endif
