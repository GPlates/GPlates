/* $Id: FlowlineUtils.h 8209 2010-04-27 14:24:11Z rwatson $ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date: 2010-04-27 16:24:11 +0200 (ti, 27 apr 2010) $
* 
* Copyright (C) 2009, 2010 Geological Survey of Norway.
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

#ifndef GPLATES_APPLOGIC_MOTIONTRACKUTILS_H
#define GPLATES_APPLOGIC_MOTIONTRACKUTILS_H

#include "app-logic/ReconstructionGeometryCollection.h"
#include "app-logic/ReconstructionTree.h"
#include "maths/PolylineOnSphere.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometryCollection;
	class ReconstructionTree;

	namespace MotionTrackUtils
	{

		/**
		* Used to obtain motion-track-relevant parameters from a motion track feature.
		*/
		class MotionTrackPropertyFinder :
			public GPlatesModel::ConstFeatureVisitor
		{
		public:
			MotionTrackPropertyFinder():
				d_has_geometry(false)
			{  }

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_reconstruction_plate_id() const
			{
				return d_reconstruction_plate_id;
			}

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_relative_plate_id() const
			{
				return d_relative_plate_id;
			}

			const std::vector<double> &
			get_times()
			{
				return d_times;
			}

			const std::vector<GPlatesMaths::FiniteRotation> &
			get_rotations() const
			{
				return d_rotations;
			}


			QString
			get_feature_info_string()
			{
				return d_feature_info;
			}

			QString
			get_name() 
			{
				return d_name;
			}

			bool
			has_geometry()
			{
				return d_has_geometry;
			}

			bool
			can_process_motion_track();

		private:
			virtual
			bool
			initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

			virtual
			void
			finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

			virtual
			void
			visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				d_has_geometry = true;
			}


			virtual
			void
			visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point)
			{
				d_has_geometry = true;
			}


			virtual
			void
			visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}

			virtual
			void
			visit_gpml_irregular_sampling(
				const gpml_irregular_sampling_type &gpml_irregular_sampling);

			virtual
			void
			visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

			bool d_has_geometry;

			boost::optional<GPlatesModel::integer_plate_id_type> d_reconstruction_plate_id;
			boost::optional<GPlatesModel::integer_plate_id_type> d_relative_plate_id;
			QString d_feature_info;
			QString d_name;

			// Times to be used in motion_track calculations. The first element in the current reconstruction time, and
			// subsequent elements go back in time. 
			// This vector is not necessarily identical to the motion_track input times.
			std::vector<double> d_times;

			// A vector of rotations used in motion track calculations. These are the stage poles from the current
			// reconstruction time to each of the other times in the @a d_times vector.
			//
			// For example, the first rotation is the stage pole from t0 to t1, for plates d_reconstruction_plate_id and d_relative_plate_id,
			// where t0 and t1 are the first two elements of @a d_times.
			//
			// The second element would be the stage pole from t0 to t2 and so on.
			std::vector<GPlatesMaths::FiniteRotation> d_rotations;

		};


		void
		calculate_motion_track(
			const GPlatesMaths::PointOnSphere &point,
			const MotionTrackPropertyFinder	 &motion_track_parameters,
			std::vector<GPlatesMaths::PointOnSphere> &motion_track,
			const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &tree,
			const std::vector<GPlatesMaths::FiniteRotation> &rotations);

	}

}

#endif  // GPLATES_APPLOGIC_MOTIONTRACKUTILS_H
