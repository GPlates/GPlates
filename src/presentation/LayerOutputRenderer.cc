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

#include <algorithm>
#include <boost/foreach.hpp>

#include "LayerOutputRenderer.h"

#include "app-logic/AppLogicFwd.h"
#include "app-logic/MultiPointVectorField.h"
#include "app-logic/RasterLayerProxy.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionLayerProxy.h"
#include "app-logic/ReconstructLayerProxy.h"
#include "app-logic/ReconstructMethodFiniteRotation.h"
#include "app-logic/ResolvedTopologicalGeometry.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ScalarField3DLayerProxy.h"
#include "app-logic/TopologyGeometryResolverLayerProxy.h"
#include "app-logic/TopologyNetworkResolverLayerProxy.h"
#include "app-logic/VelocityFieldCalculatorLayerProxy.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "presentation/ReconstructionGeometryRenderer.h"

#include "utils/Profile.h"


namespace GPlatesPresentation
{
	namespace
	{
		//! Convenience typedef.
		typedef GPlatesAppLogic::ReconstructLayerProxy::reconstructed_feature_geometries_spatial_partition_type
				reconstructed_feature_geometries_spatial_partition_type;

		/**
		 * Information associating a ReconstructedFeatureGeometry with its location in a spatial partition.
		 */
		struct ReconstructedFeatureGeometrySpatialPartitionInfo
		{
			ReconstructedFeatureGeometrySpatialPartitionInfo(
					const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type &rfg_,
					const reconstructed_feature_geometries_spatial_partition_type::location_type &rfg_spatial_partition_location_) :
				rfg(rfg_),
				rfg_spatial_partition_location(rfg_spatial_partition_location_)
			{  }

			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type rfg;
			reconstructed_feature_geometries_spatial_partition_type::location_type rfg_spatial_partition_location;
		};

		/**
		 * Helper structure to sort rendered geometries in their render order.
		 */
		struct ReconstructedFeatureGeometryRenderOrder
		{
			ReconstructedFeatureGeometryRenderOrder(
					unsigned int rfg_spatial_partition_info_index_,
					const boost::optional<const GPlatesAppLogic::ReconstructMethodFiniteRotation &> &rfg_transform_) :
				rfg_spatial_partition_info_index(rfg_spatial_partition_info_index_),
				rfg_transform(rfg_transform_)
			{  }

			unsigned int rfg_spatial_partition_info_index;
			// Note that the boost::optional contains a reference since we don't want to sort by pointer.
			boost::optional<const GPlatesAppLogic::ReconstructMethodFiniteRotation &> rfg_transform;;

			//! Used to sort by transform.
			struct SortTransform
			{
				bool
				operator()(
						const ReconstructedFeatureGeometryRenderOrder &lhs,
						const ReconstructedFeatureGeometryRenderOrder &rhs) const
				{
					return lhs.rfg_transform < rhs.rfg_transform;
				}
			};
		};

