/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010, 2011 Geological Survey of Norway
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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTEDMOTIONPATH_H
#define GPLATES_APP_LOGIC_RECONSTRUCTEDMOTIONPATH_H

#include "ReconstructedFeatureGeometry.h"
#include "maths/PolylineOnSphere.h"



namespace GPlatesAppLogic
{

	/**
	 * A reconstructed motion track. 
	 *
	 * Should this be minus the seed point? (which is a @a ReconstructedFeatureGeometry)?
	 *
	 */
	class ReconstructedMotionPath :
			public ReconstructedFeatureGeometry
	{
	public:
		/**
		 * A convenience typedef for a non-null shared pointer to a non-const @a ReconstructedMotionPath.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedMotionPath>
				non_null_ptr_type;

		/**
		 * A convenience typedef for a non-null shared pointer to a const @a ReconstructedMotionPath.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedMotionPath>
				non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ReconstructedMotionPath>.
		 */
		typedef boost::intrusive_ptr<ReconstructedMotionPath> maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const ReconstructedMotionPath>.
		 */
		typedef boost::intrusive_ptr<const ReconstructedMotionPath> maybe_null_ptr_to_const_type;


		/** 
		 * A convenience typedef for a PointOnSphere. 
		 */
		typedef GPlatesMaths::PointOnSphere seed_point_type;

		/** 
		 * A convenience typedef for a GeometryOnSphere::non_null_ptr_to_const type. 
		 */
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type feature_geom_ptr_type;

		/** 
		 * A convenience typedef for a PointOnSphere::non_null_ptr_to_const type. 
		 */
		typedef GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type motion_path_geom_ptr_type;

		/**
		 * Create a ReconstructedMotionPath instance with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const seed_point_type &present_day_seed_point,
				const seed_point_type &reconstructed_seed_point,
				const motion_path_geom_ptr_type &motion_path_points,
				const GPlatesModel::integer_plate_id_type &reconstruction_plate_id,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator,
				// All reconstructed seed points (not just the one referenced by this ReconstructedMotionPath).
				// This is the reconstructed geometry in the base RFG class.
				// It needs to be *all* seed points otherwise the geometry modification tools (eg, MoveVertex) won't work...
				const feature_geom_ptr_type &reconstructed_geometry_)
		{
			return non_null_ptr_type(
					new ReconstructedMotionPath(
							reconstruction_tree,
							reconstruction_tree_creator,
							present_day_seed_point,
							reconstructed_seed_point,
							motion_path_points,
							reconstruction_plate_id,
							feature_handle,
							property_iterator,
							reconstructed_geometry_));
		}

		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor);


		motion_path_geom_ptr_type
		motion_path_points() const
		{
			return d_motion_path_points;
		}

		const seed_point_type &
		present_day_seed_point() const
		{
			return d_present_day_seed_point;
		}

		/**
		 * The reconstructed version of @a present_day_seed_point.
		 */
		const seed_point_type &
		reconstructed_seed_point() const
		{
			return d_reconstructed_seed_point;
		}

	private:
		/**
		 * Instantiate a reconstructed motion path.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedMotionPath(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const seed_point_type &present_day_seed_point,
				const seed_point_type &reconstructed_seed_point,
				const motion_path_geom_ptr_type &motion_path_points_,
				const GPlatesModel::integer_plate_id_type &reconstruction_plate_id_,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator,
				const feature_geom_ptr_type &reconstructed_geometry_):
			ReconstructedFeatureGeometry(
				reconstruction_tree_,
				reconstruction_tree_creator,
				feature_handle,
				property_iterator,
				reconstructed_geometry_,
				ReconstructMethod::MOTION_PATH,
				reconstruction_plate_id_,
				boost::none),
			d_present_day_seed_point(present_day_seed_point),
			d_reconstructed_seed_point(reconstructed_seed_point),
			d_motion_path_points(motion_path_points_)
		{  }

		GPlatesMaths::PointOnSphere d_present_day_seed_point;
		GPlatesMaths::PointOnSphere d_reconstructed_seed_point;
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type d_motion_path_points;

	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTEDMOTIONPATH_H
