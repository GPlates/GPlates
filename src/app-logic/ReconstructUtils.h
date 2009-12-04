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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTUTILS_H
#define GPLATES_APP_LOGIC_RECONSTRUCTUTILS_H

#include <utility>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "model/FeatureCollectionHandle.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	namespace ReconstructUtils
	{
		/**
		 * Create and return a reconstruction tree for the reconstruction time @a time,
		 * with root @a root.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 */
		const GPlatesModel::ReconstructionTree::non_null_ptr_type
		create_reconstruction_tree(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection,
				const double &time,
				GPlatesModel::integer_plate_id_type root);


		/**
		 * Create and return a reconstruction for the reconstruction time @a time, with
		 * root @a root.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 */
		const GPlatesModel::Reconstruction::non_null_ptr_type
		create_reconstruction(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstructable_features_collection,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection,
				const double &time,
				GPlatesModel::integer_plate_id_type root);


		/**
		 * Create and return an empty reconstruction for the reconstruction time @a time,
		 * with root @a root.
		 *
		 * The reconstruction tree contained within the reconstruction will also be empty.
		 *
		 * FIXME:  Remove this function once it is possible to create empty reconstructions
		 * by simply passing empty lists of feature-collections into the prev function.
		 */
		const GPlatesModel::Reconstruction::non_null_ptr_type
		create_empty_reconstruction(
				const double &time,
				GPlatesModel::integer_plate_id_type root);
	}
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTUTILS_H
