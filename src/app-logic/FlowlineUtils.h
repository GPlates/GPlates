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

#ifndef GPLATES_APPLOGIC_FLOWLINEUTILS_H
#define GPLATES_APPLOGIC_FLOWLINEUTILS_H

#include <functional>

#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/VelocityDeltaTime.h"

#include "maths/PolylineOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/PropertyName.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"


namespace GPlatesAppLogic
{
	namespace FlowlineUtils
	{
		/**
		* Determines if there are any flowline features in the collection.
		*/
		class DetectFlowlineFeatures:
			public GPlatesModel::ConstFeatureVisitor
		{
		public:
			DetectFlowlineFeatures() :
			  d_found_flowline_features(false)
			  {  }


			  bool
				  has_flowline_features() const
			  {
				  return d_found_flowline_features;
			  }


				virtual
				bool
				initialise_pre_feature_properties(
						feature_handle_type &feature_handle)
				{
					if (d_found_flowline_features)
					{
						// We've already found a flowline feature so just return.
						// NOTE: We don't actually want to visit the feature's properties.
						return false;
					}

					static const GPlatesModel::FeatureType flowline_feature_type =
					GPlatesModel::FeatureType::create_gpml("Flowline");

					if (feature_handle.feature_type() == flowline_feature_type)
					{
						d_found_flowline_features = true;
					}

					// NOTE: We don't actually want to visit the feature's properties.
					return false;
				}

		private:
			bool d_found_flowline_features;
		};

		/**
		 * Used to obtain flowline-relevant parameters from a flowline feature.
		 */
		class FlowlinePropertyFinder :
			public GPlatesModel::ConstFeatureVisitor
		{
		public:
			FlowlinePropertyFinder(
				const double &reconstruction_time):
				d_feature_is_defined_at_recon_time(true),
				d_has_geometry(false),
				d_reconstruction_time(
				    GPlatesPropertyValues::GeoTimeInstant(reconstruction_time))
			{  }

			FlowlinePropertyFinder():
				d_feature_is_defined_at_recon_time(true),
				d_has_geometry(false)
			{  }

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_reconstruction_plate_id() const
			{
				return d_reconstruction_plate_id;
			}

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_left_plate() const
			{
				return d_left_plate;
			}

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_right_plate() const
			{
				return d_right_plate;
			}

			const std::vector<double> &
			get_times()
			{
				return d_times;
			}

			/**
			 * Returns the "gpml:geometryImportTime" property, if found (and not distant past or future).
			 */
			const boost::optional<double> &
			get_geometry_import_time() const
			{
				return d_geometry_import_time;
			}

			/**
			 * Returns the time the flowline's spreading starts from: its geometry import time, or its
			 * youngest time if it has none (see FlowlineUtils::fill_seed_point_rotations).
			 *
			 * The times must not be empty.
			 */
			double
			get_spreading_start_time() const;

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

			/**
			 * Whether or not we should calculate flowlines for the
			 * current time.
			 */
			bool
			can_process_flowline();

			/**
			 * Whether or not we should display the seed point for the
			 * current time.
			 */
			bool
			can_process_seed_point();

			/**
			 * Whether or not we have enough info in the feature to
			 * perform a seed-point correction.
			 */
			bool
			can_correct_seed_point();

			/**
			 * Returns optional time of appearance if a "gml:validTime" property is found.
			 */
			const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
			get_time_of_appearance() const
			{
				return d_time_of_appearance;
			}


			/**
			 * Returns optional time of disappearance if a "gml:validTime" property is found.
			 */
			const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
			get_time_of_disappearance() const
			{
				return d_time_of_appearance;
			}

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
			visit_gpml_array(
				const GPlatesPropertyValues::GpmlArray &gpml_array);

			virtual
			void
			visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}

			virtual
			void
			visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

			virtual
			void
			visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

			virtual
			void
			visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

			virtual
			void
			visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);


			bool d_feature_is_defined_at_recon_time;
			bool d_has_geometry;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_reconstruction_time;
			boost::optional<GPlatesModel::integer_plate_id_type> d_reconstruction_plate_id;
			boost::optional<GPlatesModel::integer_plate_id_type> d_left_plate;
			boost::optional<GPlatesModel::integer_plate_id_type> d_right_plate;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_appearance;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_dissappearance;
			boost::optional<double> d_geometry_import_time;
			QString d_feature_info;
			QString d_name;

