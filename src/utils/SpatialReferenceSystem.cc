/* $Id: XmlNamespaces.cc 10554 2010-12-13 05:57:08Z mchin $ */

/**
 * \file
 *
 * Most recent change:
 *   $Date: 2010-12-13 16:57:08 +1100 (Mon, 13 Dec 2010) $
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
#include <QDebug>
#include <boost/scoped_ptr.hpp>
#include "ogr_spatialref.h"
#include "SpatialReferenceSystem.h"

void
GPlatesUtils::SpatialReferenceSystem::transform(
		const CoordinateReferenceSystem& from,
		const CoordinateReferenceSystem& to,
		std::vector<Coordinates>& points)
{
	//step 1: prepare input data.
	std::size_t len = points.size();
	boost::scoped_ptr<double> x(new double[len]);
	boost::scoped_ptr<double> y(new double[len]);
	boost::scoped_ptr<double> z;
	
	if(from.is_3D())
	{
		z.reset(new double[len]);
	}

	for(std::size_t i=0; i<len; ++i)
	{
		x.get()[i] = points[i].x();
		y.get()[i] = points[i].y();
		if(from.is_3D())
		{
			z.get()[i] = points[i].z();;
		}
	}

	//step 2: call OGR libary to transform.
	OGRSpatialReference from_srs, to_srs;
	OGRCoordinateTransformation *transform;

	from_srs.SetWellKnownGeogCS( from.name().toStdString().c_str() );
	to_srs.SetWellKnownGeogCS( to.name().toStdString().c_str());

	transform = OGRCreateCoordinateTransformation(&from_srs, &to_srs);
	if( transform == NULL )
	{
		//TODO:
		//throw exception
		qWarning() << "cannot create transform.";
		return;
	}

	if( !transform->Transform( len, x.get(), y.get(), z.get() ) )
	{
		//TODO:
		//throw exception
		qWarning() << "Error occurred during Transform.";
		return;
	}

	//step 3: prepare return data.
	points.clear();
	for(std::size_t j=0; j<len; j++)
	{
		std::vector<double> coordinates;
		coordinates.push_back(x.get()[j]);
		coordinates.push_back(y.get()[j]);
		if(to.is_3D())
		{
			coordinates.push_back(z.get()[j]);
		}
		points.push_back(Coordinates(coordinates,to));
	}
}




