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

#include "ResolvedRaster.h"

#include "RasterLayerProxy.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructionGeometryVisitor.h"

#include "model/WeakObserverVisitor.h"


GPlatesAppLogic::ResolvedRaster::ResolvedRaster(
		GPlatesModel::FeatureHandle &feature_handle,
		const double &reconstruction_time,
		const raster_layer_proxy_non_null_ptr_type &raster_layer_proxy,
		const boost::optional<reconstruct_layer_proxy_non_null_ptr_type> &reconstructed_polygons_layer_proxy,
		const boost::optional<raster_layer_proxy_non_null_ptr_type> &age_grid_raster_layer_proxy,
		const boost::optional<raster_layer_proxy_non_null_ptr_type> &normal_map_raster_layer_proxy) :
	ReconstructionGeometry(reconstruction_time),
	WeakObserverType(feature_handle),
	d_raster_layer_proxy(raster_layer_proxy),
	d_reconstructed_polygons_layer_proxy(reconstructed_polygons_layer_proxy),
	d_age_grid_raster_layer_proxy(age_grid_raster_layer_proxy),
	d_normal_map_raster_layer_proxy(normal_map_raster_layer_proxy)
{
}


void
GPlatesAppLogic::ResolvedRaster::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ResolvedRaster::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ResolvedRaster::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_resolved_raster(*this);
}
