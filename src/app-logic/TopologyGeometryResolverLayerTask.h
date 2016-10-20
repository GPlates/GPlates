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
 
#ifndef GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYRESOLVERLAYERTASK_H
#define GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYRESOLVERLAYERTASK_H

#include <utility>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "LayerParams.h"
#include "LayerTask.h"
#include "ReconstructLayerProxy.h"
#include "TopologyGeometryResolverLayerProxy.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task that resolves topological geometries (boundaries and lines) from
	 * feature collection(s) containing topological geometries.
	 */
	class TopologyGeometryResolverLayerTask :
			public LayerTask
	{
	public:
		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<TopologyGeometryResolverLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<TopologyGeometryResolverLayerTask>(
					new TopologyGeometryResolverLayerTask());
		}


		~TopologyGeometryResolverLayerTask();


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER;
		}


		virtual
		std::vector<LayerInputChannelType>
		get_input_channel_types() const;


		virtual
		LayerInputChannelName::Type
		get_main_input_feature_collection_channel() const;


		virtual
		void
		activate(
				bool active);


		virtual
		void
		add_input_file_connection(
				LayerInputChannelName::Type input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		virtual
		void
		remove_input_file_connection(
				LayerInputChannelName::Type input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		virtual
		void
		modified_input_file(
				LayerInputChannelName::Type input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		virtual
		void
		add_input_layer_proxy_connection(
				LayerInputChannelName::Type input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy);

		virtual
		void
		remove_input_layer_proxy_connection(
				LayerInputChannelName::Type input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy);


		virtual
		void
		update(
				const Reconstruction::non_null_ptr_type &reconstruction);


		virtual
		LayerProxy::non_null_ptr_type
		get_layer_proxy()
		{
			return d_topology_geometry_resolver_layer_proxy;
		}


		virtual
		LayerParams::non_null_ptr_type
		get_layer_params()
		{
			return d_layer_params;
		}

	private:

		LayerParams::non_null_ptr_type d_layer_params;

		/**
		 * Keep track of the default reconstruction layer proxy.
		 */
		ReconstructionLayerProxy::non_null_ptr_type d_default_reconstruction_layer_proxy;

		//! Are we using the default reconstruction layer proxy.
		bool d_using_default_reconstruction_layer_proxy;

		//! Any currently connected 'reconstructed geometry' topological section layers.
		std::vector<ReconstructLayerProxy::non_null_ptr_type>
				d_current_reconstructed_geometry_topological_sections_layer_proxies;
		//! Any currently connected 'resolved line' topological section layers.
		std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type>
				d_current_resolved_line_topological_sections_layer_proxies;

		/**
		 * Does all the resolving.
		 */
		TopologyGeometryResolverLayerProxy::non_null_ptr_type d_topology_geometry_resolver_layer_proxy;


		//! Constructor.
		TopologyGeometryResolverLayerTask() :
				d_layer_params(LayerParams::create()),
				d_default_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
				d_using_default_reconstruction_layer_proxy(true),
				d_topology_geometry_resolver_layer_proxy(TopologyGeometryResolverLayerProxy::create())
		{  }

		/**
		 * Returns true if any topological section layers are currently connected.
		 */
		bool
		connected_to_topological_section_layers() const;

		/**
		 * Returns the 'reconstructed geometry' topological section layers.
		 */
		void
		get_reconstructed_geometry_topological_sections_layer_proxies(
				std::vector<ReconstructLayerProxy::non_null_ptr_type> &
						reconstructed_geometry_topological_sections_layer_proxies,
				const Reconstruction::non_null_ptr_type &reconstruction) const;

		/**
		 * Returns the 'resolved line' topological section layers.
		 */
		void
		get_resolved_line_topological_sections_layer_proxies(
				std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &
						resolved_line_topological_sections_layer_proxies,
				const Reconstruction::non_null_ptr_type &reconstruction) const;
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYRESOLVERLAYERTASK_H
