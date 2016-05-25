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

#include "AppLogicFwd.h"
#include "LayerTask.h"
#include "LayerTaskParams.h"

#include "model/FeatureHandle.h"
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

		/**
		 * App-logic parameters for a scalar field layer.
		 */
		class Params :
				public LayerTaskParams
		{
		public:

			//! Returns the raster feature or boost::none if one is currently not set on the layer.
			const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &
			get_scalar_field_feature() const;

			//! Returns the minimum depth layer radius of scalar field or none if no field.
			boost::optional<double>
			get_minimum_depth_layer_radius() const
			{
				return d_minimum_depth_layer_radius;
			}

			//! Returns the maximum depth layer radius of scalar field or none if no field.
			boost::optional<double>
			get_maximum_depth_layer_radius() const
			{
				return d_maximum_depth_layer_radius;
			}

			/**
			 * Returns the minimum scalar value across the entire scalar field or none if no field.
			 *
			 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
			 */
			boost::optional<double>
			get_scalar_min() const
			{
				return d_scalar_min;
			}

			/**
			 * Returns the maximum scalar value across the entire scalar field or none if no field.
			 *
			 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
			 */
			boost::optional<double>
			get_scalar_max() const
			{
				return d_scalar_max;
			}

			/**
			 * Returns the mean scalar value across the entire scalar field or none if no field.
			 *
			 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
			 */
			boost::optional<double>
			get_scalar_mean() const
			{
				return d_scalar_mean;
			}

			/**
			 * Returns the standard deviation of scalar values across the entire scalar field or none if no field.
			 *
			 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
			 */
			boost::optional<double>
			get_scalar_standard_deviation() const
			{
				return d_scalar_standard_deviation;
			}

			/**
			 * Returns the minimum gradient magnitude across the entire scalar field or none if no field.
			 *
			 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
			 */
			boost::optional<double>
			get_gradient_magnitude_min() const
			{
				return d_gradient_magnitude_min;
			}

			/**
			 * Returns the maximum gradient magnitude across the entire scalar field or none if no field.
			 *
			 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
			 */
			boost::optional<double>
			get_gradient_magnitude_max() const
			{
				return d_gradient_magnitude_max;
			}

			/**
			 * Returns the mean gradient magnitude across the entire scalar field or none if no field.
			 *
			 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
			 */
			boost::optional<double>
			get_gradient_magnitude_mean() const
			{
				return d_gradient_magnitude_mean;
			}

			/**
			 * Returns the standard deviation of gradient magnitudes across the entire scalar field or none if no field.
			 *
			 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
			 */
			boost::optional<double>
			get_gradient_magnitude_standard_deviation() const
			{
				return d_gradient_magnitude_standard_deviation;
			}

		private:

			//! The scalar field feature.
			boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_scalar_field_feature;

			boost::optional<double> d_minimum_depth_layer_radius;
			boost::optional<double> d_maximum_depth_layer_radius;

			boost::optional<double> d_scalar_min;
			boost::optional<double> d_scalar_max;
			boost::optional<double> d_scalar_mean;
			boost::optional<double> d_scalar_standard_deviation;

			boost::optional<double> d_gradient_magnitude_min;
			boost::optional<double> d_gradient_magnitude_max;
			boost::optional<double> d_gradient_magnitude_mean;
			boost::optional<double> d_gradient_magnitude_standard_deviation;

			Params();

			void
			set_scalar_field_feature(
					boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref);

			//! Update state to reflect a new, or modified, raster feature.
			void
			updated_scalar_field_feature();

			// Make friend so can access constructor.
			friend class ScalarField3DLayerTask;
		};


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


		~ScalarField3DLayerTask();


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
		LayerTaskParams &
		get_layer_task_params()
		{
			return d_layer_task_params;
		}

	private:

		/**
		 * Extra parameters for this layer.
		 */
		Params d_layer_task_params;

		scalar_field_3d_layer_proxy_non_null_ptr_type d_scalar_field_layer_proxy;


		//! Constructor.
		ScalarField3DLayerTask();
	};
}

#endif // GPLATES_APP_LOGIC_SCALARFIELD3DLAYERTASK_H
