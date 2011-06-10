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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONLAYER_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONLAYER_H

#include <utility>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "LayerTask.h"
#include "LayerTaskParams.h"
#include "ReconstructionLayerProxy.h"

#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task that generates a @a ReconstructionTree from feature collection(s)
	 * containing reconstruction features.
	 */
	class ReconstructionLayerTask :
			public LayerTask
	{
	public:
		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<ReconstructionLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<ReconstructionLayerTask>(new ReconstructionLayerTask());
		}


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::RECONSTRUCTION;
		}


		virtual
		std::vector<LayerInputChannelType>
		get_input_channel_types() const;


		virtual
		QString
		get_main_input_feature_collection_channel() const;


		virtual
		void
		activate(
				bool active)
		{  }


		void
		add_input_file_connection(
				const QString &input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		void
		remove_input_file_connection(
				const QString &input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		void
		modified_input_file(
				const QString &input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		void
		add_input_layer_proxy_connection(
				const QString &input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy)
		{
			// Ignore - we're only interested in input feature collections.
		}

		void
		remove_input_layer_proxy_connection(
				const QString &input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy)
		{
			// Ignore - we're only interested in input feature collections.
		}


		virtual
		void
		update(
				const Reconstruction::non_null_ptr_type &reconstruction);


		LayerProxy::non_null_ptr_type
		get_layer_proxy()
		{
			return d_reconstruction_layer_proxy;
		}


		virtual
		LayerTaskParams &
		get_layer_task_params()
		{
			return d_layer_task_params;
		}

	private:
		static const QString RECONSTRUCTION_FEATURES_CHANNEL_NAME;

		LayerTaskParams d_layer_task_params;

		/**
		 * The layer proxy at the output of the layer.
		 */
		ReconstructionLayerProxy::non_null_ptr_type d_reconstruction_layer_proxy;


		//! Constructor.
		ReconstructionLayerTask() :
				d_reconstruction_layer_proxy(ReconstructionLayerProxy::create())
		{  }
	};
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONLAYER_H
