/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include <boost/intrusive_ptr.hpp>
#include "model/Model.h"
#include "maths/UnitVector3D.h"


namespace {
	typedef GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> ReconstructedPoint;
	typedef GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> ReconstructedPolyline;
}


void
GPlatesModel::Model::create_reconstruction(
		std::vector<ReconstructedPoint> &point_reconstructions,
		std::vector<ReconstructedPolyline> &polyline_reconstructions,
		const double &time,
		unsigned long root)
{
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::UnitVector3D;
	boost::intrusive_ptr<PointOnSphere> point = PointOnSphere::create_on_heap(UnitVector3D(1.0, 0.0, 0.0));
	point_reconstructions.push_back(ReconstructedPoint(point));
}
