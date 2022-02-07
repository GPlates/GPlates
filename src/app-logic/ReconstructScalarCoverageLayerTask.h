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
#include <boost/shared_ptr.hpp>
#include <QObject>

#include "LayerTask.h"
#include "ReconstructScalarCoverageLayerParams.h"
#include "ReconstructScalarCoverageLayerProxy.h"
#include "ScalarCoverageFeatureProperties.h"

#include "model/FeatureCollectionHandle.h"


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
		get_layer_proxy()
		{
			return d_reconstruct_scalar_coverage_layer_proxy;
		}


		virtual
		LayerParams::non_null_ptr_type
		get_layer_params()
		{
			return d_layer_params;
		}

	private Q_SLOTS:

		void
		handle_reconstruct_scalar_coverage_params_modified(
				GPlatesAppLogic::ReconstructScalarCoverageLayerParams &layer_params);

	private:

		/**
		 * Evolves scalar values for coverages that support it (eg, crustal thickness).
		 */
		ReconstructScalarCoverageLayerProxy::non_null_ptr_type d_reconstruct_scalar_coverage_layer_proxy;

		/**
		 * Parameters used when calculating reconstructed scalar coverages.
		 *
		 * NOTE: Should be declared after @a d_reconstruct_scalar_coverage_layer_proxy.
		 */
		ReconstructScalarCoverageLayerParams::non_null_ptr_type d_layer_params;


		//! Constructor.
		ReconstructScalarCoverageLayerTask();
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERTASK_H
