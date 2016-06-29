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
 
#ifndef GPLATES_APP_LOGIC_COREGISTRATIONLAYERTASK_H
#define GPLATES_APP_LOGIC_COREGISTRATIONLAYERTASK_H

#include <utility>
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <QString>

#include "CoRegistrationLayerParams.h"
#include "CoRegistrationLayerProxy.h"
#include "LayerTask.h"

#include "model/FeatureCollectionHandle.h"

namespace GPlatesAppLogic
{
	/**
	 * A layer task that co-registers reconstructed seed geometries with reconstructed target features.
	 */
	class CoRegistrationLayerTask :
			public QObject,
			public LayerTask
	{
		Q_OBJECT

	public:

		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<CoRegistrationLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<CoRegistrationLayerTask>(new CoRegistrationLayerTask());
		}


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::CO_REGISTRATION;
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
			return d_coregistration_layer_proxy;
		}


		virtual
		LayerParams::non_null_ptr_type
		get_layer_params()
		{
			return d_layer_params;
		}

	private Q_SLOTS:

		void
		handle_cfg_table_modified(
				GPlatesAppLogic::CoRegistrationLayerParams &layer_params);

	private:
		CoRegistrationLayerParams::non_null_ptr_type d_layer_params;

		/**
		 * Does the co-registration.
		 */
		CoRegistrationLayerProxy::non_null_ptr_type d_coregistration_layer_proxy;


		//! Constructor.
		CoRegistrationLayerTask();
	};
}


#endif // GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERTASK_H
