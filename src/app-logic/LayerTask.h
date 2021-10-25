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
 
#ifndef GPLATES_APP_LOGIC_LAYERTASK_H
#define GPLATES_APP_LOGIC_LAYERTASK_H

#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <QString>

#include "Layer.h"
#include "LayerInputChannelName.h"
#include "LayerInputChannelType.h"
#include "LayerParams.h"
#include "LayerProxy.h"
#include "LayerTaskType.h"
#include "Reconstruction.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	/**
	 * Abstract interface for processing input feature collections and/or the outputs
	 * of other layers (each layer has a layer proxy at its output).
	 */
	class LayerTask
	{
	public:
		virtual
		~LayerTask()
		{ }


		/**
		 * Returns the type of this layer task.
		 *
		 * This is useful for customising the visual representation of each type of
		 * layer task.
		 */
		virtual
		LayerTaskType::Type
		get_layer_type() const = 0;


		/**
		 * Returns the input channels expected by this task and the data types and arity
		 * for each channel.
		 */
		virtual
		std::vector<LayerInputChannelType>
		get_input_channel_types() const = 0;


		/**
		 * Returns the main input feature collection channel used by this layer task.
		 *
		 * This is the channel containing the feature collection(s) used to determine the
		 * layer tasks that are applicable to this layer.
		 *
		 * This can be used by the GUI to list available layer tasks to the user.
		 */
		virtual
		LayerInputChannelName::Type
		get_main_input_feature_collection_channel() const = 0;


		/**
		 * Activates (or deactivates) this layer tasks to reflect active state of owning layer.
		 */
		virtual
		void
		activate(
				bool active) = 0;


		/**
		 * An input file has been connected on the specified input channel.
		 */
		virtual
		void
		add_input_file_connection(
				LayerInputChannelName::Type input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection) = 0;

		/**
		 * An input file has been disconnected on the specified input channel.
		 */
		virtual
		void
		remove_input_file_connection(
				LayerInputChannelName::Type input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection) = 0;


		/**
		 * An input file has been modified.
		 *
		 * Either a feature was added or removed from the feature collection or an existing
		 * feature in the collection was modified (property value added/removed/modified).
		 */
		virtual
		void
		modified_input_file(
				LayerInputChannelName::Type input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection) = 0;


		/**
		 * The output of another layer (a layer proxy) has been connected on the specified input channel.
		 */
		virtual
		void
		add_input_layer_proxy_connection(
				LayerInputChannelName::Type input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy) = 0;

		/**
		 * The output of another layer (a layer proxy) has been disconnected on the specified input channel.
		 */
		virtual
		void
		remove_input_layer_proxy_connection(
				LayerInputChannelName::Type input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy) = 0;


		/**
		 * Update this task.
		 *
		 * This typically happens when one, or more, of the following occurs:
		 * - the reconstruction time changes, or
		 * - the anchored plate changes, or
		 * - something in the model changed, or
		 * - a layer connection somewhere was added or removed, or
		 * - layer task parameters of some layer were modified.
		 *
		 * This gives the layer task a chance to update itself and flush any cached internal data.
		 * This can happen for instance if a dependent layer changes and this layer needs to
		 * flush any cached data as a result.
		 *
		 * NOTE: @a reconstruction contains all active layer proxies including 'this' one.
		 *
		 * NOTE: Each layer proxy already knows about its layer connection changes so this method is
		 * really just to respond to changes in other layers or changes in any input feature collections.
		 */
		virtual
		void
		update(
				const Reconstruction::non_null_ptr_type &reconstruction) = 0;


		/**
		 * Returns the layer proxy that clients can use to request results from this
		 * layer - typically the layer proxy does the real processing and it sits at the output
		 * of this layer in the reconstruct graph.
		 */
		virtual
		LayerProxy::non_null_ptr_type
		get_layer_proxy() = 0;


		/**
		 * Returns the additional parameters and configuration options of this layer.
		 */
		virtual
		LayerParams::non_null_ptr_type
		get_layer_params() = 0;
	};
}


#endif // GPLATES_APP_LOGIC_LAYERTASK_H