		/**
		 * Sort the RFGs by transform (essentially by plate id) and render them in that order.
		 *
		 * This ensures a consistent stable ordering when, for example, polygons start to overlap.
		 */
		void
		render_in_transform_sorted_order(
				ReconstructionGeometryRenderer &reconstruction_geometry_renderer,
				const reconstructed_feature_geometries_spatial_partition_type &rfg_spatial_partition)
		{
			// Get RFGs and associated information.
			// The information is separated into two vectors in order to minimise copying during sorting.
			std::vector<ReconstructedFeatureGeometrySpatialPartitionInfo> rfg_spatial_partition_infos;
			rfg_spatial_partition_infos.reserve(rfg_spatial_partition.size());
			std::vector<ReconstructedFeatureGeometryRenderOrder> rfg_render_orders;
			rfg_render_orders.reserve(rfg_spatial_partition.size());

			// Visit the spatial partition to collect the RFGs and their locations in the spatial partition.
			reconstructed_feature_geometries_spatial_partition_type::const_iterator rfg_iter =
					rfg_spatial_partition.get_iterator();
			for ( ; !rfg_iter.finished(); rfg_iter.next())
			{
				// The current reconstructed feature geometry.
				const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type rfg =
						rfg_iter.get_element();

				// Get the RFG's transform if it has one.
				//
				// If it doesn't have one then it means the RFG was created without specifying a
				// transform or it does not make sense to have a rigid transform for the RFG
				// (eg, a deformed RFG or a flowline). These types will get lumped into the same
				// transform bin (the no-transform bin) and will not be ordered relative to each other.
				boost::optional<const GPlatesAppLogic::ReconstructMethodFiniteRotation &> rfg_transform;
				if (rfg->finite_rotation_reconstruction())
				{
					rfg_transform = *rfg->finite_rotation_reconstruction()->get_reconstruct_method_finite_rotation();
				}

				// Associate RFG with its spatial partition location and transform.
				//
				// NOTE: It is important to specify the spatial partition location otherwise we
				// essentially lose our nice partitioning and the benefits it affords such as
				// hierarchical view-frustum culling and efficient rendering of filled polygons in
				// the globe view.
				rfg_render_orders.push_back(
						ReconstructedFeatureGeometryRenderOrder(
								rfg_spatial_partition_infos.size(),
								rfg_transform));
				rfg_spatial_partition_infos.push_back(
						ReconstructedFeatureGeometrySpatialPartitionInfo(
								rfg,
								rfg_iter.get_location()));
			}

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					rfg_spatial_partition_infos.size() == rfg_render_orders.size(),
					GPLATES_ASSERTION_SOURCE);

			// Sort the RFGs by their transforms.
			// We use a stable sort to retain the original order for those RFGs with the same transform.
			std::stable_sort(
					rfg_render_orders.begin(),
					rfg_render_orders.end(),
					ReconstructedFeatureGeometryRenderOrder::SortTransform());

			// Render the RFGs in transform order.
			const unsigned int num_rfgs = rfg_spatial_partition_infos.size();
			for (unsigned int rfg_index = 0; rfg_index < num_rfgs; ++rfg_index)
			{
				const unsigned int rfg_spatial_partition_info_index =
						rfg_render_orders[rfg_index].rfg_spatial_partition_info_index;
				const ReconstructedFeatureGeometrySpatialPartitionInfo &rfg_spatial_partition_info =
						rfg_spatial_partition_infos[rfg_spatial_partition_info_index];

				// Render the RFG and let the renderer know its location in the spatial partition.
				reconstruction_geometry_renderer.render(
						rfg_spatial_partition_info.rfg,
						rfg_spatial_partition_info.rfg_spatial_partition_location);
			}
		}
	}
}


