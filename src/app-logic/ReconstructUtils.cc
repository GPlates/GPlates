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

#include "ReconstructUtils.h"

#include "AppLogicUtils.h"
#include "ReconstructContext.h"
#include "ReconstructionTreePopulator.h"
#include "SmallCircleGeometryPopulator.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/types.h"


#include <boost/foreach.hpp>
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
				const GPlatesAppLogic::ReconstructionTree &recon_tree,
				bool reverse_reconstruct) :
		d_left_plate_id(left_plate_id),
		d_right_plate_id(right_plate_id),
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
					GPlatesAppLogic::ReconstructUtils::reconstruct_as_half_stage(
							multi_point_on_sphere,
							d_left_plate_id,
							d_right_plate_id,
							d_recon_tree,
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
							d_recon_tree,
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
							d_recon_tree,
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
							d_recon_tree,
							d_reverse_reconstruct).get();
		}

	private:
		const GPlatesModel::integer_plate_id_type d_left_plate_id;
		const GPlatesModel::integer_plate_id_type d_right_plate_id;
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
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructParams &reconstruct_params)
{
	// Create a reconstruct context - it will determine which reconstruct method each
	// reconstructable feature requires.
	ReconstructContext reconstruct_context(
			reconstruct_method_registry,
			reconstructable_features_collection);

	// Reconstruct the reconstructable features.
	return reconstruct_context.reconstruct(
			reconstructed_feature_geometries,
			reconstruct_params,
			reconstruction_tree_creator,
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
	ReconstructMethodRegistry reconstruct_method_registry;
	register_default_reconstruct_method_types(reconstruct_method_registry);

	ReconstructionTreeCreator reconstruction_tree_creator =
			get_cached_reconstruction_tree_creator(
					reconstruction_features_collection,
					reconstruction_time,
					anchor_plate_id,
					reconstruction_tree_cache_size);

	return reconstruct(
			reconstructed_feature_geometries,
			reconstruction_time,
			anchor_plate_id,
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
		const double &reconstruction_time,
		bool reverse_reconstruct)
{
	// Find out how to reconstruct the geometry based on the feature containing the reconstruction properties.
	const ReconstructMethod::Type reconstruct_method_type =
			reconstruct_method_registry.get_reconstruct_method_type_or_default(reconstruction_properties);

	// Get the reconstruct method so we can reconstruct (or reverse reconstruct) the geometry.
	ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
			reconstruct_method_registry.get_reconstruct_method(reconstruct_method_type);

	return reconstruct_method->reconstruct_geometry(
			geometry,
			reconstruction_properties,
			reconstruction_tree_creator,
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
		bool reverse_reconstruct,
		unsigned int reconstruction_tree_cache_size)
{
	ReconstructionTreeCreator reconstruction_tree_creator =
			get_cached_reconstruction_tree_creator(
					reconstruction_features_collection,
					reconstruction_time,
					anchor_plate_id,
					reconstruction_tree_cache_size);

	ReconstructMethodRegistry reconstruct_method_registry;
	register_default_reconstruct_method_types(reconstruct_method_registry);

	return reconstruct_geometry(
			geometry,
			reconstruct_method_registry,
			reconstruction_properties,
			reconstruction_tree_creator,
			reconstruction_time,
			reverse_reconstruct);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
		const ReconstructionTree &reconstruction_tree,
		bool reverse_reconstruct)
{
	return reconstruct_geometry(
			geometry,
			reconstruction_properties,
			reconstruction_tree.get_reconstruction_time(),
			reconstruction_tree.get_anchor_plate_id(),
			reconstruction_tree.get_reconstruction_features(),
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
	const ReconstructionTree &reconstruction_tree,
	bool reverse_reconstruct)
{
	ReconstructGeometryOnSphereWithHalfStage reconstruct_geom_on_sphere(
		left_plate_id, right_plate_id, reconstruction_tree, reverse_reconstruct);

	return reconstruct_geom_on_sphere.reconstruct(geometry);
}


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesAppLogic::ReconstructUtils::get_half_stage_rotation(
		const ReconstructionTree &reconstruction_tree,
		GPlatesModel::integer_plate_id_type left_plate_id,
		GPlatesModel::integer_plate_id_type right_plate_id)
{
	//
	// Rotation from present day (0Ma) to current reconstruction time 't' of mid-ocean ridge MOR
	// with left/right plate ids 'L' and 'R':
	//
	// R(0->t,A->MOR)
	// R(0->t,A->L) * R(0->t,L->MOR)
	// R(0->t,A->L) * Half[R(0->t,L->R)] // Assumes L->R spreading from 0->t1 *and* t1->t2
	// R(0->t,A->L) * Half[R(0->t,L->A) * R(0->t,A->R)]
	// R(0->t,A->L) * Half[inverse[R(0->t,A->L)] * R(0->t,A->R)]
	//
	// ...where 'A' is the anchor plate id.
	//

	using namespace GPlatesMaths;

	FiniteRotation right_rotation = reconstruction_tree.get_composed_absolute_rotation(right_plate_id).first;

	FiniteRotation left_rotation = reconstruction_tree.get_composed_absolute_rotation(left_plate_id).first;

	const FiniteRotation& r = compose(get_reverse(left_rotation), right_rotation);
 
	UnitQuaternion3D quat = r.unit_quat();

	if(!represents_identity_rotation(quat))
	{
		UnitQuaternion3D::RotationParams params = quat.get_rotation_params(r.axis_hint());
		real_t half_angle = 0.5 * params.angle;

		FiniteRotation half_rotation = 
			FiniteRotation::create(
			UnitQuaternion3D::create_rotation(
			params.axis, 
			half_angle),
			r.axis_hint());

		return compose(left_rotation, half_rotation);

	}
	else
	{
		return boost::none;
	}

	//
	// NOTE: The above algorithm works only if there is no motion of the right plate relative to
	// the left plate outside time intervals when ridge spreading is occurring because the algorithm
	// does not know when spreading is not occurring and just calculates the half-stage rotation
	// from the current reconstruction time back to present day (0Ma).
	//
	// The following example is for a mid-ocean ridge that only spreads between t1->t2:
	//
	// This assumes no spreading from 0->t1 and spreading from t1->t2...
	// R(0->t2,A->MOR)
	// R(0->t2,A->L) * R(0->t2,L->MOR)
	// R(0->t2,A->L) * R(t1->t2,L->MOR) * R(0->t1,L->MOR)
	// R(0->t2,A->L) * Half[R(t1->t2,L->R)] * R(0->t1,L->R) // No L->R spreading from 0->t1
	//
	// This (incorrectly) assumes spreading from 0->t2 (ie, both 0->t1 and t1->t2)...
	// R(0->t2,A->MOR)
	// R(0->t2,A->L) * R(0->t2,L->MOR)
	// R(0->t2,A->L) * Half[R(0->t2,L->R)] // Assumes L->R spreading from 0->t1 *and* t1->t2
	// R(0->t2,A->L) * Half[R(t1->t2,L->R) * R(0->t1,L->R)]
	//
	// Both equations above are only equivalent if there is no rotation between L and R from 0->t1.
	// Which happens if R(0->t1,L->R) is the identity rotation.
	// Note that the second equation is the one we actually use so it only works if the rotation
	// file has identity stage rotations between poles (of the MOR moving/fixed plate pair) where
	// no ridge spreading is occurring (ie, the two poles around an identity stage rotation are equal).
	//
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::ReconstructUtils::get_stage_pole(
	const GPlatesAppLogic::ReconstructionTree &reconstruction_tree_1,
	const GPlatesAppLogic::ReconstructionTree &reconstruction_tree_2,
	const GPlatesModel::integer_plate_id_type &moving_plate_id,
	const GPlatesModel::integer_plate_id_type &fixed_plate_id)
{
	//
	// Rotation from present day (0Ma) to time 't2' (via time 't1'):
	//
	// R(0->t2)  = R(t1->t2) * R(0->t1)
	// ...or by post-multiplying both sides by R(t1->0) this becomes...
	// R(t1->t2) = R(0->t2) * R(t1->0)
	//
	// Rotation from anchor plate 'A' to moving plate 'M' (via fixed plate 'F'):
	//
	// R(A->M) = R(A->F) * R(F->M)
	// ...or by pre-multiplying both sides by R(F->A) this becomes...
	// R(F->M) = R(F->A) * R(A->M)
	//
	// NOTE: The rotations for relative times and for relative plates have the opposite order of each other !
	// In other words:
	//   * For times 0->t1->t2 you apply the '0->t1' rotation first followed by the 't1->t2' rotation:
	//     R(0->t2)  = R(t1->t2) * R(0->t1)
	//   * For plate circuit A->F->M you apply the 'F->M' rotation first followed by the 'A->F' rotation:
	//     R(A->M) = R(A->F) * R(F->M)
	//     Note that this is not 'A->F' followed by 'F->M' as you might expect (looking at the time example).
	// This is probably best explained by the difference between thinking in terms of the grand fixed
	// coordinate system and local coordinate system (see http://glprogramming.com/red/chapter03.html#name2).
	// Essentially, in the plate circuit A->F->M, the 'F->M' rotation can be thought of as a rotation
	// within the local coordinate system of 'A->F'. In other words 'F->M' is not a rotation that
	// occurs relative to the global spin axis but a rotation relative to the local coordinate system
	// of plate 'F' *after* it has been rotated relative to the anchor plate 'A'.
	// For the times 0->t1->t2 this local/relative coordinate system concept does not apply.
	//
	// NOTE: A rotation must be relative to present day (0Ma) before it can be separated into
	// a (plate circuit) chain of moving/fixed plate pairs.
	// For example, the following is correct...
	//
	//    R(t1->t2,A->C)
	//       = R(0->t2,A->C) * R(t1->0,A->C)
	//       = R(0->t2,A->C) * inverse[R(0->t1,A->C)]
	//       // Now that all times are relative to 0Ma we can split A->C into A->B->C...
	//       = R(0->t2,A->B) * R(0->t2,B->C) * inverse[R(0->t1,A->B) * R(0->t1,B->C)]
	//       = R(0->t2,A->B) * R(0->t2,B->C) * inverse[R(0->t1,B->C)] * inverse[R(0->t1,A->B)]
	//
	// ...but the following is *incorrect*...
	//
	//    R(t1->t2,A->C)
	//       = R(t1->t2,A->B) * R(t1->t2,B->C)   // <-- This line is *incorrect*
	//       = R(0->t2,A->B) * R(t1->0,A->B) * R(0->t2,B->C) * R(t1->0,B->C)
	//       = R(0->t2,A->B) * inverse[R(0->t1,A->B)] * R(0->t2,B->C) * inverse[R(0->t1,B->C)]
	//
	// ...as can be seen above this gives two different results - the same four rotations are
	// present in each result but in a different order.
	// A->B->C means B->C is the rotation of C relative to B and A->B is the rotation of B relative to A.
	// The need for rotation A->C to be relative to present day (0Ma) before it can be split into
	// A->B and B->C is because A->B and B->C are defined (in the rotation file) as total reconstruction
	// poles which are always relative to present day.
	//
	//
	// So the stage rotation of moving plate relative to fixed plate and from time 't1' to time 't2':
	//
	// R(t1->t2,F->M)
	//    = R(0->t2,F->M) * R(t1->0,F->M)
	//    = R(0->t2,F->M) * inverse[R(0->t1,F->M)]
	//    = R(0->t2,F->A) * R(0->t2,A->M) * inverse[R(0->t1,F->A) * R(0->t1,A->M)]
	//    = inverse[R(0->t2,A->F)] * R(0->t2,A->M) * inverse[inverse[R(0->t1,A->F)] * R(0->t1,A->M)]
	//    = inverse[R(0->t2,A->F)] * R(0->t2,A->M) * inverse[R(0->t1,A->M)] * R(0->t1,A->F)
	//
	// ...where 'A' is the anchor plate, 'F' is the fixed plate and 'M' is the moving plate.
	//

	// For t1, get the rotation for plate M w.r.t. anchor	
	GPlatesMaths::FiniteRotation rot_0_to_t1_M = 
		reconstruction_tree_1.get_composed_absolute_rotation(moving_plate_id).first;

	// For t1, get the rotation for plate F w.r.t. anchor	
	GPlatesMaths::FiniteRotation rot_0_to_t1_F = 
		reconstruction_tree_1.get_composed_absolute_rotation(fixed_plate_id).first;	


	// For t2, get the rotation for plate M w.r.t. anchor	
	GPlatesMaths::FiniteRotation rot_0_to_t2_M = 
		reconstruction_tree_2.get_composed_absolute_rotation(moving_plate_id).first;

	// For t2, get the rotation for plate F w.r.t. anchor	
	GPlatesMaths::FiniteRotation rot_0_to_t2_F = 
		reconstruction_tree_2.get_composed_absolute_rotation(fixed_plate_id).first;	

	// Compose these rotations so that we get
	// the stage pole from time t1 to time t2 for plate M w.r.t. plate F.

	GPlatesMaths::FiniteRotation rot_t1 = 
		GPlatesMaths::compose(GPlatesMaths::get_reverse(rot_0_to_t1_F), rot_0_to_t1_M);

	GPlatesMaths::FiniteRotation rot_t2 = 
		GPlatesMaths::compose(GPlatesMaths::get_reverse(rot_0_to_t2_F), rot_0_to_t2_M);	

	GPlatesMaths::FiniteRotation stage_pole = 
		GPlatesMaths::compose(rot_t2,GPlatesMaths::get_reverse(rot_t1));	

	return stage_pole;	

}

void
GPlatesAppLogic::ReconstructUtils::display_rotation(
    const GPlatesMaths::FiniteRotation &rotation)

{
    if (represents_identity_rotation(rotation.unit_quat()))
    {
        qDebug() << "Identity rotation.";
        return;
    }

    using namespace GPlatesMaths;
    const UnitQuaternion3D old_uq = rotation.unit_quat();



    const boost::optional<GPlatesMaths::UnitVector3D> &axis_hint = rotation.axis_hint();
    UnitQuaternion3D::RotationParams params = old_uq.get_rotation_params(axis_hint);
    //real_t angle = params.angle;

    PointOnSphere point(params.axis);
    LatLonPoint llp = make_lat_lon_point(point);

    qDebug() << "Pole: Lat" << llp.latitude() << ", lon: " << llp.longitude() << ", angle: "
             << GPlatesMaths::convert_rad_to_deg(params.angle.dval());

}

