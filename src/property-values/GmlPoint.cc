/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2010 The University of Sydney, Australia
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

#include <iostream>

#include "GmlPoint.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "model/BubbleUpRevisionHandler.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlPoint::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("Point");


GPlatesMaths::LatLonPoint
GPlatesPropertyValues::GmlPoint::get_point_in_lat_lon() const
{
	const std::pair<double, double> &pos_2d = get_point_2d();

	// Note that the 2D point stores as (lat, lon) which is the order stored in GPML file.
	// This will throw if the lat/lon is outside valid range.
	return GPlatesMaths::LatLonPoint(pos_2d.first/*lat*/, pos_2d.second/*lon*/);
}


void
GPlatesPropertyValues::GmlPoint::set_point(
		const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &p)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	Revision &revision = revision_handler.get_revision<Revision>();

	revision.point_on_sphere = p;
	revision.point_2d = boost::none;

	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlPoint::set_gml_property(
		GmlProperty gml_property_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().gml_property = gml_property_;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlPoint::print_to(
		std::ostream &os) const
{
	return os << *get_point();
}


bool
GPlatesPropertyValues::GmlPoint::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return get_point_2d() == other_revision.get_point_2d() &&
			gml_property == other_revision.gml_property &&
			PropertyValue::Revision::equality(other);
}


const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
GPlatesPropertyValues::GmlPoint::Revision::get_point() const
{
	if (!point_on_sphere)
	{
		// At least one type of point must always exist.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				point_2d,
				GPLATES_ASSERTION_SOURCE);

		// Note that the 2D point stores as (lat, lon) which is the order stored in GPML file.
		// This will throw if the lat/lon is outside valid range.
		const GPlatesMaths::LatLonPoint lat_lon_point(
				point_2d->first/*lat*/,
				point_2d->second/*lon*/);

		point_on_sphere = GPlatesMaths::make_point_on_sphere(lat_lon_point).clone_as_point();
	}

	return point_on_sphere.get();
}


const std::pair<double, double> &
GPlatesPropertyValues::GmlPoint::Revision::get_point_2d() const
{
	if (!point_2d)
	{
		// At least one type of point must always exist.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				point_on_sphere,
				GPLATES_ASSERTION_SOURCE);

		const GPlatesMaths::LatLonPoint lat_lon_point = make_lat_lon_point(*point_on_sphere.get());

		// Note that the 2D point stores as (lat, lon) which is the order stored in GPML file.
		point_2d = std::make_pair(lat_lon_point.latitude(), lat_lon_point.longitude());
	}

	return point_2d.get();
}
