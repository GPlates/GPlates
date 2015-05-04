/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERTASK_H
#define GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERTASK_H

#include <utility>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "AppLogicFwd.h"
#include "LayerTask.h"
#include "LayerTaskParams.h"
#include "ReconstructScalarCoverageParams.h"

#include "model/FeatureCollectionHandle.h"

#include "property-values/ValueObjectType.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task that can evolve specific types of scalar coverages over time
	 * (such as crustal thickness and topography).
	 *
	 * The domains are regular geometries (points/multipoints/polylines/polygons) whose positions
	 * are reconstructed by a ReconstructLayerProxy, whereas the scalar values associated with those positions
	 * can be evolved (according to strain calculated in ReconstructLayerTask) to account for the
	 * deformation in the resolved topological networks.
	 *
	 * If the type of scalar coverage does not support evolving (changing over time due to deformation)
	 * then the scalar values are not modified (they remain constant over time).
	 */
	class ReconstructScalarCoverageLayerTask :
			public LayerTask
	{
	public:
		/**
		 * App-logic parameters for a reconstructing coverages layer.
		 */
		class Params :
				public LayerTaskParams
		{
		public:

			//! Returns the scalar type.
			const GPlatesPropertyValues::ValueObjectType &
			get_scalar_type() const
			{
				return d_scalar_type;
			}

			//! Returns the list of scalar types available in the scalar coverage features.
			const std::vector<GPlatesPropertyValues::ValueObjectType> &
			get_scalar_types() const
			{
				return d_scalar_types;
			}

			//! Sets the scalar type, of the scalar coverage, for visualisation/processing.
			void
			set_scalar_type(
					const GPlatesPropertyValues::ValueObjectType &scalar_type);


			/**
			 * Returns the 'const' reconstructing coverage parameters.
			 */
			const ReconstructScalarCoverageParams &
			get_reconstruct_scalar_coverage_params() const
			{
				return d_reconstruct_scalar_coverage_params;
			}

			/**
			 * Sets the reconstructing coverage parameters.
			 *
			 * NOTE: This will flush any cached reconstructed scalar coverages in this layer.
			 */
			void
			set_reconstruct_scalar_coverage_params(
					const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params);

		private:

			/**
			 * The scalar type that has been selected for visualisation/processing.
			 */
			GPlatesPropertyValues::ValueObjectType d_scalar_type;

			//! The list of scalar types available in the scalar coverage features.
			std::vector<GPlatesPropertyValues::ValueObjectType> d_scalar_types;

			ReconstructScalarCoverageParams d_reconstruct_scalar_coverage_params;

			/**
			 * Is true if @a set_scalar_type has been called.
			 *
			 * Used to let ReconstructScalarCoverageLayerTask know that an external client has modified this state.
			 *
			 * ReconstructScalarCoverageLayerTask will reset this explicitly.
			 */
			bool d_set_scalar_type_called;

			/**
			 * Is true if @a set_reconstruct_scalar_coverage_params has been called.
			 *
			 * Used to let ReconstructScalarCoverageLayerTask know that an external client has modified this state.
			 *
			 * ReconstructScalarCoverageLayerTask will reset this explicitly.
			 */
			bool d_set_reconstruct_scalar_coverage_params_called;

			Params();

			void
			update(
					const GPlatesPropertyValues::ValueObjectType &scalar_type,
					const std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types);

			// Make friend so can access constructor and @a d_set_reconstruct_scalar_coverage_params_called.
			friend class ReconstructScalarCoverageLayerTask;
		};


		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<ReconstructScalarCoverageLayerTask>
		create_layer_task();


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::RECONSTRUCT_SCALAR_COVERAGE;
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
		 * Evolves scalar values for coverages that support it (eg, crustal thickness).
		 */
		reconstruct_scalar_coverage_layer_proxy_non_null_ptr_type d_reconstruct_scalar_coverage_layer_proxy;


		//! Constructor.
		ReconstructScalarCoverageLayerTask();
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERTASK_H
