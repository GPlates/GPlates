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

#include <algorithm>
#include <boost/foreach.hpp>

#include "RasterLayerParams.h"

#include "ExtractRasterFeatureProperties.h"

#include "model/FeatureVisitor.h"

#include "property-values/RawRasterUtils.h"


GPlatesAppLogic::RasterLayerParams::RasterLayerParams() :
	d_band_name(GPlatesUtils::UnicodeString()),
	d_raster_type(GPlatesPropertyValues::RasterType::UNKNOWN)
{
}


void
GPlatesAppLogic::RasterLayerParams::set_band_name(
		GPlatesPropertyValues::TextContent band_name)
{
	bool modified_params = false;

	// Is the band name one of the available bands in the raster?
	// If not, then change the band name to be the first of the available bands.
	boost::optional<std::size_t> band_name_index;
	if (!d_band_names.empty())
	{
		band_name_index = find_raster_band_name(d_band_names, band_name);
		if (!band_name_index)
		{
			// Set the band name using the default band index of zero.
			band_name_index = 0;
			band_name = d_band_names[band_name_index.get()]->value();
		}
	}

	if (d_band_name != band_name)
	{
		d_band_name = band_name;

		modified_params = true;
	}

	// Set the statistics associated with the raster band.
	if (band_name_index &&
		band_name_index.get() < d_band_statistics.size())
	{
		d_band_statistic = d_band_statistics[band_name_index.get()];
	}

	if (modified_params)
	{
		Q_EMIT modified_band_name(*this);
		emit_modified();
	}
}


void
GPlatesAppLogic::RasterLayerParams::set_raster_feature(
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> raster_feature)
{
	d_raster_feature = raster_feature;

	// Clear everything (except band name) in case error (and return early).
	d_band_names.clear();
	d_band_statistic = GPlatesPropertyValues::RasterStatistics();
	d_band_statistics.clear();
	d_georeferencing = boost::none;
	d_spatial_reference_system = boost::none;
	d_raster_type = GPlatesPropertyValues::RasterType::UNKNOWN;

	// If there is no raster feature then clear everything.
	if (!d_raster_feature)
	{
		// TODO: Only emit if actually changed. For now just emitting always.
		emit_modified();
		return;
	}

	// NOTE: We are visiting properties at (default) present day.
	// Raster statistics, for example, will change over time for time-dependent rasters.
	ExtractRasterFeatureProperties visitor;
	visitor.visit_feature(d_raster_feature.get());

	// Get the georeferencing.
	d_georeferencing = visitor.get_georeferencing();

	// Get the spatial reference system.
	d_spatial_reference_system = visitor.get_spatial_reference_system();

	// If there are raster band names...
	boost::optional<std::size_t> band_name_index;
	if (visitor.get_raster_band_names() &&
		!visitor.get_raster_band_names()->empty())
	{
		d_band_names = visitor.get_raster_band_names().get();

		// Is the selected band name one of the available bands in the raster?
		// If not, then change the band name to be the first of the available bands.
		band_name_index = find_raster_band_name(d_band_names, d_band_name);
		if (!band_name_index)
		{
			// Set the band name using the default band index of zero.
			band_name_index = 0;
			d_band_name = d_band_names[band_name_index.get()]->value();
		}
	}

	if (visitor.get_proxied_rasters())
	{
		const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &proxied_rasters =
				visitor.get_proxied_rasters().get();

		BOOST_FOREACH(GPlatesPropertyValues::RawRaster::non_null_ptr_type proxied_raster, proxied_rasters)
		{
			GPlatesPropertyValues::RasterStatistics *raster_statistics =
					GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*proxied_raster);

			d_band_statistics.push_back(
					raster_statistics
							? *raster_statistics
							: GPlatesPropertyValues::RasterStatistics());

			// Get the raster type as an enumeration.
			// All band types should be the same - if they're not then the type will get set to the last band.
			d_raster_type = GPlatesPropertyValues::RawRasterUtils::get_raster_type(*proxied_raster);
		}

		if (band_name_index &&
			band_name_index.get() < d_band_statistics.size())
		{
			d_band_statistic = d_band_statistics[band_name_index.get()];
		}
	}

	// Notify observers (such as RasterVisualLayerParams) that the band name(s) have changed -
	// since it might need to update the colour palette if a new raw raster band is used.
	//
	// TODO: Only emit if actually changed. For now just emitting always.
	emit_modified();
}
