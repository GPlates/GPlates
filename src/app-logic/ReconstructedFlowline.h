/* $Id: ReconstructedFlowline.h 9024 2010-07-30 10:47:35Z elau $ */


/**
 * \file 
 * $Revision: 9024 $
 * $Date: 2010-07-30 12:47:35 +0200 (fr, 30 jul 2010) $
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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTEDFLOWLINE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTEDFLOWLINE_H

#include "ReconstructedFeatureGeometry.h"
#include "maths/PolylineOnSphere.h"

namespace GPlatesAppLogic
{

	class ReconstructedFlowline :
			public ReconstructedFeatureGeometry
	{
	public:
		/**
		 * A convenience typedef for a non-null shared pointer to a non-const @a ReconstructedFlowline.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedFlowline>
				non_null_ptr_type;

		/**
		 * A convenience typedef for a non-null shared pointer to a const @a ReconstructedFlowline.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedFlowline>
				non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ReconstructedFlowline>.
		 */
		typedef boost::intrusive_ptr<ReconstructedFlowline> maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const ReconstructedFlowline>.
		 */
		typedef boost::intrusive_ptr<const ReconstructedFlowline> maybe_null_ptr_to_const_type;


		/** 
		 * A convenience typedef for a PointOnSphere::non_null_ptr_to_const type. 
		 */
		typedef GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type seed_point_geom_ptr_type;

		/** 
		 * A convenience typedef for a GeometryOnSphere::non_null_ptr_to_const type. 
		 */
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type feature_geom_ptr_type;

		/** 
		 * A convenience typedef for a PointOnSphere::non_null_ptr_to_const type. 
		 */
		typedef GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type flowline_geom_ptr_type;

		/**
		 * Create a ReconstructedFlowline instance with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const seed_point_geom_ptr_type &present_day_seed_point_geometry_ptr,
				const seed_point_geom_ptr_type &reconstructed_seed_point_geometry_ptr,
				const flowline_geom_ptr_type &left_flowline_points,
				const flowline_geom_ptr_type &right_flowline_points,
				const GPlatesModel::integer_plate_id_type &left_plate_id,
				const GPlatesModel::integer_plate_id_type &right_plate_id,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator)
		{
			return non_null_ptr_type(
					new ReconstructedFlowline(
							reconstruction_tree,
							reconstruction_tree_creator,
							present_day_seed_point_geometry_ptr,
							reconstructed_seed_point_geometry_ptr,
							left_flowline_points,
							right_flowline_points,
							left_plate_id,
							right_plate_id,
							feature_handle,
							property_iterator),
							GPlatesUtils::NullIntrusivePointerHandler());
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


		flowline_geom_ptr_type
		left_flowline_points() const
		{
			return d_left_flowline_points;
		}

		flowline_geom_ptr_type
		right_flowline_points() const
		{
			return d_right_flowline_points;
		}

		seed_point_geom_ptr_type
		present_day_seed_point() const
		{
			return d_present_day_seed_point;
		}

		/**
		 * The reconstructed version of @a present_day_seed_point.
		 */
		seed_point_geom_ptr_type
		reconstructed_seed_point() const
		{
			return d_reconstructed_seed_point;
		}

		GPlatesModel::integer_plate_id_type
		left_plate_id() const
		{
			return d_left_plate_id;
		}

		GPlatesModel::integer_plate_id_type
		right_plate_id() const
		{
			return d_right_plate_id;
		}

	private:
		/**
		 * Instantiate a reconstructed flowline.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedFlowline(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const seed_point_geom_ptr_type &present_day_seed_point_geometry_ptr,
				const seed_point_geom_ptr_type &reconstructed_seed_point_geometry_ptr,
				const flowline_geom_ptr_type &left_flowline_points_,
				const flowline_geom_ptr_type &right_flowline_points_,
				const GPlatesModel::integer_plate_id_type &left_plate_id_,
				const GPlatesModel::integer_plate_id_type &right_plate_id_,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator):
			ReconstructedFeatureGeometry(
				reconstruction_tree_,
				reconstruction_tree_creator,
				feature_handle,
				property_iterator,
				reconstructed_seed_point_geometry_ptr,
				ReconstructMethod::FLOWLINE,
				boost::none,
				boost::none,
				boost::none),
			d_present_day_seed_point(present_day_seed_point_geometry_ptr),
			d_reconstructed_seed_point(reconstructed_seed_point_geometry_ptr),
			d_left_flowline_points(left_flowline_points_),
			d_right_flowline_points(right_flowline_points_),
			d_left_plate_id(left_plate_id_),
			d_right_plate_id(right_plate_id_)
		{  }

		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type d_present_day_seed_point;
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type d_reconstructed_seed_point;
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type d_left_flowline_points;
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type d_right_flowline_points;

		// Left/Right plate ids are here purely for colouring.
		GPlatesModel::integer_plate_id_type d_left_plate_id;
		GPlatesModel::integer_plate_id_type d_right_plate_id;
	};

}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTEDFLOWLINE_H
