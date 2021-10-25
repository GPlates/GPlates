/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <map>
#include <QDebug>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "ReconstructUtils.h"

#include "AppLogicUtils.h"
#include "ReconstructContext.h"
#include "ReconstructionTreePopulator.h"
#include "SmallCircleGeometryPopulator.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/types.h"

namespace
{
	/**
	 * Can either rotate a present day @a GeometryOnSphere into the past or
	 * rotate a @a GeometryOnSphere from the past back to present day.
	 */
	class ReconstructGeometryOnSphereByPlateId :
		private GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		ReconstructGeometryOnSphereByPlateId(
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

			return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(d_reconstructed_geom.get());
		}

	private:
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_reconstructed_geom =
					GPlatesAppLogic::ReconstructUtils::reconstruct_by_plate_id(
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
					GPlatesAppLogic::ReconstructUtils::reconstruct_by_plate_id(
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
					GPlatesAppLogic::ReconstructUtils::reconstruct_by_plate_id(
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
					GPlatesAppLogic::ReconstructUtils::reconstruct_by_plate_id(
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

	/**
	 * Can either rotate a present day @a GeometryOnSphere into the past or
	 * rotate a @a GeometryOnSphere from the past back to present day.
	 */
	class ReconstructGeometryOnSphereWithHalfStage :
		private GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		ReconstructGeometryOnSphereWithHalfStage(
				const GPlatesModel::integer_plate_id_type left_plate_id,
				const GPlatesModel::integer_plate_id_type right_plate_id,
				const double &reconstruction_time,
				const GPlatesAppLogic::ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &spreading_asymmetry,
				const double &half_stage_rotation_interval,
				bool reverse_reconstruct) :
			d_left_plate_id(left_plate_id),
			d_right_plate_id(right_plate_id),
			d_reconstruction_time(reconstruction_time),
			d_recon_tree_creator(reconstruction_tree_creator),
			d_spreading_asymmetry(spreading_asymmetry),
			d_half_stage_rotation_interval(half_stage_rotation_interval),
			d_reverse_reconstruct(reverse_reconstruct)
		{
		}

		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry)
		{
			geometry->accept_visitor(*this);

			return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(d_reconstructed_geom.get());
		}

	private:
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_reconstructed_geom =
					GPlatesAppLogic::ReconstructUtils::reconstruct_as_half_stage(
							multi_point_on_sphere,
							d_left_plate_id,
							d_right_plate_id,
							d_reconstruction_time,
							d_recon_tree_creator,
							d_spreading_asymmetry,
							d_half_stage_rotation_interval,
							d_reverse_reconstruct).get();
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_reconstructed_geom =
					GPlatesAppLogic::ReconstructUtils::reconstruct_as_half_stage(
							point_on_sphere,
							d_left_plate_id,
							d_right_plate_id,
							d_reconstruction_time,
							d_recon_tree_creator,
							d_spreading_asymmetry,
							d_half_stage_rotation_interval,
							d_reverse_reconstruct).get();
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_reconstructed_geom =
					GPlatesAppLogic::ReconstructUtils::reconstruct_as_half_stage(
							polygon_on_sphere,
							d_left_plate_id,
							d_right_plate_id,
							d_reconstruction_time,
							d_recon_tree_creator,
							d_spreading_asymmetry,
							d_half_stage_rotation_interval,
							d_reverse_reconstruct).get();
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_reconstructed_geom =
					GPlatesAppLogic::ReconstructUtils::reconstruct_as_half_stage(
							polyline_on_sphere,
							d_left_plate_id,
							d_right_plate_id,
							d_reconstruction_time,
							d_recon_tree_creator,
							d_spreading_asymmetry,
							d_half_stage_rotation_interval,
							d_reverse_reconstruct).get();
		}

	private:
		const GPlatesModel::integer_plate_id_type d_left_plate_id;
		const GPlatesModel::integer_plate_id_type d_right_plate_id;
		double d_reconstruction_time;
		const GPlatesAppLogic::ReconstructionTreeCreator &d_recon_tree_creator;
		double d_spreading_asymmetry;
		double d_half_stage_rotation_interval;
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


bool
GPlatesAppLogic::ReconstructUtils::is_reconstructable_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
		const ReconstructMethodRegistry &reconstruct_method_registry)
{
	// See if any reconstruct methods can reconstruct the current feature.
	return reconstruct_method_registry.can_reconstruct_feature(feature_ref);
}


bool
GPlatesAppLogic::ReconstructUtils::has_reconstructable_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
		const ReconstructMethodRegistry &reconstruct_method_registry)
{
	// Iterate over the features in the feature collection.
	GPlatesModel::FeatureCollectionHandle::const_iterator features_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator features_end = feature_collection->end();
	for ( ; features_iter != features_end; ++features_iter)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*features_iter)->reference();

