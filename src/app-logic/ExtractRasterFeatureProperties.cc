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
namespace
{
	/**
	 * Visits a feature collection and determines whether the feature collection
	 * contains any raster features.
	 *
	 * The heuristic that we're using here is that it is a raster feature if there
	 * is all of the following:
	 *  - GmlRectifiedGrid inside a GpmlConstantValue inside a gpml:domainSet
	 *    top level property.
	 *  - GmlFile inside a GpmlConstantValue or a GpmlPiecewiseAggregation inside
	 *    a gpml:rangeSet top level property.
	 *  - any proxied raw raster (for any band) in the GmlFile is initialised.
	 *    TODO: Check only the band number that this layer task is interested in.
	 *          Although maybe not because the user could switch the band number in
	 *          the layer controls and this class is designed to test if a raster layer
	 *          can process the input feature.
	 *          So probably better to leave as is and just check that any band can be processed.
	 *  - GpmlRasterBandNames (not inside any time dependent structure) inside a
	 *    gpml:bandNames top level property.
	 */
	class CanResolveRasterFeature :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		CanResolveRasterFeature() :
			d_seen_gml_rectified_grid(false),
			d_seen_gml_file(false),
			d_seen_gpml_raster_band_names(false),
			d_inside_constant_value(false),
			d_inside_piecewise_aggregation(false),
			d_has_raster_feature(false)
		{  }

		bool
		has_raster_feature() const
		{
			return d_has_raster_feature;
		}

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			d_seen_gml_rectified_grid = false;
			d_seen_gml_file = false;
			d_seen_at_least_one_valid_proxied_raw_raster = false;
			d_seen_gpml_raster_band_names = false;

			return true;
		}

		virtual
		void
		finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			if (d_seen_gml_rectified_grid &&
					d_seen_gml_file &&
					d_seen_at_least_one_valid_proxied_raw_raster &&
					d_seen_gpml_raster_band_names)
			{
				d_has_raster_feature = true;
			}
		}

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			d_inside_constant_value = true;
			gpml_constant_value.value()->accept_visitor(*this);
			d_inside_constant_value = false;
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			d_inside_piecewise_aggregation = true;
			const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow> &time_windows =
					gpml_piecewise_aggregation.time_windows();
			BOOST_FOREACH(
					GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_to_const_type time_window,
					time_windows)
			{
				time_window->time_dependent_value()->accept_visitor(*this);
			}
			d_inside_piecewise_aggregation = false;
		}

		virtual
		void
		visit_gml_rectified_grid(
				const GPlatesPropertyValues::GmlRectifiedGrid &gml_rectified_grid)
		{
			static const GPlatesModel::PropertyName DOMAIN_SET =
				GPlatesModel::PropertyName::create_gpml("domainSet");

			if (d_inside_constant_value)
			{
				const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
				if (propname && *propname == DOMAIN_SET)
				{
					d_seen_gml_rectified_grid = true;
				}
			}
		}

		virtual
		void
		visit_gml_file(
				const GPlatesPropertyValues::GmlFile &gml_file)
		{
			static const GPlatesModel::PropertyName RANGE_SET =
				GPlatesModel::PropertyName::create_gpml("rangeSet");

			if (d_inside_constant_value || d_inside_piecewise_aggregation)
			{
				const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
				if (propname && *propname == RANGE_SET)
				{
					d_seen_gml_file = true;

					// Make sure we have at least one initialised proxied raw raster for a band.
					// If we have at least one then it means we can process something (even if
					// it's only one band).
					const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> proxied_rasters =
							gml_file.get_proxied_raw_rasters();
					BOOST_FOREACH(
							const GPlatesPropertyValues::RawRaster::non_null_ptr_type &proxied_raster,
							proxied_rasters)
					{
						if (GPlatesPropertyValues::RawRasterUtils::has_proxied_data(*proxied_raster))
						{
							d_seen_at_least_one_valid_proxied_raw_raster = true;
							break;
						}
					}
				}
			}
		}

		virtual
		void
		visit_gpml_raster_band_names(
				const GPlatesPropertyValues::GpmlRasterBandNames &gpml_raster_band_names)
		{
			static const GPlatesModel::PropertyName BAND_NAMES =
				GPlatesModel::PropertyName::create_gpml("bandNames");

			if (!d_inside_constant_value && !d_inside_piecewise_aggregation)
			{
				const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
				if (propname && *propname == BAND_NAMES)
				{
					d_seen_gpml_raster_band_names = true;
				}
			}
		}

	private:
		bool d_seen_gml_rectified_grid;
		bool d_seen_gml_file;
		bool d_seen_at_least_one_valid_proxied_raw_raster;
		bool d_seen_gpml_raster_band_names;

		bool d_inside_constant_value;
		bool d_inside_piecewise_aggregation;
		
		bool d_has_raster_feature;
	};
}


bool
GPlatesAppLogic::is_raster_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	CanResolveRasterFeature visitor;
	visitor.visit_feature(feature);
	return visitor.has_raster_feature();
}


bool
GPlatesAppLogic::contains_raster_feature(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	CanResolveRasterFeature visitor;
	for (GPlatesModel::FeatureCollectionHandle::const_iterator iter = feature_collection->begin();
			iter != feature_collection->end(); ++iter)
	{
		visitor.visit_feature(iter);
	}
	return visitor.has_raster_feature();
}


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
	const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow> &time_windows =
			gpml_piecewise_aggregation.time_windows();
	BOOST_FOREACH(
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_to_const_type time_window,
			time_windows)
	{
		const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
				time_window->valid_time();

		// If the time window period contains the current reconstruction time then visit.
		// The time periods should be mutually exclusive - if we happen to be it
		// two time periods then we're probably right on the boundary between the two
		// and then it doesn't really matter which one we choose.
		if (time_period->contains(d_reconstruction_time))
		{
			time_window->time_dependent_value()->accept_visitor(*this);
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
			d_proxied_rasters = gml_file.get_proxied_raw_rasters();
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
			d_raster_band_names = gpml_raster_band_names.get_band_names();
		}
	}
}
