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
 
#ifndef GPLATES_APP_LOGIC_AGEGRIDRASTER_H
#define GPLATES_APP_LOGIC_AGEGRIDRASTER_H

#include <boost/optional.hpp>

#include "ReconstructionGeometry.h"

#include "Layer.h"

#include "model/FeatureHandle.h"
#include "model/types.h"
#include "model/WeakObserver.h"

#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRaster.h"


namespace GPlatesAppLogic
{
	/**
	 * A type of @a ReconstructionGeometry representing an age grid raster.
	 */
	class AgeGridRaster :
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		/**
		 * A convenience typedef for a shared pointer to a non-const @a AgeGridRaster.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<AgeGridRaster> non_null_ptr_type;

		/**
		 * A convenience typedef for a shared pointer to a non-const @a AgeGridRaster.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const AgeGridRaster> non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for the WeakObserver base class of this class.
		 */
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;


		/**
		 * Create a @a AgeGridRaster.
		 *
		 * @a created_from_layer is the layer this age grid raster was created in.
		 * This is currently used so we can keep track of which persistent OpenGL objects
		 * were created for which layer so that we can destroy them when the layer is destroyed.
		 * FIXME: This is temporary until we implement a better way to handle persistent
		 * objects downstream from the reconstruction process.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesModel::FeatureHandle &feature_handle,
				const Layer &created_from_layer_,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &proxied_rasters,
				const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &raster_band_names)
		{
			return non_null_ptr_type(
					new AgeGridRaster(
							feature_handle,
							created_from_layer_,
							reconstruction_tree_,
							georeferencing,
							proxied_rasters,
							raster_band_names),
					GPlatesUtils::NullIntrusivePointerHandler());
		}


		/**
		 * Returns the layer that this resolved raster was created in.
		 */
		const Layer &
		get_layer() const
		{
			return d_created_from_layer;
		}


		/**
		 * Returns the georeferencing parameters to position the raster onto the globe.
		 */
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
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor);

	protected:
		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		AgeGridRaster(
				GPlatesModel::FeatureHandle &feature_handle,
				const Layer &created_from_layer_,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &proxied_rasters,
				const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &raster_band_names) :
			ReconstructionGeometry(reconstruction_tree_),
			WeakObserverType(feature_handle),
			d_created_from_layer(created_from_layer_),
			d_georeferencing(georeferencing),
			d_proxied_rasters(proxied_rasters),
			d_raster_band_names(raster_band_names)
		{  }

	private:
		/**
		 * The layer that this resolved raster was created in.
		 */
		Layer d_created_from_layer;

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
	};
}

#endif // GPLATES_APP_LOGIC_AGEGRIDRASTER_H