			// The GpmlArray<TimePeriod> times converted into a vector of doubles. 
			std::vector<double> d_times;

		};

		void
		calculate_flowline(
			const GPlatesMaths::PointOnSphere &seed_point,
			const FlowlinePropertyFinder &flowline_parameters,
			std::vector<GPlatesMaths::PointOnSphere> &flowline,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			const std::vector<GPlatesMaths::FiniteRotation> &rotations);

		/**
		 * Halves the angle of the provided FiniteRotation.
		 */
		void
		get_half_angle_rotation(
			GPlatesMaths::FiniteRotation &rotation);

		/**
		 * Fills @a times with @a reconstruction_time (clamped to the range of the sorted
		 * @a time_samples), followed by each time in @a time_samples older than it.
		 *
		 * For a flowline's own times, these are the times of the vertices of each half of the
		 * flowline reconstructed to @a reconstruction_time. Younger than the youngest time, each half
		 * starts at its youngest time, not at the seed point. At or older than the oldest, there is
		 * only the seed point.
		 */
		void
		fill_times_vector(
			std::vector<double> &times,
			const double &reconstruction_time,
			const std::vector<double> &time_samples);

		void
		get_times_from_time_period_array(
			std::vector<double> &times,
			const GPlatesPropertyValues::GpmlArray &array);

		GPlatesMaths::PointOnSphere
		reconstruct_seed_point(
			const GPlatesMaths::PointOnSphere &seed_point,
			const std::vector<GPlatesMaths::FiniteRotation> &rotations,
			bool reverse = false);

		/**
		 * Reconstructs (or reverse reconstructs) a flowline's seed point(s) by
		 * @a get_seed_point_rotation.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_flowline_seed_points(
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type seed_points,
			const double &current_time,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			const GPlatesModel::FeatureHandle::weak_ref &feature_handle,
			bool reverse = false);

		/**
		 * Fills @a flowline_rotations with the half-stage rotations that build one half of a flowline
		 * out from its seed point at @a reconstruction_time: one rotation per vertex, for each of
		 * @a flowline_times older than @a reconstruction_time, each from the previous vertex (the
		 * first from the seed point).
		 *
		 * The half is on @a fixed_plate_id, and spreads from @a moving_plate_id. When the flowline's
		 * @a spreading_start_time lies between two vertices it splits that step (its two half-stage
		 * rotations are composed into the one rotation), so that the vertices are where the spreading
		 * centre was at their times. That doesn't depend on the reconstruction time, unless a
		 * plate's stage pole changes between it and the next vertex: it ends a step too (see
		 * @a fill_seed_point_rotations).
		 *
		 * There are none at or older than the oldest of @a flowline_times.
		 */
		void
		fill_flowline_rotations(
			const double &reconstruction_time,
			const std::vector<double> &flowline_times,
			const double &spreading_start_time,
			const GPlatesModel::integer_plate_id_type &fixed_plate_id,
			const GPlatesModel::integer_plate_id_type &moving_plate_id,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			std::vector<GPlatesMaths::FiniteRotation> &flowline_rotations);

		/**
		 * Returns the rotation which transfers a flowline's seed point from the frame of the
		 * left plate into the frame of the right plate.
		 *
		 * A flowline's seed point is stored in the frame of its *left* plate - that is the frame
		 * @a reconstruct_flowline_seed_points reverse reconstructs it into. The right-hand half of
		 * the flowline is however built up in the frame of the *right* plate, so the seed point
		 * must be moved into that frame before it is used to seed the right-hand half.
		 *
		 * This is the rotation of the left plate relative to the right plate at
		 * @a flowline_start_time (the flowline's spreading start time, see
		 * FlowlinePropertyFinder::get_spreading_start_time, which is when the seed point is at the
		 * spreading centre in both plates' frames), ie R(0->flowline_start_time, right_plate->left_plate).
		 *
		 * It is the identity rotation whenever the two plates coincide at @a flowline_start_time - for
		 * example at present day, where rotations are usually the identity rotation. It is *not* the
		 * identity at a past time, or when a rotation file has been hand-edited so that present day is
		 * not the identity rotation.
		 */
		GPlatesMaths::FiniteRotation
		get_left_to_right_plate_frame_rotation(
			const double &flowline_start_time,
			const GPlatesModel::integer_plate_id_type &left_plate_id,
			const GPlatesModel::integer_plate_id_type &right_plate_id,
			const ReconstructionTreeCreator &reconstruction_tree_creator);

		/**
		 * Returns the rotation that reconstructs a flowline's (stored) seed point to
		 * @a reconstruction_time - the same rotation as @a reconstruct_flowline_seed_points.
		 *
		 * That is the left plate's rotation at @a reconstruction_time composed with the half-stage
		 * rotations of @a fill_seed_point_rotations.
		 */
		GPlatesMaths::FiniteRotation
		get_seed_point_rotation(
			const double &reconstruction_time,
			const std::vector<double> &flowline_times,
			const double &spreading_start_time,
			const GPlatesModel::integer_plate_id_type &left_plate_id,
			const GPlatesModel::integer_plate_id_type &right_plate_id,
			const ReconstructionTreeCreator &reconstruction_tree_creator);

		/**
		 * Returns the stage rotation over the velocity time range at @a reconstruction_time of a
		 * flowline's seed point, as reconstructed by @a get_seed_point_rotation.
		 *
		 * The velocity time range is that of
		 * PlateVelocityUtils::get_half_stage_rotation_velocity_time_range.
		 */
		GPlatesMaths::FiniteRotation
		calculate_seed_point_stage_rotation(
			const double &reconstruction_time,
			const std::vector<double> &flowline_times,
			const double &spreading_start_time,
			const GPlatesModel::integer_plate_id_type &left_plate_id,
			const GPlatesModel::integer_plate_id_type &right_plate_id,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			const double &velocity_delta_time,
			VelocityDeltaTime::Type velocity_delta_time_type);

		/**
		 * Fills @a seed_point_rotations with the half-stage rotations that move a flowline's seed
		 * point, in the left plate's frame, from where the spreading centre is at
		 * @a spreading_start_time to where it is at @a current_time, in steps between the
		 * @a flowline_times in between. Backward in time when @a current_time is older than
		 * @a spreading_start_time, forward when younger (as for 'HalfStageRotationVersion3' ridges).
		 *
		 * The spreading continues outside the flowline's times (younger than the youngest, older than
		 * the oldest), so the seed point is always where the spreading centre is. That needs rotations
		 * of both plates there; if they aren't spreading then, the flowline's valid time can hide it.
		 * There it takes one step from the nearest flowline time (or the spreading start time) to the
		 * current time. Each step uses only the rotations at its two ends, so a rotation sample inside
		 * it (where the stage pole changes) makes the flowline differ from one whose times stop there
		 * (eg, a 40-60 Ma piece viewed at 1 Ma, with samples at 10 and 20 Ma, was 85 km from a 10-60 Ma
		 * flowline digitised at the same time and point). The same happens inside the flowline's
		 * times, between two of them. Stepping at the rotation sample times would be exact.
		 * (Carrying the seed point by the left plate alone older than the oldest time would need only
		 * that plate's rotations there, but then changing the oldest time would shift it, and
		 * pieces of a flowline digitised older than their oldest time wouldn't join.) There are no
		 * rotations at the spreading start time (or when there are no flowline times).
		 *
		 * The spreading start time is the flowline's geometry import time, or its youngest time if
		 * it has none. So it doesn't depend on the flowline's times (if it has an import time), and
		 * neither does the flowline: changing its times only changes which vertices are drawn.
		 * The seed point only moves with the left plate between present day and the spreading start
		 * time, so rotations younger than that time don't change where it is then (as for
		 * 'HalfStageRotationVersion3' ridges).
		 */
		void
		fill_seed_point_rotations(
			const double &current_time,
			const std::vector<double> &flowline_times,
			const double &spreading_start_time,
			const GPlatesModel::integer_plate_id_type &left_plate_id,
			const GPlatesModel::integer_plate_id_type &right_plate_id,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			std::vector<GPlatesMaths::FiniteRotation> &seed_point_rotations);

		/**
		 * Given a flowline end point(s) @a geometry_ at time @a reconstruction_time,
		 * calculates the spreading centre for that flowline.
		 *
		 * @a plate_1 is the plate of the half of the flowline the end point is on, and
		 * @a plate_2 the other plate.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		correct_end_point_to_centre(
		    GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_,
		    const GPlatesModel::integer_plate_id_type &plate_1,
		    const GPlatesModel::integer_plate_id_type &plate_2,
		    const std::vector<double> &times,
		    const double &spreading_start_time,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
		    const double &reconstruction_time);

		/**
		 * Returns the youngest time of @a feature_ref, if it is a flowline with times and no
		 * "gpml:geometryImportTime" (see @a set_default_geometry_import_time).
		 */
		boost::optional<double>
		get_default_geometry_import_time(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref);

		/**
		 * Gives the flowline @a feature_ref a "gpml:geometryImportTime" at its youngest time, if it is
		 * a flowline with times and no geometry import time.
		 *
		 * That is where its spreading already starts from (see
		 * FlowlinePropertyFinder::get_spreading_start_time), so it isn't shifted (at any
		 * reconstruction time), but then it isn't shifted when its times change either. Returns
		 * true if the property was added.
		 */
		bool
		set_default_geometry_import_time(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref);

		/**
		 * Edits the property @a property_name of @a feature_ref by calling @a edit_property, which
		 * returns whether it modified the property, and returns that.
		 *
		 * If @a feature_ref is a flowline without a "gpml:geometryImportTime", it spreads from its
		 * youngest time, so an edit of its "gpml:times" would shift it (at every reconstruction
		 * time). So if the edit modifies them, the flowline is then given a geometry import time at
		 * its youngest time *before* the edit (see @a get_default_geometry_import_time), and stays
		 * where it was. For any other feature, property or edit, this only calls @a edit_property.
		 *
		 * Edits that don't come through here (or pyGPlates' Feature.set_times, which does the same)
		 * still shift such a flowline: pyGPlates' Feature.set with "gpml:times", modifying the
		 * GpmlArray of times in place, or deleting the times in GPlates and adding them again with
		 * the Add Property dialog. They are left documented rather than covered.
		 *
		 * The property is added after the edit, so it can't disturb whatever is doing the edit (eg,
		 * a property edit widget that is still committing its value).
		 */
		bool
		edit_property_without_shifting_flowline(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const GPlatesModel::PropertyName &property_name,
			const std::function<bool ()> &edit_property);

	}

}


#endif // GPLATES_APPLOGIC_FLOWLINEUTILS_H

