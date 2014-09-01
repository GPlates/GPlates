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
		const GmlGridEnvelope::non_null_ptr_to_const_type &limits_,
		const axes_list_type &axes_,
		const GmlPoint::non_null_ptr_to_const_type &origin_,
		const offset_vector_list_type &offset_vectors_,
		const xml_attributes_type &xml_attributes_)
{
	return new GmlRectifiedGrid(
			limits_,
			axes_,
			origin_,
			offset_vectors_,
			xml_attributes_);
}


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
	GmlPoint::non_null_ptr_type origin_ = GmlPoint::create_from_pos_2d(
			// This version of create takes (lat, lon) but doesn't check for valid lat/lon ranges
			// in case georeferenced coordinates are not in a lat/lon coordinate system.
			// For example they could be in a *projection* coordinate system...
			std::make_pair(params.top_left_y_coordinate/*lat*/, params.top_left_x_coordinate/*lon*/));

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
	result->d_cached_georeferencing = georeferencing;

	return result;
}


std::ostream &
GPlatesPropertyValues::GmlRectifiedGrid::print_to(
		std::ostream &os) const
{
	return os << "GmlRectifiedGrid";
}


const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
GPlatesPropertyValues::GmlRectifiedGrid::convert_to_georeferencing() const
{
	if (d_cached_georeferencing)
	{
		// Already calculated, just use it.
		return d_cached_georeferencing;
	}

	if (d_offset_vectors.size() != 2)
	{
		return d_cached_georeferencing /* = boost::none */;
	}

	const offset_vector_type &longitude_offset_vector = d_offset_vectors[0];
	const offset_vector_type &latitude_offset_vector = d_offset_vectors[1];

	// NOTE: We don't call 'GmlPoint::point_in_lat_lon()' because that checks the lat/lon are in
	// valid ranges and our georeferenced origin might be in a *projection* coordinate system
	// (ie, not a lat/lon coordinate system) and hence could easily be outside the valid lat/lon range.
	const std::pair<double, double> &origin_2d = d_origin->point_2d();

	Georeferencing::parameters_type params = {{{
		origin_2d.second/*longitude*/,
		longitude_offset_vector[0],
		latitude_offset_vector[0],
		origin_2d.first/*latitude*/,
		longitude_offset_vector[1],
		latitude_offset_vector[1]
	}}};

	Georeferencing::non_null_ptr_type georeferencing = Georeferencing::create(params);
	d_cached_georeferencing = Georeferencing::non_null_ptr_to_const_type(georeferencing.get());

	return d_cached_georeferencing;
}