		// See if any reconstruct methods can reconstruct the current feature.
		if (reconstruct_method_registry.can_reconstruct_feature(feature_ref))
		{
			// Only need to be able to process one feature to be able to process the entire collection.
			return true;
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries,
		const double &reconstruction_time,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructParams &reconstruct_params)
{
	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_features;
	const ReconstructHandle::type reconstruct_handle = reconstruct(
			reconstructed_features,
			reconstruction_time,
			reconstruct_method_registry,
			reconstructable_features_collection,
			reconstruction_tree_creator,
			reconstruct_params);

	// Copy the RFGs in the ReconstructContext::ReconstructedFeature's.
	// The ReconstructContext::ReconstructedFeature's store RFGs and geometry property handles.
	// This format only needs the RFG.
	BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
	{
		const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructed_feature_reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructed_feature_reconstructions)
		{
			reconstructed_feature_geometries.push_back(
					reconstruction.get_reconstructed_feature_geometry());
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructContext::Reconstruction> &reconstructions,
		const double &reconstruction_time,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructParams &reconstruct_params)
{
	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_features;
	const ReconstructHandle::type reconstruct_handle = reconstruct(
			reconstructed_features,
			reconstruction_time,
			reconstruct_method_registry,
			reconstructable_features_collection,
			reconstruction_tree_creator,
			reconstruct_params);

	// Copy the ReconstructContext::Reconstruction's in the ReconstructContext::ReconstructedFeature's.
	BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
	{
		const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructed_feature_reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructed_feature_reconstructions)
		{
			reconstructions.push_back(reconstruction);
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features,
		const double &reconstruction_time,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructParams &reconstruct_params)
{
	// Create a reconstruct context - it will determine which reconstruct method each
	// reconstructable feature requires.
	ReconstructContext reconstruct_context(reconstruct_method_registry);
	reconstruct_context.set_features(reconstructable_features_collection);

	// Create the context state in which to reconstruct.
	const ReconstructMethodInterface::Context reconstruct_method_context(
			reconstruct_params,
			reconstruction_tree_creator);
	const ReconstructContext::context_state_reference_type context_state =
			reconstruct_context.create_context_state(reconstruct_method_context);

	// Reconstruct the reconstructable features.
	return reconstruct_context.reconstruct_feature_geometries(
			reconstructed_features,
			context_state,
			reconstruction_time);
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const ReconstructParams &reconstruct_params,
		unsigned int reconstruction_tree_cache_size)
{
	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_features;
	const ReconstructHandle::type reconstruct_handle = reconstruct(
			reconstructed_features,
			reconstruction_time,
			anchor_plate_id,
			reconstructable_features_collection,
			reconstruction_features_collection,
			reconstruct_params,
			reconstruction_tree_cache_size);

	// Copy the RFGs in the ReconstructContext::ReconstructedFeature's.
	// The ReconstructContext::ReconstructedFeature's store RFGs and geometry property handles.
	// This format only needs the RFG.
	BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
	{
		const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructed_feature_reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructed_feature_reconstructions)
		{
			reconstructed_feature_geometries.push_back(
					reconstruction.get_reconstructed_feature_geometry());
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructContext::Reconstruction> &reconstructions,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const ReconstructParams &reconstruct_params,
		unsigned int reconstruction_tree_cache_size)
{
	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_features;
	const ReconstructHandle::type reconstruct_handle = reconstruct(
			reconstructed_features,
			reconstruction_time,
			anchor_plate_id,
			reconstructable_features_collection,
			reconstruction_features_collection,
			reconstruct_params,
			reconstruction_tree_cache_size);

	// Copy the ReconstructContext::Reconstruction's in the ReconstructContext::ReconstructedFeature's.
	BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
	{
		const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructed_feature_reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructed_feature_reconstructions)
		{
			reconstructions.push_back(reconstruction);
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const ReconstructParams &reconstruct_params,
		unsigned int reconstruction_tree_cache_size)
{
	ReconstructMethodRegistry reconstruct_method_registry;
	register_default_reconstruct_method_types(reconstruct_method_registry);

	ReconstructionTreeCreator reconstruction_tree_creator =
			create_cached_reconstruction_tree_creator(
					reconstruction_features_collection,
					anchor_plate_id,
					reconstruction_tree_cache_size);

	return reconstruct(
			reconstructed_features,
			reconstruction_time,
			reconstruct_method_registry,
			reconstructable_features_collection,
			reconstruction_tree_creator,
			reconstruct_params);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time,
		bool reverse_reconstruct)
{
	// Create the context in which to reconstruct.
	const ReconstructMethodInterface::Context reconstruct_method_context(
			reconstruct_params,
			reconstruction_tree_creator);

	// Find out how to reconstruct the geometry based on the feature containing the reconstruction properties.
	ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
			reconstruct_method_registry.create_reconstruct_method_or_default(
					reconstruction_properties,
					reconstruct_method_context);

	return reconstruct_method->reconstruct_geometry(
			geometry,
			reconstruct_method_context,
			reconstruction_time,
			reverse_reconstruct);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const ReconstructParams &reconstruct_params,
		bool reverse_reconstruct,
		unsigned int reconstruction_tree_cache_size)
{
	ReconstructionTreeCreator reconstruction_tree_creator =
			create_cached_reconstruction_tree_creator(
					reconstruction_features_collection,
					anchor_plate_id,
					reconstruction_tree_cache_size);

	ReconstructMethodRegistry reconstruct_method_registry;
	register_default_reconstruct_method_types(reconstruct_method_registry);

	return reconstruct_geometry(
			geometry,
			reconstruct_method_registry,
			reconstruction_properties,
			reconstruction_tree_creator,
			reconstruct_params,
			reconstruction_time,
			reverse_reconstruct);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
		const ReconstructionTree &reconstruction_tree,
		const ReconstructParams &reconstruct_params,
		bool reverse_reconstruct)
{
	return reconstruct_geometry(
			geometry,
			reconstruction_properties,
			reconstruction_tree.get_reconstruction_time(),
			reconstruction_tree.get_anchor_plate_id(),
			reconstruction_tree.get_reconstruction_features(),
			reconstruct_params,
			reverse_reconstruct);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct_by_plate_id(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::integer_plate_id_type reconstruction_plate_id,
		const ReconstructionTree &reconstruction_tree,
		bool reverse_reconstruct)
{
	ReconstructGeometryOnSphereByPlateId reconstruct_geom_on_sphere_by_plate_id(
			reconstruction_plate_id, reconstruction_tree, reverse_reconstruct);

	return reconstruct_geom_on_sphere_by_plate_id.reconstruct(geometry);
}

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct_as_half_stage(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::integer_plate_id_type left_plate_id,
		const GPlatesModel::integer_plate_id_type right_plate_id,
		const double &reconstruction_time,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &spreading_asymmetry,
		const double &half_stage_rotation_interval,
		bool reverse_reconstruct)
{
	ReconstructGeometryOnSphereWithHalfStage reconstruct_geom_on_sphere(
		left_plate_id,
		right_plate_id,
		reconstruction_time,
		reconstruction_tree_creator,
		spreading_asymmetry,
		half_stage_rotation_interval,
		reverse_reconstruct);

	return reconstruct_geom_on_sphere.reconstruct(geometry);
}
