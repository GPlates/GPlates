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

#ifndef GPLATES_APP_LOGIC_RASTERLAYERPROXY_H
#define GPLATES_APP_LOGIC_RASTERLAYERPROXY_H

#include <boost/optional.hpp>

#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "RasterLayerTask.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedRaster.h"

#include "maths/types.h"

#include "model/FeatureHandle.h"

#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRaster.h"
#include "property-values/TextContent.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer proxy for resolving, and optionally reconstructing, a raster.
	 */
	class RasterLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a RasterLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<RasterLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a RasterLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const RasterLayerProxy> non_null_ptr_to_const_type;


		/**
		 * Creates a @a RasterLayerProxy object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new RasterLayerProxy());
		}


		/**
		 * Returns the resolved raster for the current reconstruction time.
		 *
		 * Returns boost::none if there is no input raster feature connected or it cannot be resolved.
		 */
		boost::optional<ResolvedRaster::non_null_ptr_type>
		get_resolved_raster()
		{
			return get_resolved_raster(d_current_reconstruction_time);
		}

		/**
		 * Returns the resolved raster for the specified time.
		 *
		 * Returns boost::none if there is no input raster feature connected or it cannot be resolved.
		 */
		boost::optional<ResolvedRaster::non_null_ptr_type>
		get_resolved_raster(
				const double &reconstruction_time);


		/**
		 * Returns the proxied raw raster, for the current reconstruction time, of the band
		 * of the raster selected for processing.
		 */
		const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
		get_proxied_raster()
		{
			return get_proxied_raster(d_current_reconstruction_time);
		}

		/**
		 * Returns the proxied raw raster, for the specified time, of the band
		 * of the raster selected for processing.
		 */
		const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
		get_proxied_raster(
				const double &reconstruction_time);


		/**
		 * Returns the list of proxied rasters, for the current reconstruction time, for the raster bands.
		 */
		const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
		get_proxied_rasters()
		{
			return get_proxied_rasters(d_current_reconstruction_time);
		}

		/**
		 * Returns the list of proxied rasters, for the specified time, for the raster bands.
		 */
		const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
		get_proxied_rasters(
				const double &reconstruction_time);


		/**
		 * Returns the name of the band selected for this raster.
		 */
		const GPlatesPropertyValues::TextContent &
		get_raster_band_name() const
		{
			return d_current_raster_band_name;
		}

		/**
		 * Sets the names of the bands of the raster.
		 */
		const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &
		get_raster_band_names() const
		{
			return d_current_raster_band_names;
		}


		/**
		 * Returns the georeferencing of the raster feature.
		 */
		const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
		get_georeferencing() const
		{
			return d_current_georeferencing;
		}


		/**
		 * Returns the subject token that clients can use to determine if this raster layer proxy has changed.
		 *
		 * This is mainly useful for other layers that have this layer connected as their input.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token();


		/**
		 * Returns the subject token that clients can use to determine if the proxied raster
		 * has changed for the current reconstruction time.
		 *
		 * This is useful for time-dependent rasters where only the proxied raw rasters change.
		 */
		const GPlatesUtils::SubjectToken &
		get_proxied_raster_subject_token()
		{
			return get_proxied_raster_subject_token(d_current_reconstruction_time);
		}

		/**
		 * Returns the subject token that clients can use to determine if the proxied raster
		 * has changed for the specified reconstruction time.
		 *
		 * This is useful for time-dependent rasters where only the proxied raw rasters change.
		 */
		const GPlatesUtils::SubjectToken &
		get_proxied_raster_subject_token(
				const double &reconstruction_time);


		/**
		 * Returns the subject token that clients can use to determine if the raster feature has changed.
		 *
		 * This is useful for determining if only the raster feature has changed (excludes any
		 * changes to the optional reconstructed polygons and the optional age grid and any changes
		 * in the reconstruction time).
		 *
		 * This is useful if this raster layer represents an age grid raster - another raster layer
		 * can use this age grid layer to help reconstruct it - in which case only the *present-day*
		 * proxied raster of this age grid raster is accessed (ie, the proxied raster subject token,
		 * which is time-dependent, is avoided).
		 */
		const GPlatesUtils::SubjectToken &
		get_raster_feature_subject_token();


		/**
		 * Accept a ConstLayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerProxyVisitor &visitor) const
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

		/**
		 * Accept a LayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				LayerProxyVisitor &visitor)
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}


		//
		// Used by LayerTask...
		//

		/**
		 * Sets the current reconstruction time as set by the layer system.
		 */
		void
		set_current_reconstruction_time(
				const double &reconstruction_time);

		/**
		 * Adds the specified reconstructed polygons layer proxy.
		 */
		void
		set_current_reconstructed_polygons_layer_proxy(
				boost::optional<ReconstructLayerProxy::non_null_ptr_type> reconstructed_polygons_layer_proxy);

		/**
		 * Set the age grid raster layer proxy.
		 */
		void
		set_current_age_grid_raster_layer_proxy(
				boost::optional<RasterLayerProxy::non_null_ptr_type> age_grid_raster_layer_proxy);

		/**
		 * Specify the raster feature.
		 */
		void
		set_current_raster_feature(
				boost::optional<GPlatesModel::FeatureHandle::weak_ref> raster_feature,
				const RasterLayerTask::Params &raster_params);


		/**
		 * The currently selected raster band name has changed.
		 */
		void
		set_current_raster_band_name(
				const RasterLayerTask::Params &raster_params);

		/**
		 * The raster feature has been modified.
		 */
		void
		modified_raster_feature(
				const RasterLayerTask::Params &raster_params);

	private:
		/**
		 * Potentially time-varying feature properties for the currently resolved raster
		 * (ie, at the cached reconstruction time).
		 */
		struct ResolvedRasterFeatureProperties
		{
			void
			reset()
			{
				proxied_raster = boost::none;
				proxied_rasters = boost::none;
			}

			//! The proxied raw raster of the currently selected raster band.
			boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> proxied_raster;

			//! The proxied raw raster for all the raster bands.
			boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > proxied_rasters;
		};


		/**
		 * The input reconstructed polygons, if any connected to our input.
		 */
		LayerProxyUtils::OptionalInputLayerProxy<ReconstructLayerProxy> d_current_reconstructed_polygons_layer_proxy;

		/**
		 * Optional age grid raster input.
		 */
		LayerProxyUtils::OptionalInputLayerProxy<RasterLayerProxy> d_current_age_grid_raster_layer_proxy;

		/**
		 * The raster input feature.
		 */
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_current_raster_feature;

		//! The selected raster band name.
		GPlatesPropertyValues::TextContent d_current_raster_band_name;

		//! The raster band names.
		GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type d_current_raster_band_names;

		//! The georeferencing of the raster.
		boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> d_current_georeferencing;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		/**
		 * Cached reconstruction time.
		 */
		boost::optional<GPlatesMaths::real_t> d_cached_reconstruction_time;

		//! Time-varying (potentially) raster feature properties.
		ResolvedRasterFeatureProperties d_cached_resolved_raster_feature_properties;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * The subject token that clients can use to determine if the proxied raster has changed.
		 */
		mutable GPlatesUtils::SubjectToken d_proxied_raster_subject_token;

		/**
		 * The subject token that clients can use to determine if the raster feature has changed.
		 */
		mutable GPlatesUtils::SubjectToken d_raster_feature_subject_token;


		RasterLayerProxy() :
			d_current_raster_band_name(GPlatesUtils::UnicodeString()),
			d_current_reconstruction_time(0)
		{  }


		void
		invalidate_raster_feature();

		void
		invalidate_proxied_raster();

		void
		invalidate();


		/**
		 * Attempts to resolve a raster.
		 *
		 * Can fail if not enough information is available to resolve the raster.
		 * Such as no raster feature or raster feature does not have the required property values.
		 * In this case the returned value will be false.
		 */
		bool
		resolve_raster_feature(
				const double &reconstruction_time);


		//! Sets some raster parameters.
		void
		set_raster_params(
				const RasterLayerTask::Params &raster_params);
	};
}

#endif // GPLATES_APP_LOGIC_RASTERLAYERPROXY_H
