/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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
		const RevisionId &revision_id,
		const FeatureCollectionHandle::weak_ref &target_collection)
{	
	GPlatesModel::FeatureHandle::non_null_ptr_type feature_handle =
			GPlatesModel::FeatureHandle::create(feature_type, revision_id);
	
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


namespace 
{
	template< typename FeatureCollectionIterator >
	void
	visit_feature_collections(
			FeatureCollectionIterator collections_begin, 
			FeatureCollectionIterator collections_end,
			GPlatesModel::FeatureVisitor &visitor) {

		using namespace GPlatesModel;

		// We visit each of the features in each of the feature collections in
		// the given range.
		FeatureCollectionIterator collections_iter = collections_begin;
		for ( ; collections_iter != collections_end; ++collections_iter) {

			FeatureCollectionHandle::weak_ref feature_collection = *collections_iter;

			// Before we dereference the weak_ref using 'operator->',
			// let's be sure that it's valid to dereference.
			if (feature_collection.is_valid()) {
				FeatureCollectionHandle::features_iterator iter =
						feature_collection->features_begin();
				FeatureCollectionHandle::features_iterator end =
						feature_collection->features_end();
				for ( ; iter != end; ++iter) {
					(*iter)->accept_visitor(visitor);
				}
			}
		}
	}
}


const GPlatesModel::Reconstruction::non_null_ptr_type
GPlatesModel::Model::create_reconstruction(
		const std::vector<FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const std::vector<FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const double &time,
		GPlatesModel::integer_plate_id_type root)
{
	ReconstructionGraph graph(time);
	ReconstructionTreePopulator rtp(time, graph);

	visit_feature_collections(
			reconstruction_features_collection.begin(),
			reconstruction_features_collection.end(),
			rtp);

	// Build the reconstruction tree, using 'root' as the root of the tree.
	ReconstructionTree::non_null_ptr_type tree = graph.build_tree(root);
	Reconstruction::non_null_ptr_type reconstruction = Reconstruction::create(tree);

	ReconstructedFeatureGeometryPopulator rfgp(time, root, *reconstruction,
			reconstruction->reconstruction_tree(),
			reconstruction->point_geometries(),
			reconstruction->polyline_geometries());

	visit_feature_collections(
		reconstructable_features_collection.begin(),
		reconstructable_features_collection.end(),
		rfgp);

	return reconstruction;
}


// Remove this function once it is possible to create empty reconstructions by simply passing empty
// lists of feature-collections into the previous function.
const GPlatesModel::Reconstruction::non_null_ptr_type
GPlatesModel::Model::create_empty_reconstruction(
		const double &time,
		GPlatesModel::integer_plate_id_type root)
{
	ReconstructionGraph graph(time);

	// Build the reconstruction tree, using 'root' as the root of the tree.
	ReconstructionTree::non_null_ptr_type tree = graph.build_tree(root);
	Reconstruction::non_null_ptr_type reconstruction = Reconstruction::create(tree);

	return reconstruction;
}


#ifdef HAVE_PYTHON
boost::python::tuple
GPlatesModel::Model::create_reconstruction_py(
		const double &time,
		unsigned long root)
{
	GPlatesModel::FeatureCollectionHandle::weak_ref reconstructable_features = GPlatesModel::Model::create_feature_collection();
	GPlatesModel::FeatureCollectionHandle::weak_ref reconstruction_features = GPlatesModel::Model::create_feature_collection();
	GPlatesModel::Reconstruction::non_null_ptr_type reconstruction = create_reconstruction(reconstructable_features, reconstruction_features, time, root);
	boost::python::list points;
	boost::python::list polylines;
	/*
	for (std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >::iterator p = point_reconstructions.begin();
			p != point_reconstructions.end(); ++p)
	{
		points.append(*(p->geometry()));
	}
	for (std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >::iterator p = polyline_reconstructions.begin();
			p != polyline_reconstructions.end(); ++p)
	{
		polylines.append(*(p->geometry()));
	}
	*/
	return boost::python::make_tuple(points, polylines);
}


using namespace boost::python;


void
GPlatesModel::export_Model()
{
	class_<GPlatesModel::Model>("Model", init<>())
		.def("create_reconstruction", &GPlatesModel::Model::create_reconstruction_py)
	;
}
#endif

