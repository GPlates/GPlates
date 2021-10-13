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

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/types.h"


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

			return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(d_reconstructed_geom.get());
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

	using namespace GPlatesMaths;

	FiniteRotation right_rotation = reconstruction_tree.get_composed_absolute_rotation(right_plate_id).first;

	FiniteRotation left_rotation = reconstruction_tree.get_composed_absolute_rotation(left_plate_id).first;

	const FiniteRotation& r = compose(left_rotation, get_reverse(right_rotation));
 
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


		return compose(half_rotation,right_rotation);

	}
	else
	{
		return boost::none;
	}
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::ReconstructUtils::get_stage_pole(
	const GPlatesAppLogic::ReconstructionTree &reconstruction_tree_1,
	const GPlatesAppLogic::ReconstructionTree &reconstruction_tree_2,
	const GPlatesModel::integer_plate_id_type &moving_plate_id,
	const GPlatesModel::integer_plate_id_type &fixed_plate_id)
{
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
		GPlatesMaths::compose(GPlatesMaths::get_reverse(rot_0_to_t1_F),rot_0_to_t1_M);

	GPlatesMaths::FiniteRotation rot_t2 = 
		GPlatesMaths::compose(GPlatesMaths::get_reverse(rot_0_to_t2_F),rot_0_to_t2_M);	

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
    real_t angle = params.angle;

    PointOnSphere point(params.axis);
    LatLonPoint llp = make_lat_lon_point(point);

    qDebug() << "Pole: Lat" << llp.latitude() << ", lon: " << llp.longitude() << ", angle: "
             << GPlatesMaths::convert_rad_to_deg(params.angle.dval());

}

