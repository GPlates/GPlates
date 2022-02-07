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

#ifndef GPLATES_APP_LOGIC_EXTRACTRASTERFEATUREPROPERTIES_H
#define GPLATES_APP_LOGIC_EXTRACTRASTERFEATUREPROPERTIES_H

#include <boost/optional.hpp>

#include "model/FeatureVisitor.h"

#include "property-values/Georeferencing.h"
#include "property-values/GmlFile.h"
#include "property-values/GmlRectifiedGrid.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/RasterStatistics.h"
#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/SpatialReferenceSystem.h"
#include "property-values/XsString.h"


namespace GPlatesAppLogic
{
	/**
	 * Returns true if the specified feature is a raster feature.
	 */
	bool
	is_raster_feature(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature);

	/**
	 * Returns true if the specified feature collection contains a raster feature.
	 */
	bool
	contains_raster_feature(
			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


	/**
	 * Visits a raster feature and extracts the following properties from it:
	 *  - GmlRectifiedGrid inside a GpmlConstantValue inside a gpml:domainSet
	 *    top level property.
	 *  - GmlFile inside a GpmlConstantValue or a GpmlPiecewiseAggregation
	 *    inside a gpml:rangeSet top-level property.
	 *  - GpmlRasterBandNames (not inside any time-dependent structure) inside a
	 *    gpml:bandNames top-level property.
	 *
	 * NOTE: The properties are extracted at the specified reconstruction time.
	 */
	class ExtractRasterFeatureProperties :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		explicit
		ExtractRasterFeatureProperties(
				const double &reconstruction_time = 0);


		const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
		get_georeferencing() const;


		/**
		 * FIXME: Currently this is extracted from the (possibly time-dependent) raster at the
		 * reconstruction time passed into constructor.
		 * Later, when the spatial reference system is stored in a property value, this will not
		 * potentially vary with the reconstruction time.
		 */
		const boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> &
		get_spatial_reference_system() const;


		const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
		get_proxied_rasters() const;


		const boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> &
		get_raster_band_names() const;


		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);


		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);


		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);


		virtual
		void
		visit_gml_rectified_grid(
				const GPlatesPropertyValues::GmlRectifiedGrid &gml_rectified_grid);


		virtual
		void
		visit_gml_file(
				const GPlatesPropertyValues::GmlFile &gml_file);


		virtual
		void
		visit_gpml_raster_band_names(
				const GPlatesPropertyValues::GpmlRasterBandNames &gpml_raster_band_names);

	private:
		/**
		 * The reconstruction time at which properties are extracted.
		 */
		GPlatesPropertyValues::GeoTimeInstant d_reconstruction_time;

		/**
		 * The georeferencing for the raster - currently treated as a constant value over time.
		 */
		boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> d_georeferencing;

		/**
		 * The raster's spatial reference system.
		 *
		 * Currently treated as a constant value over time.
		 */
		boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> d_spatial_reference_system;

		/**
		 * The proxied rasters of the first GmlFile encountered.
		 *
		 * The reason why we are only interested in the first GmlFile encountered is
		 * that the auto-generated raster colour palette is created based on the first
		 * frame of a time-dependent raster sequence.
		 */
		boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> >
			d_proxied_rasters;

		/**
		 * The list of band names - one for each proxied raster.
		 */
		boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> d_raster_band_names;

		bool d_inside_constant_value;
		bool d_inside_piecewise_aggregation;
	};


	/**
	 * Returns the index of @a band_name inside @a band_names_list if present.
	 * If not present, returns boost::none.
	 */
	inline
	boost::optional<std::size_t>
	find_raster_band_name(
			const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &band_names_list,
			const GPlatesPropertyValues::TextContent &band_name)
	{
		for (std::size_t i = 0; i != band_names_list.size(); ++i)
		{
			if (band_names_list[i].get_name()->get_value() == band_name)
			{
				return i;
			}
		}
		return boost::none;
	}
}

#endif // GPLATES_APP_LOGIC_EXTRACTRASTERFEATUREPROPERTIES_H
