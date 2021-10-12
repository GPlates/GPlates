/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_APP_LOGIC_PALEOMAGWORKFLOW_H
#define GPLATES_APP_LOGIC_PALEOMAGWORKFLOW_H

#include <vector>
#include <QString>

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionWorkflow.h"

#include "file-io/FileInfo.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureCollectionHandleUnloader.h"
#include "model/ModelInterface.h"
#include "model/types.h"

// FIXME: There should be no view operation code here (this is app logic code).
// Fix 'solve_velocities()' below to not create any rendered geometries (move to
// a higher source code tier).
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesFeatureVisitors
{
	class TopologyResolver;
}

namespace GPlatesGui
{
	class ColourTable;
}

namespace GPlatesModel
{
	class Reconstruction;
}

namespace GPlatesAppLogic
{
	/**
	 * Class to handle velocity feature collection loading/unloading and calculations.
	 */
	class PaleomagWorkflow : public
			FeatureCollectionWorkflow
	{
	public:
		/**
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		PaleomagWorkflow(
				ApplicationState &application_state,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type paleomag_layer) :
			d_model(application_state.get_model_interface()),
			d_paleomag_layer(paleomag_layer)
		{
		}


		virtual
		tag_type
		get_tag() const
		{
			return "PaleomagWorkflow";
		}


		/**
		 * Priority of this @a FeatureCollectionFileState workflow.
		 */
		virtual
		priority_type
		get_priority() const
		{
			return PRIORITY_NORMAL;
		}


		/**
		 * Callback method notifying of new file (called from @a FeatureCollectionFileState).
		 *
		 * If the feature collection contains features that can be used for
		 * velocity calculations then this method returns true and a new
		 * feature collection is created internally that is used directly
		 * by the velocity solver.
		 */
		virtual
		bool
		add_file(
				FeatureCollectionFileState::file_iterator file_iter,
				const ClassifyFeatureCollection::classifications_type &classification,
				bool used_by_higher_priority_workflow);


		/**
		 * Callback method notifying about to remove file (called from @a FeatureCollectionFileState).
		 */
		virtual
		void
		remove_file(
				FeatureCollectionFileState::file_iterator file_iter);


		/**
		 * Callback method notifying file has changed (called from @a FeatureCollectionFileState).
		 *
		 * If the feature collection contains features that can be used for
		 * velocity calculations then this method returns true and a new
		 * feature collection is created internally that is used directly
		 * by the velocity solver.
		 */
		virtual
		bool
		changed_file(
				FeatureCollectionFileState::file_iterator file_iter,
				GPlatesFileIO::File &old_file,
				const ClassifyFeatureCollection::classifications_type &new_classification);


		virtual
		void
		set_file_active(
				FeatureCollectionFileState::file_iterator file_iter,
				bool active);


		/**
		 * Returns the number of velocity feature collections currently being calculated.
		 */
		unsigned int
		get_num_paleomag_feature_collections() const
		{
			return d_paleomag_feature_collection_infos.size();
		}


		/**
		 * 
		 *
		 * 
		 * 
		 */
		void
		draw_paleomag_features(
				GPlatesModel::Reconstruction &reconstruction,
				const double &reconstruction_time,
				const GPlatesGui::ColourTable &colour_table);

	private:
		/**
		 * Used to associate a mesh node feature collection with a
		 * velocity field feature collection so that when the former is deleted
		 * we can stop calculating velocities for the latter.
		 */
		struct PaleomagFeatureCollectionInfo
		{
			PaleomagFeatureCollectionInfo(
					FeatureCollectionFileState::file_iterator file_iter):
				d_file_iterator(file_iter),
				d_active(false)
			{  }

			FeatureCollectionFileState::file_iterator d_file_iterator;
			bool d_active;
		};

		/**
		 * Typedef for a sequence of associations between mesh node velocity feature collections
		 * and corresponding velocity field feature collections.
		 */
		typedef std::vector<PaleomagFeatureCollectionInfo>
				paleomag_feature_collection_info_seq_type;

		GPlatesModel::ModelInterface d_model;
		paleomag_feature_collection_info_seq_type d_paleomag_feature_collection_infos;
		
		/*
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type d_paleomag_layer;
	};
}

#endif // GPLATES_APP_LOGIC_PALEOMAGWORKFLOW_H
