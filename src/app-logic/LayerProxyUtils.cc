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

#include "LayerProxyUtils.h"

#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "ReconstructLayerProxy.h"


void
GPlatesAppLogic::LayerProxyUtils::get_reconstructed_feature_geometries(
		std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries,
		const Reconstruction &reconstruction)
{
	// Get the reconstruct layer outputs.
	std::vector<GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type> reconstruct_layer_proxies;
	reconstruction.get_active_layer_outputs<GPlatesAppLogic::ReconstructLayerProxy>(reconstruct_layer_proxies);

	BOOST_FOREACH(
			const GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type &reconstruct_layer_proxy,
			reconstruct_layer_proxies)
	{
		// Get the reconstructed feature geometries from the current layer for the
		// current reconstruction time and anchor plate id.
		reconstruct_layer_proxy->get_reconstructed_feature_geometries(reconstructed_feature_geometries);
	}
}
