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
 
#ifndef GPLATES_APP_LOGIC_RESOLVEDRASTER_H
#define GPLATES_APP_LOGIC_RESOLVEDRASTER_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructionGeometry.h"

#include "AppLogicFwd.h"

#include "model/FeatureHandle.h"
#include "model/types.h"
#include "model/WeakObserver.h"


namespace GPlatesAppLogic
{
	/**
	 * A type of @a ReconstructionGeometry representing a raster.
	 *
	 * Used to represent a constant or time-dependent (possibly reconstructed) raster.
	 * This currently just references the raster layer proxy and the optional age grid and
	 * reconstructed polygon layer proxies (if raster is reconstructed) - clients will be required
	 * to use those layer proxy interfaces.
	 */
	class ResolvedRaster :
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ResolvedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a non-const @a ResolvedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedRaster> non_null_ptr_to_const_type;

		//! A convenience typedef for the WeakObserver base class of this class.
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;


		/**
		 * Create a @a ResolvedRaster.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesModel::FeatureHandle &feature_handle,
				const double &reconstruction_time,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const raster_layer_proxy_non_null_ptr_type &raster_layer_proxy,
				const boost::optional<reconstruct_layer_proxy_non_null_ptr_type> &reconstructed_polygons_layer_proxy,
				const boost::optional<raster_layer_proxy_non_null_ptr_type> &age_grid_raster_layer_proxy)
		{
			return non_null_ptr_type(
					new ResolvedRaster(
							feature_handle,
							reconstruction_time,
							reconstruction_tree_,
							raster_layer_proxy,
							reconstructed_polygons_layer_proxy,
							age_grid_raster_layer_proxy));
		}


		/**
		 * Returns the reconstruction time at which raster is resolved/reconstructed.
		 */
		const double &
		get_reconstruction_time() const
		{
			return d_reconstruction_time;
		}


		/**
		 * Returns the raster layer proxy.
		 */
		const raster_layer_proxy_non_null_ptr_type &
		get_raster_layer_proxy() const
		{
			return d_raster_layer_proxy;
		}


		/**
		 * Returns the optional reconstructed polygons layer proxy.
		 */
		const boost::optional<reconstruct_layer_proxy_non_null_ptr_type> &
		get_reconstructed_polygons_layer_proxy() const
		{
			return d_reconstructed_polygons_layer_proxy;
		}


		/**
		 * Returns the optional age grid layer proxy.
		 */
		const boost::optional<raster_layer_proxy_non_null_ptr_type> &
		get_age_grid_layer_proxy() const
		{
			return d_age_grid_raster_layer_proxy;
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
		ResolvedRaster(
				GPlatesModel::FeatureHandle &feature_handle,
				const double &reconstruction_time,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const raster_layer_proxy_non_null_ptr_type &raster_layer_proxy,
				const boost::optional<reconstruct_layer_proxy_non_null_ptr_type> &reconstructed_polygons_layer_proxy,
				const boost::optional<raster_layer_proxy_non_null_ptr_type> &age_grid_raster_layer_proxy);

	private:
		/**
		 * The reconstruction time at which raster is resolved/reconstructed.
		 */
		double d_reconstruction_time;

		/**
		 * The raster layer proxy.
		 */
		raster_layer_proxy_non_null_ptr_type d_raster_layer_proxy;

		/**
		 * The optional reconstructed polygons layer proxy.
		 */
		boost::optional<reconstruct_layer_proxy_non_null_ptr_type> d_reconstructed_polygons_layer_proxy;

		/**
		 * The optional age grid layer proxy.
		 */
		boost::optional<raster_layer_proxy_non_null_ptr_type> d_age_grid_raster_layer_proxy;
	};
}

#endif // GPLATES_APP_LOGIC_RESOLVEDRASTER_H
