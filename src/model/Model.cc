/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#include <boost/intrusive_ptr.hpp>
#ifdef HAVE_PYTHON
# include <boost/python.hpp>
#endif

#include "Model.h"
#include "DummyTransactionHandle.h"
#include "FeatureHandle.h"
#include "FeatureRevision.h"
#include "FeatureVisitor.h"
#include "ReconstructionGraph.h"
#include "ReconstructionTreePopulator.h"
#include "ReconstructedFeatureGeometryPopulator.h"

#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"


GPlatesModel::Model::Model():
	d_feature_store(FeatureStore::create())
{  }


const GPlatesModel::FeatureCollectionHandle::weak_ref
GPlatesModel::Model::create_feature_collection()
{
	GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
			GPlatesModel::FeatureCollectionHandle::create();
	GPlatesModel::FeatureStoreRootHandle::collections_iterator iter =
			d_feature_store->root()->append_feature_collection(feature_collection,
			transaction);
	transaction.commit();
	return (*iter)->reference();
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::Model::create_feature(
		const FeatureType &feature_type,
		const FeatureCollectionHandle::weak_ref &target_collection)
{
	FeatureId feature_id;
	return create_feature(feature_type, feature_id, target_collection);
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::Model::create_feature(
		const FeatureType &feature_type,
		const FeatureId &feature_id,
		const FeatureCollectionHandle::weak_ref &target_collection)
{
	GPlatesModel::FeatureHandle::non_null_ptr_type feature_handle =
			GPlatesModel::FeatureHandle::create(feature_type, feature_id);
	
	DummyTransactionHandle transaction(__FILE__, __LINE__);
	target_collection->append_feature(feature_handle, transaction);
	transaction.commit();

	return feature_handle->reference();
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::Model::create_feature(
		const FeatureType &feature_type,
		const FeatureId &feature_id,
		const RevisionId &revision_id,
		const FeatureCollectionHandle::weak_ref &target_collection)
{
	GPlatesModel::FeatureHandle::non_null_ptr_type feature_handle =
			GPlatesModel::FeatureHandle::create(feature_type, feature_id, revision_id);
	
	DummyTransactionHandle transaction(__FILE__, __LINE__);
	target_collection->append_feature(feature_handle, transaction);
	transaction.commit();

	return feature_handle->reference();
}


#ifdef HAVE_PYTHON
using namespace boost::python;

void
GPlatesModel::export_Model()
{
	// FIXME: I moved create_reconstruction() over to GPlatesAppLogic::Reconstruct so
	// this needs fixing.
	class_<GPlatesModel::Model>("Model", init<>())
		.def("create_reconstruction", &GPlatesModel::Model::create_reconstruction_py)
	;
}
#endif

