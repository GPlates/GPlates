/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_PLATEVELOCITYUTILS_H
#define GPLATES_APP_LOGIC_PLATEVELOCITYUTILS_H

#include "file-io/FileInfo.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureCollectionHandleUnloader.h"
#include "model/ModelInterface.h"
#include "model/types.h"

// FIXME: There should be no view operation code here (this is app logic code).
// Fix 'solve_velocities()' below to not create any rendered geometries (move to
// a higher source code tier).
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesModel
{
	class Reconstruction;
	class ReconstructionTree;
}

namespace GPlatesAppLogic
{
	namespace PlateVelocityUtils
	{
		/**
		 * Returns true if any features in @a feature_collection can be used
		 * as a domain for velocity calculations (currently this is
		 * feature type "gpml:MeshNode").
		 */
		bool
		detect_velocity_mesh_nodes(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		/**
		 * Creates a new feature collection containing a feature of type
		 * "gpml:VelocityField" for every feature in @a feature_collection_with_mesh_nodes
		 * that can be used as a domain for velocity calculations (currently this is
		 * feature type "gpml:MeshNode").
		 *
		 * Note: Returned feature collection may not be valid (eg, if
		 * @a feature_collection_with_mesh_nodes is not valid).
		 * Note: Returned feature collection might be empty if
		 * @a feature_collection_with_mesh_nodes contains no features that have a
		 * domain suitable for velocity calculations (use @a detect_velocity_mesh_nodes
		 * to avoid this).
		 */
		GPlatesModel::FeatureCollectionHandleUnloader::shared_ref
		create_velocity_field_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_with_mesh_nodes,
				GPlatesModel::ModelInterface &model);


		/**
		 * Solves velocities for features in @a velocity_field_feature_collection.
		 *
		 * The velocities are calculated at the domain points specified in each feature
		 * (currently by the GmlMultiPoint in the gpml:VelocityField feature).
		 *
		 * The generated velocities are stored in a new property named "velocities" of
		 * type GmlDataBlock.
		 *
		 * The reconstruction trees @a reconstruction_tree_1 and @a reconstruction_tree_2
		 * are used to calculate velocities and correspond to times
		 * @a reconstruction_time_1 and @a reconstruction_time_2.
		 * The larger the time delta the larger the calculated velocities will be.
		 *
		 * The feature collection @a velocity_field_feature_collection ideally should be
		 * created with @a create_velocity_field_feature_collection so that it contains
		 * features and property(s) of the correct type for the velocity solver.
		 *
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		void
		solve_velocities(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &velocity_field_feature_collection,
				GPlatesModel::Reconstruction &reconstruction,
				GPlatesModel::ReconstructionTree &reconstruction_tree_1,
				GPlatesModel::ReconstructionTree &reconstruction_tree_2,
				const double &reconstruction_time_1,
				const double &reconstruction_time_2,
				GPlatesModel::integer_plate_id_type reconstruction_root,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_point_layer,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_arrow_layer);
	}
}

#endif // GPLATES_APP_LOGIC_PLATEVELOCITYUTILS_H
