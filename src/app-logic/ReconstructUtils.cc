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

#include "ReconstructUtils.h"

#include "AppLogicUtils.h"
#include "ReconstructedFeatureGeometryPopulator.h"
#include "TopologyUtils.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/ReconstructionGraph.h"
#include "model/ReconstructionTreePopulator.h"

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
				const GPlatesModel::ReconstructionTree &recon_tree,
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
		const GPlatesModel::ReconstructionTree &d_recon_tree;
		bool d_reverse_reconstruct;
		GPlatesMaths::GeometryOnSphere::maybe_null_ptr_to_const_type d_reconstructed_geom;
	};
}


const GPlatesModel::ReconstructionTree::non_null_ptr_type
GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection,
		const double &time,
		GPlatesModel::integer_plate_id_type root)
{
	GPlatesModel::ReconstructionGraph graph(time);
	GPlatesModel::ReconstructionTreePopulator rtp(time, graph);

	GPlatesAppLogic::AppLogicUtils::visit_feature_collections(
			reconstruction_features_collection.begin(),
			reconstruction_features_collection.end(),
			rtp);

	// Build the reconstruction tree, using 'root' as the root of the tree.
	GPlatesModel::ReconstructionTree::non_null_ptr_type tree = graph.build_tree(root);
	return tree;
}


const GPlatesModel::Reconstruction::non_null_ptr_type
GPlatesAppLogic::ReconstructUtils::create_reconstruction(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection,
		const double &time,
		GPlatesModel::integer_plate_id_type root)
{
	GPlatesModel::ReconstructionTree::non_null_ptr_type tree =
			create_reconstruction_tree(reconstruction_features_collection, time, root);
	GPlatesModel::Reconstruction::non_null_ptr_type reconstruction =
			GPlatesModel::Reconstruction::create(tree, reconstruction_features_collection);

	GPlatesAppLogic::ReconstructedFeatureGeometryPopulator rfgp(time, root, *reconstruction,
			reconstruction->reconstruction_tree());

	GPlatesAppLogic::AppLogicUtils::visit_feature_collections(
		reconstructable_features_collection.begin(),
		reconstructable_features_collection.end(),
		rfgp);

	// Create resolved topologies and store them in 'reconstruction'.
	TopologyUtils::resolve_topologies(
			time, *reconstruction, reconstructable_features_collection);

	return reconstruction;
}


// Remove this function once it is possible to create empty reconstructions by simply passing empty
// lists of feature-collections into the previous function.
const GPlatesModel::Reconstruction::non_null_ptr_type
GPlatesAppLogic::ReconstructUtils::create_empty_reconstruction(
		const double &time,
		GPlatesModel::integer_plate_id_type root)
{
	GPlatesModel::ReconstructionGraph graph(time);

	// Build the reconstruction tree, using 'root' as the root of the tree.
	GPlatesModel::ReconstructionTree::non_null_ptr_type tree = graph.build_tree(root);
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> empty_vector;
	GPlatesModel::Reconstruction::non_null_ptr_type reconstruction =
			GPlatesModel::Reconstruction::create(tree, empty_vector);

	return reconstruction;
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::integer_plate_id_type reconstruction_plate_id,
		const GPlatesModel::ReconstructionTree &reconstruction_tree,
		bool reverse_reconstruct)
{
	ReconstructGeometryOnSphere reconstruct_geom_on_sphere(
			reconstruction_plate_id, reconstruction_tree, reverse_reconstruct);

	return reconstruct_geom_on_sphere.reconstruct(geometry);
}
