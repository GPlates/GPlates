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
#include <QString>
#include <QObject>

#include "CoRegistrationLayerProxy.h"
#include "LayerTask.h"
#include "LayerTaskParams.h"
#include "ReconstructionLayerProxy.h"

#include "data-mining/CoRegConfigurationTable.h"
#include "data-mining/DataTable.h"

#include "model/FeatureCollectionHandle.h"

namespace GPlatesAppLogic
{
	/**
	 * A layer task that co-registers reconstructed seed geometries with reconstructed target features.
	 */
	class CoRegistrationLayerTask :
			public LayerTask
	{
	public:
		/**
		 * App-logic parameters for a co-registration layer.
		 */
		class Params :
				public LayerTaskParams
		{
		public:
			void
			set_cfg_table(
					const GPlatesDataMining::CoRegConfigurationTable& table)
			{
				// Copy the updated configuration table.
				d_cfg_table = table;

				// Let the owning @a CoRegistrationLayerTask object know the configuration has changed.
				d_set_cfg_table_called = true;

				// FIXME: Should probably call 'emit_modified()'.
				// Currently this seems to update properly because
				// 'CoRegistrationLayerConfigurationDialog::update_cfg_table()' and
				// 'CoRegistrationLayerConfigurationDialog::update()', which are the two places
				// that call this method 'Params::set_cfg_table(), also explicitly do a reconstruction
				// which ensures an update after this modification is made.
				// TODO: Remove those calls to 'ApplicationState::reconstruct()' and call
				// 'emit_modified()' here instead. However, 'CoRegistrationLayerConfigurationDialog::update_cfg_table()'
				// does a reconstruction even if the cfg table has not changed, so that needs to be handled with care.
			}

		private:
			GPlatesDataMining::CoRegConfigurationTable d_cfg_table;

			/**
			 * Is true @a set_cfg_table has been called.
			 *
			 * Used to let CoRegistrationLayerTask know that an external client has modified this state.
			 *
			 * CoRegistrationLayerTask will reset this explicitly.
			 */
			bool d_set_cfg_table_called;

			Params() :
				d_set_cfg_table_called(false)
			{  }

			friend class CoRegistrationLayerTask;
		};


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
		QString
		get_main_input_feature_collection_channel() const;


		virtual
		void
		activate(
				bool active)
		{  }


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
				const Reconstruction::non_null_ptr_type &reconstruction);


		virtual
		LayerProxy::non_null_ptr_type
		get_layer_proxy()
		{
			return d_coregistration_layer_proxy;
		}


		virtual
		LayerTaskParams &
		get_layer_task_params()
		{
			return d_layer_task_params;
		}

		//! Channel name of the CoRegistration seed geometries.
		static const QString CO_REGISTRATION_SEED_GEOMETRIES_CHANNEL_NAME;

		//! Channel name of the CoRegistration target geometries.
		static const QString CO_REGISTRATION_TARGET_GEOMETRIES_CHANNEL_NAME;

	private:
		Params d_layer_task_params;

		/**
		 * Keep track of the default reconstruction layer proxy.
		 */
		ReconstructionLayerProxy::non_null_ptr_type d_default_reconstruction_layer_proxy;

		//! Are we using the default reconstruction layer proxy.
		bool d_using_default_reconstruction_layer_proxy;

		/**
		 * Does the co-registration.
		 */
		CoRegistrationLayerProxy::non_null_ptr_type d_coregistration_layer_proxy;


		//! Constructor.
		CoRegistrationLayerTask() :
				d_default_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
				d_using_default_reconstruction_layer_proxy(true),
				d_coregistration_layer_proxy(CoRegistrationLayerProxy::create())
		{  }
	};
}


#endif // GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERTASK_H
