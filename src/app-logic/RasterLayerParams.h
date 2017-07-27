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

#ifndef GPLATES_APP_LOGIC_RASTERLAYERPARAMS_H
#define GPLATES_APP_LOGIC_RASTERLAYERPARAMS_H

#include <vector>
#include <boost/optional.hpp>
#include <QObject>

#include "LayerParams.h"

#include "model/FeatureHandle.h"

#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RasterStatistics.h"
#include "property-values/RasterType.h"
#include "property-values/RawRaster.h"
#include "property-values/SpatialReferenceSystem.h"
#include "property-values/TextContent.h"


namespace GPlatesAppLogic
{
	/**
	 * App-logic parameters for a raster layer.
	 */
	class RasterLayerParams :
			public LayerParams
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<RasterLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const RasterLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new RasterLayerParams());
		}

		/**
		 * Sets the name of the band, of the raster, selected for processing.
		 *
		 * Emits signals 'modified_band_name' and 'modified' if a change detected. When the band name
		 * is changed the statistics of the current band (@a get_band_statistic) will change also.
		 */
		void
		set_band_name(
				GPlatesPropertyValues::TextContent band_name);

		/**
		 * Sets (or unsets) the raster feature.
		 *
		 * Emits signals 'modified_band_name' (if band name changed (due to current band name not
		 * existing in new feature's raster band names) and 'modified' if any change is detected.
		 */
		void
		set_raster_feature(
				boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref);


		//! Returns the name of the band of the raster selected for processing.
		const GPlatesPropertyValues::TextContent &
		get_band_name() const
		{
			return d_band_name;
		}

		//! Returns the list of band names that are in the raster feature.
		const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &
		get_band_names() const
		{
			return d_band_names;
		}

		/**
		 * Returns the raster statistics of the band of the raster selected for processing.
		 *
		 * NOTE: For time-dependent rasters these are the statistics of the raster at present day.
		 */
		GPlatesPropertyValues::RasterStatistics
		get_band_statistic() const
		{
			return d_band_statistic;
		}

		/**
		 * Returns the list of raster statistics for the raster bands.
		 *
		 * NOTE: For time-dependent rasters these are the statistics of the raster at present day.
		 */
		const std::vector<GPlatesPropertyValues::RasterStatistics> &
		get_band_statistics() const
		{
			return d_band_statistics;
		}

		//! Returns the georeferencing of the raster feature.
		const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
		get_georeferencing() const
		{
			return d_georeferencing;
		}

		//! Returns the raster feature's spatial reference system.
		const boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> &
		get_spatial_reference_system() const
		{
			return d_spatial_reference_system;
		}

		//! Returns the raster's type.
		GPlatesPropertyValues::RasterType::Type
		get_raster_type() const
		{
			return d_raster_type;
		}

		//! Returns the raster feature or boost::none if one is currently not set on the layer.
		const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &
		get_raster_feature() const
		{
			return d_raster_feature;
		}


		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerParamsVisitor &visitor) const
		{
			visitor.visit_raster_layer_params(*this);
		}

		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				LayerParamsVisitor &visitor)
		{
			visitor.visit_raster_layer_params(*this);
		}

	Q_SIGNALS:

		/**
		 * Emitted when @a set_band_name has been called (if a change detected).
		 */
		void
		modified_band_name(
				GPlatesAppLogic::RasterLayerParams &layer_params);

	private:

		//! The raster feature.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_raster_feature;


		//! The name of the band of the raster that has been selected for processing.
		GPlatesPropertyValues::TextContent d_band_name;

		//! The list of band names that were in the raster feature the last time we examined it.
		GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type d_band_names;

		//! The raster statistics of the band of the raster selected for processing.
		GPlatesPropertyValues::RasterStatistics d_band_statistic;

		//! The list of raster statistics for the raster bands.
		std::vector<GPlatesPropertyValues::RasterStatistics> d_band_statistics;

		//! The georeferencing of the raster.
		boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> d_georeferencing;

		//! The raster's spatial reference system.
		boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> d_spatial_reference_system;

		//! The raster's type.
		GPlatesPropertyValues::RasterType::Type d_raster_type;


		RasterLayerParams();
	};
}

#endif // GPLATES_APP_LOGIC_RASTERLAYERPARAMS_H
