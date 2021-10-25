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
#include <boost/shared_ptr.hpp>
#include <QString>

#include "LayerTask.h"
#include "LayerTaskParams.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructParams.h"

#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	class ApplicationState;

	/**
	 * A layer task that reconstructs geometries of features from feature collection(s).
	 */
	class ReconstructLayerTask :
			public LayerTask
	{
	public:
		/**
		 * App-logic parameters for a reconstruct layer.
		 */
		class Params :
				public LayerTaskParams
		{
		public:

			/**
			 * Returns the 'const' reconstruct parameters.
			 */
			const ReconstructParams &
			get_reconstruct_params() const;

			/**
			 * Returns the reconstruct parameters for modification.
			 *
			 * NOTE: This will flush any cached reconstructed feature geometries in this layer.
			 */
			ReconstructParams &
			get_reconstruct_params();

		private:

			ReconstructParams d_reconstruct_params;

			/**
			 * Is true if the non-const version of @a get_reconstruct_params has been called.
			 *
			 * Used to let ReconstructLayerTask know that an external client has modified this state.
			 *
			 * ReconstructLayerTask will reset this explicitly.
			 */
			bool d_non_const_get_reconstruct_params_called;

			Params();

			// Make friend so can access constructor and @a d_non_const_get_reconstruct_params_called.
			friend class ReconstructLayerTask;
		};


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
		QString
		get_main_input_feature_collection_channel() const;


		virtual
		void
		activate(
				bool active);


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
			return d_reconstruct_layer_proxy;
		}


		virtual
		LayerTaskParams &
		get_layer_task_params()
		{
			return d_layer_task_params;
		}

	private:

		static const QString RECONSTRUCTABLE_FEATURES_CHANNEL_NAME;

		//! This is a human-readable name for the reconstructed static/dynamic polygons/networks input channel.
		static const QString DEFORMATION_SURFACES_CHANNEL_NAME;

		/**
		 * Parameters used when reconstructing.
		 */
		Params d_layer_task_params;

		/**
		 * Keep track of the default reconstruction layer proxy.
		 */
		ReconstructionLayerProxy::non_null_ptr_type d_default_reconstruction_layer_proxy;

		//! Are we using the default reconstruction layer proxy.
		bool d_using_default_reconstruction_layer_proxy;

		/**
		 * Does all the reconstructing.
		 *
		 * NOTE: Should be declared after @a d_layer_task_params.
		 */
		ReconstructLayerProxy::non_null_ptr_type d_reconstruct_layer_proxy;


		//! Constructor.
		ReconstructLayerTask(
				const ReconstructMethodRegistry &reconstruct_method_registry) :
				d_default_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
				d_using_default_reconstruction_layer_proxy(true),
				d_reconstruct_layer_proxy(
						ReconstructLayerProxy::create(
								reconstruct_method_registry,
								d_layer_task_params.d_reconstruct_params))
		{  }
	};
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTLAYERTASK_H
