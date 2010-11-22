/* $Id: ReconstructedVirtualGeomagneticPole.h 9024 2010-07-30 10:47:35Z elau $ */


/**
 * \file 
 * $Revision: 9024 $
 * $Date: 2010-07-30 12:47:35 +0200 (fr, 30 jul 2010) $
 * 
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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTEDMOTIONTRACK_H
#define GPLATES_APP_LOGIC_RECONSTRUCTEDMOTIONTRACK_H

#include "ReconstructionGeometry.h"
#include "maths/PolylineOnSphere.h"
#include "model/WeakObserver.h"

namespace GPlatesAppLogic
{

	/**
	 * A reconstructed motion track. 
	 *
	 * Should this be minus the seed point? (which is a @a ReconstructedFeatureGeometry)?
	 *
	 */
	class ReconstructedMotionTrack :
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		/**
		 * A convenience typedef for a non-null shared pointer to a non-const @a ReconstructedMotionTrack.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedMotionTrack>
				non_null_ptr_type;

		/**
		 * A convenience typedef for a non-null shared pointer to a const @a ReconstructedMotionTrack.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedMotionTrack>
				non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ReconstructedMotionTrack>.
		 */
		typedef boost::intrusive_ptr<ReconstructedMotionTrack> maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const ReconstructedMotionTrack>.
		 */
		typedef boost::intrusive_ptr<const ReconstructedMotionTrack> maybe_null_ptr_to_const_type;

		/**
		 * A convenience typedef for the WeakObserver base class of this class.
		 */
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;


		/** 
		 * A convenience typedef for a PointOnSphere::non_null_ptr_to_const type. 
		 */
		typedef GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type seed_point_geom_ptr_type;

		/** 
		 * A convenience typedef for a PointOnSphere::non_null_ptr_to_const type. 
		 */
		typedef GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type motion_track_geom_ptr_type;

		/**
		 * Create a ReconstructedMotionTrack instance with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const seed_point_geom_ptr_type &present_day_seed_point_geometry_ptr,
				const motion_track_geom_ptr_type &motion_track_points,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator)
		{
			return non_null_ptr_type(
					new ReconstructedMotionTrack(
							reconstruction_tree,
							present_day_seed_point_geometry_ptr,
							motion_track_points,
							feature_handle,
							property_iterator),
					GPlatesUtils::NullIntrusivePointerHandler());
		}


		/**
		 * Return whether this RFG references @a that_feature_handle.
		 *
		 * This function will not throw.
		 */
		bool
		references(
				const GPlatesModel::FeatureHandle &that_feature_handle) const
		{
			return (feature_handle_ptr() == &that_feature_handle);
		}

		/**
		 * Return the pointer to the FeatureHandle.
		 *
		 * The pointer returned will be NULL if this instance does not reference a
		 * FeatureHandle; non-NULL otherwise.
		 *
		 * This function will not throw.
		 */
		GPlatesModel::FeatureHandle *
		feature_handle_ptr() const
		{
			return WeakObserverType::publisher_ptr();
		}


		/**
		 * Return whether this pointer is valid to be dereferenced (to obtain a
		 * FeatureHandle).
		 *
		 * This function will not throw.
		 */
		bool
		is_valid() const
		{
			return (feature_handle_ptr() != NULL);
		}

		/**
		 * Return a weak-ref to the feature whose reconstructed geometry this RFG contains,
		 * or an invalid weak-ref, if this pointer is not valid to be dereferenced.
		 */
		const GPlatesModel::FeatureHandle::weak_ref
		get_feature_ref() const;

		/**
		 * Access the feature property which contained the reconstructed geometry.
		 */
		const GPlatesModel::FeatureHandle::iterator
		property() const
		{
			return d_property_iterator;
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


		motion_track_geom_ptr_type
		motion_track_points() const
		{
			return d_motion_track_points;
		}

		seed_point_geom_ptr_type
		seed_point() const
		{
			return d_present_day_seed_point;
		}

	private:
		/**
		 * Instantiate a reconstructed motion_track.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedMotionTrack(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const seed_point_geom_ptr_type &present_day_seed_point,
				const motion_track_geom_ptr_type &motion_track_points_,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator):
			ReconstructionGeometry(reconstruction_tree_),
			WeakObserverType(feature_handle),
			d_present_day_seed_point(present_day_seed_point),
			d_motion_track_points(motion_track_points_)
		{  }

		/**
		 * This is an iterator to the flowline seed point from which this reconstructed flowline
		 * was derived.
		 */
		GPlatesModel::FeatureHandle::iterator d_property_iterator;
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type d_present_day_seed_point;
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type d_motion_track_points;

	
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTEDMOTIONTRACK_H
