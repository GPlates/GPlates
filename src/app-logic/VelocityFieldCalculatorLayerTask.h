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

			//! How to calculate velocities.
			enum SolveVelocitiesMethodType
			{
				SOLVE_VELOCITIES_ON_SURFACES, // Uses plate id from intersecting polygon/network surface.
				SOLVE_VELOCITIES_BY_PLATE_ID, // Uses plate id of velocity domain feature.

				NUM_SOLVE_VELOCITY_METHODS    // This must be last.
			};

			//! Gets the velocity calculation method.
			SolveVelocitiesMethodType
			get_solve_velocities_method() const;

			//! Sets the velocity calculation method.
			void
			set_solve_velocities_method(
					SolveVelocitiesMethodType solve_velocities_method);

		private:

			SolveVelocitiesMethodType d_solve_velocities_method;

			/**
			 * Is true if @a set_solve_velocities_method has been called - VelocityFieldCalculatorLayerTask will reset this explicitly.
			 *
			 * Used to let VelocityFieldCalculatorLayerTask know that an external client has modified this state.
			 */
			bool d_set_solve_velocities_method_called;

			Params();

			// Make friend so can access constructor and @a d_set_solve_velocities_method_called.
			friend class VelocityFieldCalculatorLayerTask;
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
		//! This is a human-readable name for the velocity domain features input channel.
		static const QString VELOCITY_DOMAIN_FEATURES_CHANNEL_NAME;

		//! This is a human-readable name for the reconstructed static/dynamic polygons/networks input channel.
		static const QString VELOCITY_SURFACE_LAYERS_CHANNEL_NAME;


		/**
		 * Parameters used when calculating velocities.
		 */
		Params d_layer_task_params;

		/**
		 * Keep track of the default reconstruction layer proxy.
		 */
		ReconstructionLayerProxy::non_null_ptr_type d_default_reconstruction_layer_proxy;

		//! Are we using the default reconstruction layer proxy.
		bool d_using_default_reconstruction_layer_proxy;

		/**
		 * Does all the velocity calculations.
		 */
		velocity_field_calculator_layer_proxy_non_null_ptr_type d_velocity_field_calculator_layer_proxy;


		//! Constructor.
		VelocityFieldCalculatorLayerTask();
	};
}


#endif // GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERTASK_H
