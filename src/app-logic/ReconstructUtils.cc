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

#include <boost/foreach.hpp>

#include "ReconstructUtils.h"

#include "AppLogicUtils.h"
#include "ReconstructedFeatureGeometryPopulator.h"
#include "ReconstructionGraph.h"
#include "ReconstructionTreePopulator.h"
#include "TopologyUtils.h"
#include "TopologyBoundaryResolver.h"
#include "TopologyNetworkResolver.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "utils/NullIntrusivePointerHandler.h"


namespace
{
	/**
	 * Can either rotate a present day @a GeometryOnSphere into the past or
	 * rotate a @a GeometryOnSphere from the past back to present day.
	 */
	class ReconstructGeometryOnSphere :
		private GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		ReconstructGeometryOnSphere(
				const GPlatesModel::integer_plate_id_type plate_id,
				const GPlatesAppLogic::ReconstructionTree &recon_tree,
				bool reverse_reconstruct) :
		d_plate_id(plate_id),
		d_recon_tree(recon_tree),
		d_reverse_reconstruct(reverse_reconstruct)
		{
		}

		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry)
		{
			geometry->accept_visitor(*this);

			return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(
					d_reconstructed_geom.get(),
					GPlatesUtils::NullIntrusivePointerHandler());
		}

	private:
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_reconstructed_geom =
					GPlatesAppLogic::ReconstructUtils::reconstruct(
							multi_point_on_sphere,
							d_plate_id,
							d_recon_tree,
							d_reverse_reconstruct).get();
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_reconstructed_geom =
					GPlatesAppLogic::ReconstructUtils::reconstruct(
							point_on_sphere,
							d_plate_id,
							d_recon_tree,
							d_reverse_reconstruct).get();
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_reconstructed_geom =
					GPlatesAppLogic::ReconstructUtils::reconstruct(
							polygon_on_sphere,
							d_plate_id,
							d_recon_tree,
							d_reverse_reconstruct).get();
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_reconstructed_geom =
					GPlatesAppLogic::ReconstructUtils::reconstruct(
							polyline_on_sphere,
							d_plate_id,
							d_recon_tree,
							d_reverse_reconstruct).get();
		}

	private:
		const GPlatesModel::integer_plate_id_type d_plate_id;
		const GPlatesAppLogic::ReconstructionTree &d_recon_tree;
		bool d_reverse_reconstruct;
		GPlatesMaths::GeometryOnSphere::maybe_null_ptr_to_const_type d_reconstructed_geom;
	};
}


bool
GPlatesAppLogic::ReconstructUtils::is_reconstruction_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	return ReconstructionTreePopulator::can_process(feature_ref);
}


bool
GPlatesAppLogic::ReconstructUtils::has_reconstruction_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref =
				(*feature_collection_iter)->reference();

		if (is_reconstruction_feature(feature_ref))
		{
			return true; 
		}
	}

	return false;
}


const GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
		const double &time,
		GPlatesModel::integer_plate_id_type root,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection)
{
	ReconstructionGraph graph(time, reconstruction_features_collection);
	ReconstructionTreePopulator rtp(time, graph);

	AppLogicUtils::visit_feature_collections(
			reconstruction_features_collection.begin(),
			reconstruction_features_collection.end(),
			rtp);

	// Build the reconstruction tree, using 'root' as the root of the tree.
	ReconstructionTree::non_null_ptr_type tree = graph.build_tree(root);

	return tree;
}


#if 0
const GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
		const double &time,
		GPlatesModel::integer_plate_id_type root,
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &reconstruction_features)
{
	GPlatesModel::ReconstructionGraph graph(time);
	GPlatesModel::ReconstructionTreePopulator rtp(time, graph);

	BOOST_FOREACH(const GPlatesModel::FeatureHandle::weak_ref &feature_ref, reconstruction_features)
	{
		rtp.visit_feature(feature_ref);
	}
	
	// Build the reconstruction tree, using 'root' as the root of the tree.
	GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree = graph.build_tree(root);

	return tree;
}
#endif


bool
GPlatesAppLogic::ReconstructUtils::is_reconstructable_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	return ReconstructedFeatureGeometryPopulator::can_process(feature_ref);
}


bool
GPlatesAppLogic::ReconstructUtils::has_reconstructable_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref =
				(*feature_collection_iter)->reference();

		if (is_reconstructable_feature(feature_ref))
		{
			return true; 
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstructable_features_collection)
{
	ReconstructionGeometryCollection::non_null_ptr_type reconstruction_geom_collection =
			ReconstructionGeometryCollection::create(reconstruction_tree);

	ReconstructedFeatureGeometryPopulator rfgp(*reconstruction_geom_collection);

	AppLogicUtils::visit_feature_collections(
			reconstructable_features_collection.begin(),
			reconstructable_features_collection.end(),
			rfgp);

	TopologyBoundaryResolver topology_boundary_resolver(*reconstruction_geom_collection);

	AppLogicUtils::visit_feature_collections(
			reconstructable_features_collection.begin(),
			reconstructable_features_collection.end(),
			topology_boundary_resolver);

	// Visit topological network features.
	TopologyNetworkResolver topology_network_resolver(*reconstruction_geom_collection);

	AppLogicUtils::visit_feature_collections(
			reconstructable_features_collection.begin(),
			reconstructable_features_collection.end(),
			topology_network_resolver);

	return reconstruction_geom_collection;
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::integer_plate_id_type reconstruction_plate_id,
		const ReconstructionTree &reconstruction_tree,
		bool reverse_reconstruct)
{
	ReconstructGeometryOnSphere reconstruct_geom_on_sphere(
			reconstruction_plate_id, reconstruction_tree, reverse_reconstruct);

	return reconstruct_geom_on_sphere.reconstruct(geometry);
}
