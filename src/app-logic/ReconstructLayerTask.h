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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTLAYERTASK_H
#define GPLATES_APP_LOGIC_RECONSTRUCTLAYERTASK_H

#include <utility>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <QString>

#include "LayerTask.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructLayerParams.h"
#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyNetworkResolverLayerProxy.h"

#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	class ApplicationState;

	/**
	 * A layer task that reconstructs geometries of features from feature collection(s).
	 */
	class ReconstructLayerTask :
			public QObject,
			public LayerTask
	{
		Q_OBJECT

	public:

		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
				ApplicationState &application_state);


		static
		boost::shared_ptr<ReconstructLayerTask>
		create_layer_task(
				ApplicationState &application_state);


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::RECONSTRUCT;
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
			return d_reconstruct_layer_proxy;
		}


		virtual
		LayerParams::non_null_ptr_type
		get_layer_params()
		{
			return d_layer_params;
		}

	private Q_SLOTS:

		void
		handle_reconstruct_params_modified(
				GPlatesAppLogic::ReconstructLayerParams &layer_params);

	private:

		/**
		 * Parameters used when reconstructing.
		 */
		ReconstructLayerParams::non_null_ptr_type d_layer_params;

		/**
		 * Keep track of the default reconstruction layer proxy.
		 */
		ReconstructionLayerProxy::non_null_ptr_type d_default_reconstruction_layer_proxy;

		//! Are we using the default reconstruction layer proxy.
		bool d_using_default_reconstruction_layer_proxy;

		//! Any currently connected 'resolved boundary' topology surface layers.
		std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type>
				d_current_resolved_boundary_topology_surface_layer_proxies;
		//! Any currently connected 'resolved network' topology surface layers.
		std::vector<TopologyNetworkResolverLayerProxy::non_null_ptr_type>
				d_current_resolved_network_topology_surface_layer_proxies;


		/**
		 * Does all the reconstructing.
		 *
		 * NOTE: Should be declared after @a d_layer_params.
		 */
		ReconstructLayerProxy::non_null_ptr_type d_reconstruct_layer_proxy;


		//! Constructor.
		ReconstructLayerTask(
				const ReconstructMethodRegistry &reconstruct_method_registry);

		/**
		 * Returns true if any topology surface layers are currently connected.
		 */
		bool
		connected_to_topology_surface_layers() const;

		/**
		 * Returns the 'resolved boundary' topology surface layers.
		 */
		void
		get_resolved_boundary_topology_surface_layer_proxies(
				std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &
						resolved_boundary_topology_surface_layer_proxies,
				const Reconstruction::non_null_ptr_type &reconstruction) const;

		/**
		 * Returns the 'resolved network' topology surface layers.
		 */
		void
		get_resolved_network_topology_surface_layer_proxies(
				std::vector<TopologyNetworkResolverLayerProxy::non_null_ptr_type> &
						resolved_network_topology_surface_layer_proxies,
				const Reconstruction::non_null_ptr_type &reconstruction) const;
	};
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTLAYERTASK_H
