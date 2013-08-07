/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "GmlRectifiedGrid.h"

#include "maths/LatLonPoint.h"


const GPlatesPropertyValues::GmlRectifiedGrid::non_null_ptr_type
GPlatesPropertyValues::GmlRectifiedGrid::create(
		const Georeferencing::non_null_ptr_to_const_type &georeferencing,
		unsigned int raster_width,
		unsigned int raster_height,
		const xml_attributes_type &xml_attributes_)
{
	// The GridEnvelope describes the dimensions of the grid itself.
	GmlGridEnvelope::integer_list_type low, high;
	low.push_back(0);
	low.push_back(0);
	high.push_back(raster_width);
	high.push_back(raster_height);
	GmlGridEnvelope::non_null_ptr_type limits_ = GmlGridEnvelope::create(low, high);

	// Assume that if you're using georeferencing, it's lon-lat.
	axes_list_type axes_;
	axes_.push_back(XsString::create("longitude"));
	axes_.push_back(XsString::create("latitude"));

	// The origin is the top-left corner in the georeferencing.
	Georeferencing::parameters_type params = georeferencing->parameters();
	GmlPoint::non_null_ptr_type origin_ = GmlPoint::create(
			/* this version of create takes (lon, lat) */
			std::make_pair(params.top_left_x_coordinate, params.top_left_y_coordinate));

	// The x-axis (longitude) offset vector.
	offset_vector_type longitude_offset_vector;
	longitude_offset_vector.push_back(params.x_component_of_pixel_width);
	longitude_offset_vector.push_back(params.y_component_of_pixel_width);

	// The y-axis (latitude) offset vector.
	offset_vector_type latitude_offset_vector;
	latitude_offset_vector.push_back(params.x_component_of_pixel_height);
	latitude_offset_vector.push_back(params.y_component_of_pixel_height);

	offset_vector_list_type offset_vectors_;
	offset_vectors_.push_back(longitude_offset_vector);
	offset_vectors_.push_back(latitude_offset_vector);

	non_null_ptr_type result = create(
			limits_,
			axes_,
			origin_,
			offset_vectors_,
			xml_attributes_);

	const Revision &revision = result->get_current_revision<Revision>();
	revision.cached_georeferencing = georeferencing;

	return result;
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_limits(
		const GmlGridEnvelope::non_null_ptr_to_const_type &limits_)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().limits = limits_;
	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_axes(
		const axes_list_type &axes_)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().axes = axes_;
	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_origin(
		const GmlPoint::non_null_ptr_to_const_type &origin_)
{
	MutableRevisionHandler revision_handler(this);

	Revision &revision = revision_handler.get_mutable_revision<Revision>();

	revision.origin = origin_;

	// Invalidate the georeferencing cache because that's calculated using the origin.
	revision.cached_georeferencing = boost::none;

	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_offset_vectors(
		const offset_vector_list_type &offset_vectors_)
{
	MutableRevisionHandler revision_handler(this);

	Revision &revision = revision_handler.get_mutable_revision<Revision>();

	revision.offset_vectors = offset_vectors_;

	// Invalidate the georeferencing cache because that's calculated using the offset vectors.
	revision.cached_georeferencing = boost::none;

	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_xml_attributes(
		const xml_attributes_type &xml_attributes_)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().xml_attributes = xml_attributes_;
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GmlRectifiedGrid::print_to(
		std::ostream &os) const
{
	return os << "GmlRectifiedGrid";
}


const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type>
GPlatesPropertyValues::GmlRectifiedGrid::convert_to_georeferencing() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (revision.cached_georeferencing)
	{
		// Already calculated, just use it.
		return revision.cached_georeferencing;
	}

	if (revision.offset_vectors.size() != 2)
	{
		return revision.cached_georeferencing /* = boost::none */;
	}

	const offset_vector_type &longitude_offset_vector = revision.offset_vectors[0];
	const offset_vector_type &latitude_offset_vector = revision.offset_vectors[1];

	GPlatesMaths::LatLonPoint llp = revision.origin.get_const()->point_in_lat_lon();

	Georeferencing::parameters_type params = {{{
		llp.longitude(),
		longitude_offset_vector[0],
		latitude_offset_vector[0],
		llp.latitude(),
		longitude_offset_vector[1],
		latitude_offset_vector[1]
	}}};

	Georeferencing::non_null_ptr_type georeferencing = Georeferencing::create(params);
	revision.cached_georeferencing = Georeferencing::non_null_ptr_to_const_type(georeferencing.get());

	return revision.cached_georeferencing;
}


bool
GPlatesPropertyValues::GmlRectifiedGrid::Revision::equality(
		const GPlatesModel::PropertyValue::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return *limits.get_const() == *other_revision.limits.get_const() &&
			axes == other_revision.axes &&
			*origin.get_const() == *other_revision.origin.get_const() &&
			offset_vectors == other_revision.offset_vectors &&
			xml_attributes == other_revision.xml_attributes &&
			GPlatesModel::PropertyValue::Revision::equality(other);
}
