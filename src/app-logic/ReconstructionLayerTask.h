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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONLAYER_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONLAYER_H

#include <utility>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "LayerTask.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task that generates a @a ReconstructionTree from feature collection(s)
	 * containing reconstruction features.
	 */
	class ReconstructionLayerTask :
			public LayerTask
	{
	public:

		/**
		 * Can be used to create a layer automatically when a file is first loaded.
		 */
		static
		bool
		is_primary_layer_task_type()
		{
			return true;
		}


		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<ReconstructionLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<ReconstructionLayerTask>(new ReconstructionLayerTask());
		}


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::RECONSTRUCTION;
		}


		virtual
		std::vector<Layer::input_channel_definition_type>
		get_input_channel_definitions() const;


		virtual
		QString
		get_main_input_feature_collection_channel() const;


		virtual
		Layer::LayerOutputDataType
		get_output_definition() const;


		virtual
		bool
		is_topological_layer_task() const
		{
			return false;
		}


		virtual
		boost::optional<layer_task_data_type>
		process(
				const Layer &layer_handle /* the layer invoking this */,
				const input_data_type &input_data,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchored_plate_id,
				const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree);

	private:
		static const char *RECONSTRUCTION_FEATURES_CHANNEL_NAME;


		ReconstructionLayerTask()
		{  }
	};
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONLAYER_H
