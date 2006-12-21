/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_MODEL_H
#define GPLATES_MODEL_MODEL_H

#include <vector>
#include "model/ModelInterface.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace GPlatesModel
{
	class Model :
		public ModelInterface
	{
	public:
		virtual
		void
		create_reconstruction(
				std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> > &point_reconstructions,
				std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> > &polyline_reconstructions,
				const double &time,
				unsigned long root);
	};
}

#endif  // GPLATES_MODEL_MODEL_H
