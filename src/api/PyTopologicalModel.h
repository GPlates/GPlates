/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2020 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYTOPOLOGICALMODEL_H
#define GPLATES_API_PYTOPOLOGICALMODEL_H

#include <vector>

#include "PyFeatureCollection.h"
#include "PyRotationModel.h"

#include "app-logic/TopologyReconstruct.h"

#include "file-io/File.h"

#include "global/python.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/ReferenceCount.h"


namespace GPlatesApi
{
	/**
	 * Dynamic plates and deforming networks, and their associated rotation model.
	 */
	class TopologicalModel :
			public GPlatesUtils::ReferenceCount<TopologicalModel>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TopologicalModel> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologicalModel> non_null_ptr_to_const_type;


		/**
		 * Create a topological model from topological features and associated rotation model.
		 */
		static
		non_null_ptr_type
		create(
				const FeatureCollectionSequenceFunctionArgument &topological_features,
				// Note we're using 'RotationModelFunctionArgument::function_argument_type' instead of
				// just 'RotationModelFunctionArgument' since we want to know if it's an existing RotationModel...
				const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
				const GPlatesPropertyValues::GeoTimeInstant &oldest_time,
				const GPlatesPropertyValues::GeoTimeInstant &youngest_time,
				const double &time_increment);


		/**
		 * Return the topological feature collections used to create this topological model.
		 */
		void
		get_topological_feature_collections(
				std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &topological_feature_collections) const;


		/**
		 * Return the topological feature collection files used to create this topological model.
		 *
		 * NOTE: Any feature collections that did not come from files will have empty filenames.
		 */
		void
		get_files(
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_feature_collections_files) const;


		/**
		 * Get the rotation model.
		 */
		RotationModel::non_null_ptr_type
		get_rotation_model() const
		{
			return d_rotation_model;
		}

	private:

		TopologicalModel(
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_feature_collections_files,
				const RotationModel::non_null_ptr_type &rotation_model) :
			d_topological_feature_collection_files(topological_feature_collections_files),
			d_rotation_model(rotation_model)
		{  }


		/**
		 * Keep the topological feature collections alive (by using intrusive pointers).
		 *
		 * We also keep track of the files the topological feature collections came from.
		 * If any feature collection did not come from a file then it will have an empty filename.
		 */
		std::vector<GPlatesFileIO::File::non_null_ptr_type> d_topological_feature_collection_files;

		RotationModel::non_null_ptr_type d_rotation_model;
	};
}

#endif // GPLATES_API_PYTOPOLOGICALMODEL_H
