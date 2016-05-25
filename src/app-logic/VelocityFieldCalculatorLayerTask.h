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
 
#ifndef GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERTASK_H
#define GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERTASK_H

#include <utility>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "AppLogicFwd.h"
#include "LayerTask.h"
#include "LayerTaskParams.h"
#include "VelocityParams.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task that calculates velocity fields on domains of mesh points inside
	 * reconstructed static polygons, resolved topological dynamic polygons or resolved
	 * topological networks.
	 */
	class VelocityFieldCalculatorLayerTask :
			public LayerTask
	{
	public:
		/**
		 * App-logic parameters for a velocity layer.
		 */
		class Params :
				public LayerTaskParams
		{
		public:

			explicit
			Params(
					VelocityFieldCalculatorLayerTask &layer_task);

			/**
			 * Returns the 'const' velocity parameters.
			 */
			const VelocityParams &
			get_velocity_params() const;

			/**
			 * Sets the velocity parameters.
			 */
			void
			set_velocity_params(
					const VelocityParams &velocity_params);

		private:

			VelocityFieldCalculatorLayerTask &d_layer_task;

			VelocityParams d_velocity_params;
		};


		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<VelocityFieldCalculatorLayerTask>
		create_layer_task();


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::VELOCITY_FIELD_CALCULATOR;
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
		LayerTaskParams &
		get_layer_task_params()
		{
			return d_layer_task_params;
		}

	private:

		/**
		 * Parameters used when calculating velocities.
		 */
		Params d_layer_task_params;

		/**
		 * Does all the velocity calculations.
		 */
		velocity_field_calculator_layer_proxy_non_null_ptr_type d_velocity_field_calculator_layer_proxy;


		//! Constructor.
		VelocityFieldCalculatorLayerTask();
	};
}


#endif // GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERTASK_H
