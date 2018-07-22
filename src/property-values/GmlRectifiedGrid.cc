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

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/LatLonPoint.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlRectifiedGrid::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("RectifiedGrid");


const GPlatesPropertyValues::GmlRectifiedGrid::non_null_ptr_type
GPlatesPropertyValues::GmlRectifiedGrid::create(
		const GmlGridEnvelope::non_null_ptr_type &limits_,
		const axes_list_type &axes_,
		const GmlPoint::non_null_ptr_type &origin_,
		const offset_vector_list_type &offset_vectors_,
		const xml_attributes_type &xml_attributes_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(
			new GmlRectifiedGrid(
					transaction, limits_, axes_, origin_, offset_vectors_, xml_attributes_));
	transaction.commit();

	return ptr;
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
	Georeferencing::parameters_type params = georeferencing->get_parameters();
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

	const Revision &revision = result->get_current_revision<Revision>();
	revision.cached_georeferencing = georeferencing;

	return result;
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_limits(
		const GmlGridEnvelope::non_null_ptr_type &limits_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().limits.change(
			revision_handler.get_model_transaction(), limits_);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_axes(
		const axes_list_type &axes_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().axes = axes_;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_origin(
		GmlPoint::non_null_ptr_type origin_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);

	Revision &revision = revision_handler.get_revision<Revision>();

	revision.origin.change(revision_handler.get_model_transaction(), origin_);

	// Invalidate the georeferencing cache because that's calculated using the origin.
	revision.cached_georeferencing = boost::none;

	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_offset_vectors(
		const offset_vector_list_type &offset_vectors_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);

	Revision &revision = revision_handler.get_revision<Revision>();

	revision.offset_vectors = offset_vectors_;

	// Invalidate the georeferencing cache because that's calculated using the offset vectors.
	revision.cached_georeferencing = boost::none;

	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlRectifiedGrid::set_xml_attributes(
		const xml_attributes_type &xml_attributes_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().xml_attributes = xml_attributes_;
	revision_handler.commit();
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

	// NOTE: We don't call 'GmlPoint::get_point_in_lat_lon()' because that checks the lat/lon are in
	// valid ranges and our georeferenced origin might be in a *projection* coordinate system
	// (ie, not a lat/lon coordinate system) and hence could easily be outside the valid lat/lon range.
	//
	// Even if there's no projection it might still be a *gridline* registered global raster which places
	// the centres of the top and bottom pixels at the North and South poles and hence GDAL adjusted the
	// origin by half a pixel (such that it is the *corner* of the top-left pixel, instead of *centre*).
	// For example, a 1 degree *gridline*-registereed raster would have an origin latitude of 90.5 degrees
	// to make it *pixel* registered (the registration GDAL and GPlates uses).
	const std::pair<double, double> &origin_2d = revision.origin.get_revisionable()->get_point_2d();

	Georeferencing::parameters_type params = {{{
		origin_2d.second/*longitude*/,
		longitude_offset_vector[0],
		latitude_offset_vector[0],
		origin_2d.first/*latitude*/,
		longitude_offset_vector[1],
		latitude_offset_vector[1]
	}}};

	Georeferencing::non_null_ptr_type georeferencing = Georeferencing::create(params);
	revision.cached_georeferencing = Georeferencing::non_null_ptr_to_const_type(georeferencing.get());

	return revision.cached_georeferencing;
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GmlRectifiedGrid::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	if (child_revisionable == revision.limits.get_revisionable())
	{
		return revision.limits.clone_revision(transaction);
	}
	if (child_revisionable == revision.origin.get_revisionable())
	{
		// Invalidate the georeferencing cache because that's calculated using the origin.
		revision.cached_georeferencing = boost::none;

		return revision.origin.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


bool
GPlatesPropertyValues::GmlRectifiedGrid::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return *limits.get_revisionable() == *other_revision.limits.get_revisionable() &&
			axes == other_revision.axes &&
			*origin.get_revisionable() == *other_revision.origin.get_revisionable() &&
			offset_vectors == other_revision.offset_vectors &&
			xml_attributes == other_revision.xml_attributes &&
			PropertyValue::Revision::equality(other);
}
