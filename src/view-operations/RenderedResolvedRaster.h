/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_VIEWOPERATIONS_RENDEREDRESOLVEDRASTER_H
#define GPLATES_VIEWOPERATIONS_RENDEREDRESOLVEDRASTER_H

#include <vector>
#include <boost/optional.hpp>

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "app-logic/Layer.h"
#include "app-logic/ReconstructRasterPolygons.h"

#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRaster.h"


namespace GPlatesViewOperations
{
	class RenderedResolvedRaster :
			public RenderedGeometryImpl
	{
	public:
		explicit
		RenderedResolvedRaster(
				const GPlatesAppLogic::Layer &layer,
				const double &reconstruction_time,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &proxied_rasters,
				const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &raster_band_names,
				const boost::optional<GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type> &
						reconstruct_raster_polygons = boost::none,
				const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
						age_grid_georeferencing = boost::none,
				const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
						age_grid_proxied_rasters = boost::none,
				const boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> &
						age_grid_raster_band_names = boost::none) :
		d_created_from_layer(layer),
		d_reconstruction_time(reconstruction_time),
		d_georeferencing(georeferencing),
		d_proxied_rasters(proxied_rasters),
		d_raster_band_names(raster_band_names),
		d_reconstruct_raster_polygons(reconstruct_raster_polygons),
		d_age_grid_georeferencing(age_grid_georeferencing),
		d_age_grid_proxied_rasters(age_grid_proxied_rasters),
		d_age_grid_raster_band_names(age_grid_raster_band_names)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_resolved_raster(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			// TODO: Implement query into georeferenced raster bounds.
			return GPlatesMaths::ProximityHitDetail::null;
		}

		/**
		 * Returns the layer that this resolved raster was created in.
		 */
		const GPlatesAppLogic::Layer &
		get_layer() const
		{
			return d_created_from_layer;
		}

		/**
		 * Returns the reconstruction time at which raster is resolved/reconstructed.
		 */
		const double &
		get_reconstruction_time() const
		{
			return d_reconstruction_time;
		}

		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &
		get_georeferencing() const
		{
			return d_georeferencing;
		}

		const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
		get_proxied_rasters() const
		{
			return d_proxied_rasters;
		}

		const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &
		get_raster_band_names() const
		{
			return d_raster_band_names;
		}

		/**
		 * Returns the optional polygon set used to reconstruct the raster.
		 */
		const boost::optional<GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type> &
		get_reconstruct_raster_polygons() const
		{
			return d_reconstruct_raster_polygons;
		}

		/**
		 * Returns the georeferencing for the age grid raster.
		 */
		const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
		get_age_grid_georeferencing() const
		{
			return d_age_grid_georeferencing;
		}

		const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
		get_age_grid_proxied_rasters() const
		{
			return d_age_grid_proxied_rasters;
		}

		const boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> &
		get_age_grid_raster_band_names() const
		{
			return d_age_grid_raster_band_names;
		}

	private:
		/**
		 * The layer that this resolved raster was created in.
		 */
		GPlatesAppLogic::Layer d_created_from_layer;

		/**
		 * The reconstruction time at which raster is resolved/reconstructed.
		 */
		double d_reconstruction_time;

		/**
		 * The georeferencing parameters to position the raster onto the globe.
		 */
		GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type d_georeferencing;

		/**
		 * The proxied rasters of the time-resolved GmlFile (in the case of time-dependent rasters).
		 *
		 * The band name will be used to look up the correct raster in the presentation code.
		 * The user-selected band name is not accessible here since this is app-logic code.
		 */
		std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> d_proxied_rasters;

		/**
		 * The list of band names - one for each proxied raster.
		 */
		GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type d_raster_band_names;

		/**
		 * The optional polygon set used to reconstruct the raster.
		 */
		boost::optional<GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type>
				d_reconstruct_raster_polygons;

		/**
		 * Georeferencing for the age grid raster.
		 */
		boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type>
				d_age_grid_georeferencing;

		boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> >
				d_age_grid_proxied_rasters;
		boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type>
				d_age_grid_raster_band_names;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDRESOLVEDRASTER_H
