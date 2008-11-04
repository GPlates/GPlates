/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_MODEL_H
#define GPLATES_MODEL_MODEL_H

#include <boost/intrusive_ptr.hpp>
#ifdef HAVE_PYTHON
# include <boost/python.hpp>
#endif

#include <vector>
#include "ModelInterface.h"
#include "FeatureStore.h"
#include "FeatureIdRegistry.h"


namespace GPlatesModel
{
	class Model: public ModelInterface
	{
	public:
		Model();

		const FeatureCollectionHandle::weak_ref
		create_feature_collection();

		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const FeatureCollectionHandle::weak_ref &target_collection);

		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const FeatureId &feature_id,
				const FeatureCollectionHandle::weak_ref &target_collection);

		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const RevisionId &revision_id,
				const FeatureCollectionHandle::weak_ref &target_collection);

		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const FeatureId &feature_id,
				const RevisionId &revision_id,
				const FeatureCollectionHandle::weak_ref &target_collection);

		const Reconstruction::non_null_ptr_type
		create_reconstruction(
				const std::vector<FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const std::vector<FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
				const double &time,
				integer_plate_id_type root);

		const Reconstruction::non_null_ptr_type
		create_empty_reconstruction(
				const double &time,
				integer_plate_id_type root);

		boost::optional<GPlatesModel::FeatureHandle::weak_ref>
		get_feature_for_id(
				const GPlatesModel::FeatureId &feature_id);

#ifdef HAVE_PYTHON
		// A Python wrapper for create_reconstruction
		boost::python::tuple
		create_reconstruction_py(
				const double &time,
				unsigned long root);
#endif

	private:
		FeatureStore::non_null_ptr_type d_feature_store;
		FeatureIdRegistry d_feature_id_registry;
	};

	void export_Model();
}

#endif  // GPLATES_MODEL_MODEL_H