GPlatesPresentation::LayerOutputRenderer::LayerOutputRenderer(
		ReconstructionGeometryRenderer &reconstruction_geometry_renderer,
		GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer) :
	d_reconstruction_geometry_renderer(reconstruction_geometry_renderer),
	d_rendered_geometry_layer(rendered_geometry_layer)
{
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<co_registration_layer_proxy_type> &layer_proxy)
{
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<raster_layer_proxy_type> &raster_layer_proxy)
{
	// Get the resolved raster for the current reconstruction time.
	boost::optional<GPlatesAppLogic::ResolvedRaster::non_null_ptr_type> resolved_raster =
			raster_layer_proxy->get_resolved_raster();

	if (resolved_raster)
	{
		d_reconstruction_geometry_renderer.begin_render(d_rendered_geometry_layer);

		// Render the resolved raster.
		d_reconstruction_geometry_renderer.render(resolved_raster.get());

		d_reconstruction_geometry_renderer.end_render();
	}
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstruct_layer_proxy_type> &reconstruct_layer_proxy)
{
	// Get the reconstructed feature geometries in a spatial partition for the current reconstruction time.
	GPlatesAppLogic::ReconstructLayerProxy::reconstructed_feature_geometries_spatial_partition_type::non_null_ptr_to_const_type
			rfg_spatial_partition =
					reconstruct_layer_proxy->get_reconstructed_feature_geometries_spatial_partition();

	d_reconstruction_geometry_renderer.begin_render(d_rendered_geometry_layer);

	// Sort the RFGs by transform (essentially by plate id) and render them in that order.
	// This ensures a consistent stable ordering when, for example, polygons start to overlap.
	//
	// If different sort order choices are implemented (for example to be selected by the user)
	// then this can be the place where that sorting happens.
	render_in_transform_sorted_order(d_reconstruction_geometry_renderer, *rfg_spatial_partition);

	d_reconstruction_geometry_renderer.end_render();
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstruction_layer_proxy_type> &reconstruction_layer_proxy)
{
	// Nothing to visualise for this layer type.
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<scalar_field_3d_layer_proxy_type> &scalar_field_layer_proxy)
{
	// Get the resolved scalar field for the current reconstruction time.
	boost::optional<GPlatesAppLogic::ResolvedScalarField3D::non_null_ptr_type> resolved_scalar_field =
			scalar_field_layer_proxy->get_resolved_scalar_field_3d();

	if (resolved_scalar_field)
	{
		d_reconstruction_geometry_renderer.begin_render(d_rendered_geometry_layer);

		// Render the resolved scalar field.
		d_reconstruction_geometry_renderer.render(resolved_scalar_field.get());

		d_reconstruction_geometry_renderer.end_render();
	}
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<topology_geometry_resolver_layer_proxy_type> &topology_geometry_resolver_layer_proxy)
{
	// Get the resolved topological geometries for the current reconstruction time.
	std::vector<GPlatesAppLogic::resolved_topological_geometry_non_null_ptr_type> resolved_topological_geometries;
	topology_geometry_resolver_layer_proxy->get_resolved_topological_geometries(resolved_topological_geometries);

	d_reconstruction_geometry_renderer.begin_render(d_rendered_geometry_layer);

	// Render each resolved topological geometry.
	BOOST_FOREACH(
			const GPlatesAppLogic::resolved_topological_geometry_non_null_ptr_type &resolved_topological_geometry,
			resolved_topological_geometries)
	{
		d_reconstruction_geometry_renderer.render(resolved_topological_geometry);
	}

	d_reconstruction_geometry_renderer.end_render();
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<topology_network_resolver_layer_proxy_type> &topology_network_resolver_layer_proxy)
{
	// Get the resolved topological networks for the current reconstruction time.
	std::vector<GPlatesAppLogic::resolved_topological_network_non_null_ptr_type> resolved_topological_networks;

	topology_network_resolver_layer_proxy->get_resolved_topological_networks(resolved_topological_networks);

	d_reconstruction_geometry_renderer.begin_render(d_rendered_geometry_layer);

	// Render each resolved topological network.
	BOOST_FOREACH(
			const GPlatesAppLogic::resolved_topological_network_non_null_ptr_type &resolved_topological_network,
			resolved_topological_networks)
	{
		d_reconstruction_geometry_renderer.render(resolved_topological_network);
	}

	d_reconstruction_geometry_renderer.end_render();
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<velocity_field_calculator_layer_proxy_type> &velocity_field_calculator_layer_proxy)
{
	// Get the velocity vector fields for the current reconstruction time.
	std::vector<GPlatesAppLogic::multi_point_vector_field_non_null_ptr_type> multi_point_vector_fields;
	velocity_field_calculator_layer_proxy->get_velocity_multi_point_vector_fields(multi_point_vector_fields);

	d_reconstruction_geometry_renderer.begin_render(d_rendered_geometry_layer);

	// Render each velocity vector field.
	BOOST_FOREACH(
			const GPlatesAppLogic::multi_point_vector_field_non_null_ptr_type &multi_point_vector_field,
			multi_point_vector_fields)
	{
		d_reconstruction_geometry_renderer.render(multi_point_vector_field);
	}

	d_reconstruction_geometry_renderer.end_render();
}
