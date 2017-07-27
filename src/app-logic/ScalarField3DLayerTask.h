/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_SCALARFIELD3DLAYERTASK_H
#define GPLATES_APP_LOGIC_SCALARFIELD3DLAYERTASK_H

#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "LayerTask.h"
#include "ScalarField3DLayerParams.h"
#include "ScalarField3DLayerProxy.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task for a 3D scalar field to be visualised using volume rendering.
	 */
	class ScalarField3DLayerTask :
			public LayerTask
	{
	public:

		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<ScalarField3DLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<ScalarField3DLayerTask>(new ScalarField3DLayerTask());
		}


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::SCALAR_FIELD_3D;
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
				bool active)
		{  }


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
		get_layer_proxy();


		virtual
		LayerParams::non_null_ptr_type
		get_layer_params()
		{
			return d_layer_params;
		}

	private:

		/**
		 * Extra parameters for this layer.
		 */
		ScalarField3DLayerParams::non_null_ptr_type d_layer_params;

		ScalarField3DLayerProxy::non_null_ptr_type d_scalar_field_layer_proxy;


		//! Constructor.
		ScalarField3DLayerTask();
	};
}

#endif // GPLATES_APP_LOGIC_SCALARFIELD3DLAYERTASK_H
