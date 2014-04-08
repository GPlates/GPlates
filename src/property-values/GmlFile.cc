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

#include "GmlFile.h"


const GPlatesPropertyValues::GmlFile::non_null_ptr_type
GPlatesPropertyValues::GmlFile::create(
		const composite_value_type &range_parameters_,
		const XsString::non_null_ptr_to_const_type &file_name_,
		const XsString::non_null_ptr_to_const_type &file_structure_,
		const boost::optional<XsString::non_null_ptr_to_const_type> &mime_type_,
		const boost::optional<XsString::non_null_ptr_to_const_type> &compression_,
		GPlatesFileIO::ReadErrorAccumulation *read_errors_)
{
	return new GmlFile(
			range_parameters_,
			file_name_,
			file_structure_,
			mime_type_,
			compression_,
			read_errors_);
}


GPlatesPropertyValues::GmlFile::GmlFile(
		const composite_value_type &range_parameters_,
		const XsString::non_null_ptr_to_const_type &file_name_,
		const XsString::non_null_ptr_to_const_type &file_structure_,
		const boost::optional<XsString::non_null_ptr_to_const_type> &mime_type_,
		const boost::optional<XsString::non_null_ptr_to_const_type> &compression_,
		GPlatesFileIO::ReadErrorAccumulation *read_errors_) :
	PropertyValue(),
	d_range_parameters(range_parameters_),
	d_file_name(file_name_),
	d_file_structure(file_structure_),
	d_mime_type(mime_type_),
	d_compression(compression_),
	d_proxied_raster_cache(ProxiedRasterCache::create(file_name_->value(), read_errors_))
{  }


GPlatesPropertyValues::GmlFile::GmlFile(
		const GmlFile &other) :
	PropertyValue(other), /* share instance id */
	d_range_parameters(other.d_range_parameters),
	d_file_name(other.d_file_name),
	d_file_structure(other.d_file_structure),
	d_mime_type(other.d_mime_type),
	d_compression(other.d_compression),
	d_proxied_raster_cache(other.d_proxied_raster_cache)
{  }


std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesPropertyValues::GmlFile::proxied_raw_rasters() const
{
	return d_proxied_raster_cache->proxied_raw_rasters();
}


boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type>
GPlatesPropertyValues::GmlFile::get_spatial_reference_system() const
{
	return d_proxied_raster_cache->get_spatial_reference_system();
}


std::ostream &
GPlatesPropertyValues::GmlFile::print_to(
		std::ostream &os) const
{
	return os << d_file_name;
}

