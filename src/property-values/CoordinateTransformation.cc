/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <boost/scoped_array.hpp>
#include <ogr_spatialref.h>

#include "CoordinateTransformation.h"


boost::optional<GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_type>
GPlatesPropertyValues::CoordinateTransformation::create(
		const SpatialReferenceSystem::non_null_ptr_to_const_type &source_srs,
		const SpatialReferenceSystem::non_null_ptr_to_const_type &target_srs)
{
	// If the src and dst spatial reference systems are the same then return identity transformation.
	if (source_srs->get_ogr_srs().IsSame(&target_srs->get_ogr_srs()))
	{
		return create();
	}

	// Copy the SRSs in case the caller later modifies theirs.
	SpatialReferenceSystem::non_null_ptr_type source_srs_copy =
			SpatialReferenceSystem::create(source_srs->get_ogr_srs());
	SpatialReferenceSystem::non_null_ptr_type target_srs_copy =
			SpatialReferenceSystem::create(target_srs->get_ogr_srs());

	std::auto_ptr<OGRCoordinateTransformation> coordinate_transformation(
			OGRCreateCoordinateTransformation(
					&source_srs_copy->get_ogr_srs(),
					&target_srs_copy->get_ogr_srs()));

	if (coordinate_transformation.get() == NULL)
	{
		// Unable to create a transformation between the specified coordinate systems.
		return boost::none;
	}

	return non_null_ptr_type(
			new CoordinateTransformation(source_srs_copy, target_srs_copy, coordinate_transformation));
}


GPlatesPropertyValues::CoordinateTransformation::CoordinateTransformation() :
	d_source_srs(SpatialReferenceSystem::get_WGS84()),
	d_target_srs(SpatialReferenceSystem::get_WGS84())
{
	// boost::scoped_ptr destructor needs complete type.
	// Presumably happens in our constructor in case exception thrown (and need to destroy
	// already constructed data members).
}


GPlatesPropertyValues::CoordinateTransformation::CoordinateTransformation(
		const SpatialReferenceSystem::non_null_ptr_to_const_type &source_srs,
		const SpatialReferenceSystem::non_null_ptr_to_const_type &target_srs,
		std::auto_ptr<OGRCoordinateTransformation> ogr_coordinate_transformation) :
	d_source_srs(source_srs),
	d_target_srs(target_srs),
	d_ogr_coordinate_transformation(ogr_coordinate_transformation.release())
{
}


GPlatesPropertyValues::CoordinateTransformation::~CoordinateTransformation()
{
	// boost::scoped_ptr destructor needs complete type.
}


boost::optional<GPlatesPropertyValues::CoordinateTransformation::Coord>
GPlatesPropertyValues::CoordinateTransformation::transform(
		const Coord &coord) const
{
	Coord transformed_coord(coord);
	if (!transform_in_place(transformed_coord))
	{
		return boost::none;
	}

	return transformed_coord;
}


bool
GPlatesPropertyValues::CoordinateTransformation::transform_in_place(
		Coord &coord) const
{
	// If using identity transform then just return the coordinates unchanged.
	if (!d_ogr_coordinate_transformation)
	{
		return true;
	}

	if (coord.z)
	{
		// Transform x, y and z in place.
		if (!d_ogr_coordinate_transformation->Transform(1, &coord.x, &coord.y, &coord.z.get()))
		{
			return false;
		}
	}
	else
	{
		// Transform x and y in place.
		if (!d_ogr_coordinate_transformation->Transform(1, &coord.x, &coord.y))
		{
			return false;
		}
	}

	return true;
}


bool
GPlatesPropertyValues::CoordinateTransformation::transform_in_place(
		double *x,
		double *y,
		double *z) const
{
	// If using identity transform then just return the coordinates unchanged.
	if (!d_ogr_coordinate_transformation)
	{
		return true;
	}

	// Transform x and y (and optionally z) in place.
	return d_ogr_coordinate_transformation->Transform(1, x, y, z);
}


bool
GPlatesPropertyValues::CoordinateTransformation::transform(
		const std::vector<Coord> &transform_input,
		std::vector<Coord> &transform_output) const
{
	// Copy the input coords.
	std::vector<Coord> coords(transform_input);

	// Transform the input coords.
	if (!transform_in_place(coords))
	{
		return false;
	}

	// Copy the transformed coords to the output.
	transform_output.insert(transform_output.end(), coords.begin(), coords.end());

	return true;
}


bool
GPlatesPropertyValues::CoordinateTransformation::transform_in_place(
		std::vector<Coord> &coords) const
{
	// If using identity transform then just return the coordinates unchanged.
	if (!d_ogr_coordinate_transformation)
	{
		return true;
	}

	const unsigned int count = coords.size();

	// If any coords have a z value.
	bool have_z_coord = false;

	// See if any coords have a z value.
	for (unsigned int n = 0; n < count; ++n)
	{
		const Coord &coord = coords[n];
		if (coord.z)
		{
			have_z_coord = true;
			break;
		}
	}

	// Allocate working space.
	boost::scoped_array<double> x(new double[count]);
	boost::scoped_array<double> y(new double[count]);
	boost::scoped_array<double> z;
	if (have_z_coord)
	{
		z.reset(new double[count]);
	}

	// Copy input to working space.
	for (unsigned int n = 0; n < count; ++n)
	{
		const Coord &coord = coords[n];

		x.get()[n] = coord.x;
		y.get()[n] = coord.y;
		if (have_z_coord)
		{
			z.get()[n] = coord.z ? coord.z.get() : 0;
		}
	}

	// Transform the x and y (and optionally z) arrays in working space.
	if (!d_ogr_coordinate_transformation->Transform(count, x.get(), y.get(), z.get()/*could be NULL*/))
	{
		return false;
	}

	// Copy working space to output.
	for (unsigned int n = 0; n < count; ++n)
	{
		Coord &coord = coords[n];

		coord.x = x.get()[n];
		coord.y = y.get()[n];
		if (coord.z)
		{
			coord.z = z.get()[n];
		}
	}

	return true;
}


bool
GPlatesPropertyValues::CoordinateTransformation::transform_in_place(
		unsigned int count,
		double *x,
		double *y,
		double *z) const
{
	// If using identity transform then just return the coordinates unchanged.
	if (!d_ogr_coordinate_transformation)
	{
		return true;
	}

	// Transform the x and y (and optionally z) arrays in place.
	return d_ogr_coordinate_transformation->Transform(count, x, y, z);
}
