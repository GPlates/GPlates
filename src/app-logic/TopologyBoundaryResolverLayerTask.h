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
 
#ifndef GPLATES_APP_LOGIC_TOPOLOGYBOUNDARYRESOLVERLAYERTASK_H
#define GPLATES_APP_LOGIC_TOPOLOGYBOUNDARYRESOLVERLAYERTASK_H

#include <utility>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "LayerTask.h"
#include "LayerTaskParams.h"
#include "TopologyBoundaryResolverLayerProxy.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task that resolves topological boundaries from feature collection(s)
	 * containing topological closed plate boundaries.
	 */
	class TopologyBoundaryResolverLayerTask :
			public LayerTask
	{
	public:
		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<TopologyBoundaryResolverLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<TopologyBoundaryResolverLayerTask>(
					new TopologyBoundaryResolverLayerTask());
		}


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::TOPOLOGY_BOUNDARY_RESOLVER;
		}


		virtual
		std::vector<LayerInputChannelType>
		get_input_channel_types() const;


		virtual
		QString
		get_main_input_feature_collection_channel() const;


		virtual
		void
		add_input_file_connection(
				const QString &input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		virtual
		void
		remove_input_file_connection(
				const QString &input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		virtual
		void
		modified_input_file(
				const QString &input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		virtual
		void
		add_input_layer_proxy_connection(
				const QString &input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy);

		virtual
		void
		remove_input_layer_proxy_connection(
				const QString &input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy);


		virtual
		void
		update(
				const Layer &layer_handle /* the layer invoking this */,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchored_plate_id,
				const ReconstructionLayerProxy::non_null_ptr_type &default_reconstruction_layer_proxy);


		virtual
		LayerProxy::non_null_ptr_type
		get_layer_proxy()
		{
			return d_topology_boundary_resolver_layer_proxy;
		}


		virtual
		LayerTaskParams &
		get_layer_task_params()
		{
			return d_layer_task_params;
		}

	private:
		static const QString TOPOLOGICAL_CLOSED_PLATES_POLYGON_FEATURES_CHANNEL_NAME;
		static const QString TOPOLOGICAL_BOUNDARY_SECTION_FEATURES_CHANNEL_NAME;

		LayerTaskParams d_layer_task_params;

		/**
		 * Keep track of the default reconstruction layer proxy.
		 */
		ReconstructionLayerProxy::non_null_ptr_type d_default_reconstruction_layer_proxy;

		//! Are we using the default reconstruction layer proxy.
		bool d_using_default_reconstruction_layer_proxy;

		/**
		 * Does all the resolving.
		 */
		TopologyBoundaryResolverLayerProxy::non_null_ptr_type d_topology_boundary_resolver_layer_proxy;


		//! Constructor.
		TopologyBoundaryResolverLayerTask() :
				d_default_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
				d_using_default_reconstruction_layer_proxy(true),
				d_topology_boundary_resolver_layer_proxy(TopologyBoundaryResolverLayerProxy::create())
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYBOUNDARYRESOLVERLAYERTASK_H
