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

#include <cmath>

#include "SphericalSubdivision.h"


GPlatesMaths::SphericalSubdivision::HierarchicalTriangularMeshTraversal::HierarchicalTriangularMeshTraversal() :
	vertex0(0, 0, 1),
	vertex1(1, 0, 0),
	vertex2(0, 1, 0),
	vertex3(-1, 0, 0),
	vertex4(0, -1, 0),
	vertex5(0, 0, -1)
{
}


const double GPlatesMaths::SphericalSubdivision::RhombicTriacontahedronTraversal::GOLDEN_RATIO = (1.0 + std::sqrt(5.0)) / 2.0;
const double GPlatesMaths::SphericalSubdivision::RhombicTriacontahedronTraversal::GOLDEN_RATIO_2 = std::pow((1.0 + std::sqrt(5.0)) / 2.0, 2.0);
const double GPlatesMaths::SphericalSubdivision::RhombicTriacontahedronTraversal::GOLDEN_RATIO_3 = std::pow((1.0 + std::sqrt(5.0)) / 2.0, 3.0);


GPlatesMaths::SphericalSubdivision::RhombicTriacontahedronTraversal::RhombicTriacontahedronTraversal() :
		vertex2 (normalise(GOLDEN_RATIO_2, 0, GOLDEN_RATIO_3)),
		vertex4 (normalise(0, GOLDEN_RATIO, GOLDEN_RATIO_3)),
		vertex6 (normalise(-GOLDEN_RATIO_2, 0, GOLDEN_RATIO_3)),
		vertex8 (normalise(0, -GOLDEN_RATIO, GOLDEN_RATIO_3)),
		vertex11(normalise(GOLDEN_RATIO_2, GOLDEN_RATIO_2, GOLDEN_RATIO_2)),
		vertex12(normalise(0, GOLDEN_RATIO_3, GOLDEN_RATIO_2)),
		vertex13(normalise(-GOLDEN_RATIO_2, GOLDEN_RATIO_2, GOLDEN_RATIO_2)),
		vertex16(normalise(-GOLDEN_RATIO_2, -GOLDEN_RATIO_2, GOLDEN_RATIO_2)),
		vertex17(normalise(0, -GOLDEN_RATIO_3, GOLDEN_RATIO_2)),
		vertex18(normalise(GOLDEN_RATIO_2, -GOLDEN_RATIO_2, GOLDEN_RATIO_2)),
		vertex20(normalise(GOLDEN_RATIO_3, 0, GOLDEN_RATIO)),
		vertex23(normalise(-GOLDEN_RATIO_3, 0, GOLDEN_RATIO)),
		vertex27(normalise(GOLDEN_RATIO_3, GOLDEN_RATIO_2, 0)),
		vertex28(normalise(GOLDEN_RATIO, GOLDEN_RATIO_3, 0)),
		vertex30(normalise(-GOLDEN_RATIO, GOLDEN_RATIO_3, 0)),
		vertex31(normalise(-GOLDEN_RATIO_3, GOLDEN_RATIO_2, 0)),
		vertex33(normalise(-GOLDEN_RATIO_3, -GOLDEN_RATIO_2, 0)),
		vertex34(normalise(-GOLDEN_RATIO, -GOLDEN_RATIO_3, 0)),
		vertex36(normalise(GOLDEN_RATIO, -GOLDEN_RATIO_3, 0)),
		vertex37(normalise(GOLDEN_RATIO_3, -GOLDEN_RATIO_2, 0)),
		vertex38(normalise(GOLDEN_RATIO_3, 0, -GOLDEN_RATIO)),
		vertex41(normalise(-GOLDEN_RATIO_3, 0, -GOLDEN_RATIO)),
		vertex45(normalise(GOLDEN_RATIO_2, GOLDEN_RATIO_2, -GOLDEN_RATIO_2)),
		vertex46(normalise(0, GOLDEN_RATIO_3, -GOLDEN_RATIO_2)),
		vertex47(normalise(-GOLDEN_RATIO_2, GOLDEN_RATIO_2, -GOLDEN_RATIO_2)),
		vertex50(normalise(-GOLDEN_RATIO_2, -GOLDEN_RATIO_2, -GOLDEN_RATIO_2)),
		vertex51(normalise(0, -GOLDEN_RATIO_3, -GOLDEN_RATIO_2)),
		vertex52(normalise(GOLDEN_RATIO_2, -GOLDEN_RATIO_2, -GOLDEN_RATIO_2)),
		vertex54(normalise(GOLDEN_RATIO_2, 0, -GOLDEN_RATIO_3)),
		vertex56(normalise(0, GOLDEN_RATIO, -GOLDEN_RATIO_3)),
		vertex58(normalise(-GOLDEN_RATIO_2, 0, -GOLDEN_RATIO_3)),
		vertex60(normalise(0, -GOLDEN_RATIO, -GOLDEN_RATIO_3))
{
}
