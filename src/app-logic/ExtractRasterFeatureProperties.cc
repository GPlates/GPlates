/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#include <boost/foreach.hpp>

#include "ExtractRasterFeatureProperties.h"


GPlatesAppLogic::ExtractRasterFeatureProperties::ExtractRasterFeatureProperties(
			const double &reconstruction_time) :
	d_reconstruction_time(reconstruction_time),
	d_inside_constant_value(false),
	d_inside_piecewise_aggregation(false)
{
}


const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
GPlatesAppLogic::ExtractRasterFeatureProperties::get_georeferencing() const
{
	return d_georeferencing;
}


const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
GPlatesAppLogic::ExtractRasterFeatureProperties::get_proxied_rasters() const
{
	return d_proxied_rasters;
}


const boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> &
GPlatesAppLogic::ExtractRasterFeatureProperties::get_raster_band_names() const
{
	return d_raster_band_names;
}


bool
GPlatesAppLogic::ExtractRasterFeatureProperties::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	d_georeferencing = boost::none;
	d_proxied_rasters = boost::none;
	d_raster_band_names = boost::none;

	return true;
}


void
GPlatesAppLogic::ExtractRasterFeatureProperties::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	d_inside_constant_value = true;
	gpml_constant_value.value()->accept_visitor(*this);
	d_inside_constant_value = false;
}


void
GPlatesAppLogic::ExtractRasterFeatureProperties::visit_gpml_piecewise_aggregation(
		const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	d_inside_piecewise_aggregation = true;
	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows =
		gpml_piecewise_aggregation.time_windows();
	BOOST_FOREACH(const GPlatesPropertyValues::GpmlTimeWindow &time_window, time_windows)
	{
		const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
				time_window.valid_time();

		// If the time window period contains the current reconstruction time then visit.
		// The time periods should be mutually exclusive - if we happen to be it
		// two time periods then we're probably right on the boundary between the two
		// and then it doesn't really matter which one we choose.
		if (time_period->contains(d_reconstruction_time))
		{
			time_window.time_dependent_value()->accept_visitor(*this);
		}
	}
	d_inside_piecewise_aggregation = false;
}


void
GPlatesAppLogic::ExtractRasterFeatureProperties::visit_gml_rectified_grid(
		const GPlatesPropertyValues::GmlRectifiedGrid &gml_rectified_grid)
{
	static const GPlatesModel::PropertyName DOMAIN_SET =
		GPlatesModel::PropertyName::create_gpml("domainSet");

	if (d_inside_constant_value)
	{
		const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
		if (propname && *propname == DOMAIN_SET)
		{
			d_georeferencing = gml_rectified_grid.convert_to_georeferencing();
		}
	}
}


void
GPlatesAppLogic::ExtractRasterFeatureProperties::visit_gml_file(
		const GPlatesPropertyValues::GmlFile &gml_file)
{
	static const GPlatesModel::PropertyName RANGE_SET =
		GPlatesModel::PropertyName::create_gpml("rangeSet");

	if (d_inside_constant_value || d_inside_piecewise_aggregation)
	{
		const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
		if (propname && *propname == RANGE_SET)
		{
			d_proxied_rasters = gml_file.proxied_raw_rasters();
		}
	}
}


void
GPlatesAppLogic::ExtractRasterFeatureProperties::visit_gpml_raster_band_names(
		const GPlatesPropertyValues::GpmlRasterBandNames &gpml_raster_band_names)
{
	static const GPlatesModel::PropertyName BAND_NAMES =
		GPlatesModel::PropertyName::create_gpml("bandNames");

	if (!d_inside_constant_value && !d_inside_piecewise_aggregation)
	{
		const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
		if (propname && *propname == BAND_NAMES)
		{
			d_raster_band_names = gpml_raster_band_names.band_names();
		}
	}
}
