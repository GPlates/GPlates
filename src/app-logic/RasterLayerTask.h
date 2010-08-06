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
 
#ifndef GPLATES_APP_LOGIC_RASTERLAYERTASK_H
#define GPLATES_APP_LOGIC_RASTERLAYERTASK_H


#include <utility>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "LayerTask.h"

#include "model/FeatureCollectionHandle.h"

#include "property-values/Georeferencing.h"
#include "property-values/RawRaster.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task that resolves a geo-referenced raster (if it's time-dependent) and
	 * optionally reconstructs it using an input channel containing polygons.
	 *
	 * Also there's another optional input channel referencing the output of an age grid
	 * layer that provides improved resolution for masking those parts of the present-day
	 * globe that don't exist in the past (eg, ocean floor near mid-ocean ridge).
	 */
	class RasterLayerTask :
			public LayerTask
	{
	public:
		/**
		 * Returns the name and description of this layer task.
		 *
		 * This is useful for display to the user so they know what this layer does.
		 */
		static
		std::pair<QString, QString>
		get_name_and_description();


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
		boost::shared_ptr<RasterLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<RasterLayerTask>(new RasterLayerTask());
		}


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::RASTER;
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
				const input_data_type &input_data,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchored_plate_id,
				const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree);

	private:
		static const char *RASTER_FEATURE_CHANNEL_NAME;
		static const char *POLYGON_FEATURES_CHANNEL_NAME;

		// FIXME: Remove these when raster is a property/band of a feature...
		boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> d_georeferencing;
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> d_raster;

#if 0 // FIXME: Use this when raster is a property/band of a feature...
		RasterLayerTask()
		{  }
#else
		RasterLayerTask();
#endif
	};
}

#endif // GPLATES_APP_LOGIC_RASTERLAYERTASK_H
