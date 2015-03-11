/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_GEOMETRYDEFORMATION_H
#define GPLATES_APP_LOGIC_GEOMETRYDEFORMATION_H

#include <deque>
#include <list>
#include <vector>
#include <boost/optional.hpp>

#include "ReconstructionTreeCreator.h"
#include "ResolvedTopologicalNetwork.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/GeometryOnSphere.h"
#include "maths/GeometryType.h"
#include "maths/PointOnSphere.h"
#include "maths/types.h"
#include "maths/Vector3D.h"

#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometry;

	namespace GeometryDeformation
	{
		/**
		 * A look up table of resolved topological networks over a time span.
		 */
		class ResolvedNetworkTimeSpan
		{
		public:

			//! Typedef for a sequence of resolved topological networks.
			typedef std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> rtn_seq_type;


			/**
			 * @throws exception if the following is not satisfied:
			 *   begin_time > end_time && time_increment > 0
			 */
			ResolvedNetworkTimeSpan(
					const double &begin_time,
					const double &end_time,
					const double &time_increment);


			/**
			 * Returns the begin time passed into constructor.
			 *
			 * NOTE: This is rounded up to the nearest time slot and hence can be earlier in the past
			 * than the begin time passed into the constructor.
			 *
			 * Essentially: begin_time = end_time + num_time_slots * time_increment
			 */
			const double &
			get_begin_time() const
			{
				return d_begin_time;
			}


			//! Returns the end time passed into constructor.
			const double &
			get_end_time() const
			{
				return d_end_time;
			}


			//! Returns the time increment passed into constructor.
			const double &
			get_time_increment() const
			{
				return d_time_increment;
			}


			//! The number of time slots across the time span.
			unsigned int
			get_num_time_slots() const
			{
				return d_resolved_networks_time_sequence.size();
			}


			/**
			 * Returns the time associated with the specified time slot.
			 *
			 * Time slots begin at the begin time and end at the end time.
			 */
			double
			get_time(
					unsigned int time_slot) const
			{
				return d_begin_time - time_slot * d_time_increment;
			}


			//! Returns the matching time slot if the specified time matches (within epsilon) the time of a time slot.
			boost::optional<unsigned int>
			get_time_slot(
					const double &time) const;


			//! Get the resolved topological networks for the specified time slot.
			const rtn_seq_type &
			get_resolved_networks_time_slot(
					unsigned int time_slot) const;


			/**
			 * Get the nearest resolved topological networks time slot for the specified time.
			 *
			 * Returns boost::none if @a time is outside the range [@a begin_time, @a end_time].
			 */
			boost::optional<const rtn_seq_type &>
			get_nearest_resolved_networks_time_slot(
					const double &time) const;


			//! Add resolved topological networks for the specified time slot.
			template <typename ResolvedTopologicalNetworkIter>
			void
			add_resolved_networks(
					ResolvedTopologicalNetworkIter resolved_topological_networks_begin,
					ResolvedTopologicalNetworkIter resolved_topological_networks_end,
					unsigned int time_slot)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						time_slot < d_resolved_networks_time_sequence.size(),
						GPLATES_ASSERTION_SOURCE);

				rtn_seq_type &rtn_time_slot = d_resolved_networks_time_sequence[time_slot];

				rtn_time_slot.insert(
						rtn_time_slot.end(),
						resolved_topological_networks_begin,
						resolved_topological_networks_end);
			}

		private:

			//! Typedef for a time sequence of resolved topological networks.
			typedef std::vector<rtn_seq_type> rtn_time_seq_type;


			double d_begin_time;
			double d_end_time;
			double d_time_increment;

			rtn_time_seq_type d_resolved_networks_time_sequence;


			/**
			 * Returns the number of time slots ending at @a end_time and beginning at, or up to
			 * one time increment before, @a begin_time.
			 */
			unsigned int
			calc_num_time_slots() const;
		};


		/**
		 * Deformation information for a point in a deformed geometry.
		 */
		struct DeformationInfo
		{
			DeformationInfo() :
				dilitation(0), // DEF_TEST
				sph_dilitation(0), // DEF_TEST
				SR22(0),
				SR33(0),
				SR23(0),
				S22(0),
				S33(0),
				S23(0),
				S_DIR(0),
				S1(0),
				S2(0),
				dilitation_strain(0),
				second_invariant_strain(0)
			{  }

			//
			// Instantaneous values (not accumulated over time).
			//

			// Instantaneous strain rates.
			double dilitation; // DEF_TEST
			double sph_dilitation; // DEF_TEST
			double SR22; // 2 = x
			double SR33; // 3 = y
			double SR23;

			//
			// Accumulated values (accumulated forward in time).
			//

			// Accumulated strain data going forward in time.
			double S22;
			double S33;
			double S23;

			double S_DIR; // principle strain direction
			double S1; 	// principle strain
			double S2; 	// principle strain
			double dilitation_strain;
			double second_invariant_strain;
		};


		/**
		 * Builds and keeps track of a geometry over a time span.
		 *
		 * Outside the time span the geometry is rigidly reconstructed.
		 * Inside the time span the geometry can be alternately deformed and rigidly rotated
		 * depending on whether it intersects any resolved topological networks at various times.
		 */
		class GeometryTimeSpan :
				public GPlatesUtils::ReferenceCount<GeometryTimeSpan>
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a GeometryTimeSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<GeometryTimeSpan> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a GeometryTimeSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<const GeometryTimeSpan> non_null_ptr_to_const_type;


			/**
			 * Creates a time span for the specified present day geometry.
			 *
			 * The resolved network time span is used to deform the geometry within its time range.
			 * Outside that time range the geometry is rigidly reconstructed using the specified
			 * reconstruction plate id. Although within that time range the geometry can be rigidly
			 * reconstructed if it does not intersect any resolved networks at specific times).
			 *
			 * NOTE: If the feature does not exist for the entire time span we still deform it.
			 * This is an issue to do with storing feature geometry in present day coordinates.
			 * We need to be able to change the feature's end time without having it change the position
			 * of the feature's deformed geometry prior to the feature's end (disappearance) time.
			 * Changing the feature's begin/end time then only changes the time window within which
			 * the feature is visible (and generates ReconstructedFeatureGeoemtry's).
			 */
			static
			non_null_ptr_type
			create(
					const ResolvedNetworkTimeSpan &resolved_network_time_span,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &feature_present_day_geometry,
					GPlatesModel::integer_plate_id_type feature_reconstruction_plate_id)
			{
				return non_null_ptr_type(
						new GeometryTimeSpan(
								resolved_network_time_span,
								reconstruction_tree_creator,
								feature_present_day_geometry,
								feature_reconstruction_plate_id));
			}

			/**
			 * Returns the deformed geometry at the specified time (which can be any time within the
			 * valid time period of the geometry's feature - it's up to the caller to check that).
			 *
			 * If there is no deformation of the geometry at @a reconstruction_time, or
			 * @a reconstruction_time is outside the resolved network time span, then the geometry
			 * is reconstructed (by its plate id) using a rigid rotation.
			 */
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
			get_geometry(
					const double &reconstruction_time,
					const ReconstructionTreeCreator &reconstruction_tree_creator);

			/**
			 * Same as @a get_geometry but also returns deformation information for each point
			 * the the deformed geometry.
			 */
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
			get_geometry_and_deformation_information(
					const double &reconstruction_time,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					std::vector<DeformationInfo> &deformation_info_points);

			/**
			 * Returns deformation information for each point in the deformed geometry at the
			 * specified time (which can be any time within the valid time period of the geometry's
			 * feature - it's up to the caller to check that).
			 */
			void
			get_deformation_information(
					std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
					std::vector<DeformationInfo> &deformation_info_points,
					const double &reconstruction_time,
					const ReconstructionTreeCreator &reconstruction_tree_creator);

			/**
			 * Calculate velocities at the geometry (domain) points at the specified time (which can
			 * be any time within the valid time period of the geometry's feature - it's up to the
			 * caller to check that).
			 *
			 * @a surfaces contains the resolved network (or network interior rigid block) that
			 * each domain point intersects (if any).
			 *
			 * The sizes of @a domain_points, @a velocities and @a surfaces are the same.
			 */
			void
			get_velocities(
					std::vector<GPlatesMaths::PointOnSphere> &domain_points,
					std::vector<GPlatesMaths::Vector3D> &velocities,
					std::vector< boost::optional<const ReconstructionGeometry *> > &surfaces,
					const double &reconstruction_time,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const ResolvedNetworkTimeSpan &resolved_network_time_span);

		private:

			// Forward declaration.
			class TimeWindow;

			/**
			 * A geometry snapshot consisting of geometry points and associated per-point info.
			 */
			class GeometrySample
			{
			public:

				//! Typedef for an optional delaunay face.
				typedef boost::optional<ResolvedTriangulation::Delaunay_2::Face_handle> delaunay_face_opt_type;

				//! Typedef for a sequence of optional delaunay faces.
				typedef std::vector<delaunay_face_opt_type> delaunay_face_opt_seq_type;


				GeometrySample() :
					d_have_initialised_instantaneous_deformation_information(false)
				{  }


				std::vector<GPlatesMaths::PointOnSphere> points;


				const std::vector<DeformationInfo> &
				get_deformation_infos() const
				{
					if (!d_have_initialised_instantaneous_deformation_information)
					{
						calc_instantaneous_deformation_information();
					}
					return d_deformation_infos;
				}

				std::vector<DeformationInfo> &
				get_deformation_infos()
				{
					if (!d_have_initialised_instantaneous_deformation_information)
					{
						calc_instantaneous_deformation_information();
					}
					return d_deformation_infos;
				}

				/**
				 * Set the delaunay face (if any) that each geometry point lies within.
				 *
				 * If this is not called then all geometry points lie outside deforming regions -
				 * note that interior rigid blocks are outside deforming regions.
				 */
				void
				set_delaunay_faces(
						const delaunay_face_opt_seq_type &delaunay_faces)
				{
					d_delaunay_faces = delaunay_faces;
				}

				/**
				 * Get the point list
				 */
				std::vector<GPlatesMaths::PointOnSphere> &
				get_points()
				{
					return points;
				}

			private:
				mutable std::vector<DeformationInfo> d_deformation_infos;
				mutable bool d_have_initialised_instantaneous_deformation_information;

				//! boost::none means all geometry points lie outside the delaunay triangulation.
				boost::optional<delaunay_face_opt_seq_type> d_delaunay_faces;

				//! Calculate instantaneous deformation values but not forward-time-accumulated values.
				void
				calc_instantaneous_deformation_information() const;

				friend class TimeWindow;
			};

			/**
			 * A time window containing a contiguous time span of geometry samples
			 * (typically deformed geometries).
			 *
			 * Time windows can be separated by time periods where only rigid rotations occur.
			 * In other words time windows do not have to abut (or touch) each other.
			 *
			 * Each time window has only an end time (and not a start time).
			 * This is because the end time of the earlier (less recent) time window implicitly
			 * forms the begin time of the next time window.
			 */
			class TimeWindow
			{
			public:

				TimeWindow(
						const double &end_time,
						const double &time_increment) :
					d_end_time(end_time),
					d_time_increment(time_increment)
				{  }

				//
				// Set methods...
				//

				/**
				 * Adds a geometry sample to the front (least recent time) of the stored geometry samples.
				 *
				 * Note that the first geometry sample added is associated with this time window's end time.
				 * And hence at least one geometry sample should be added.
				 */
				GeometrySample &
				add_geometry_sample(
						const std::vector<GPlatesMaths::PointOnSphere> &geometry_points);

				//
				// Get methods...
				//

				/**
				 * The begin time of this time window.
				 *
				 * Note that if the time window has only one sample (the minimum allowed) then
				 * the begin and end times of this time window will be equal.
				 */
				double
				get_begin_time() const;

				/**
				 * The end time of this time window.
				 */
				const double &
				get_end_time() const
				{
					return d_end_time;
				}

				/**
				 * Returns the number of geometry samples (which is one more than the number of intervals).
				 */
				unsigned int
				get_num_geometry_samples() const
				{
					return d_geometry_sample_time_span.size();
				}

				/**
				 * Returns the geometry sample at the specified index.
				 *
				 * Increasing indices move forward in time (from earlier to recent times).
				 *
				 * Throws exception if @a geometry_sample_index is not less than @a get_num_geometry_samples.
				 */
				GeometrySample &
				get_geometry_sample(
						unsigned int geometry_sample_index);

				/**
				 * Returns the geometry sample at the specified reconstruction time.
				 *
				 * Throws exception if @a reconstruction_time is outside the time window's begin/end range.
				 */
				const GeometrySample &
				get_geometry_sample(
						const double &reconstruction_time) const
				{
					return d_geometry_sample_time_span[get_geometry_sample_index(reconstruction_time)];
				}

				/**
				 * Returns the geometry sample at the beginning of this time window.
				 */
				const GeometrySample &
				get_geometry_sample_at_begin_time() const;

				//! Non-const overload.
				GeometrySample &
				get_geometry_sample_at_begin_time();

			private:

				/**
				 * Typedef for a sequence of geometry samples over a contiguous sequence of time slots.
				 *
				 * This is a deque so that we can efficiently add to the front since doing so does not
				 * move geometry samples in memory. And we can still access using indices.
				 */
				typedef std::deque<GeometrySample> geometry_sample_time_span_type;


				//! End time of this time window.
				double d_end_time;

				//! The size of a time interval.
				double d_time_increment;

				/**
				 * The geometry samples ordered least recent to most recent with the last sample
				 * associated with the end time.
				 */
				geometry_sample_time_span_type d_geometry_sample_time_span;

				/**
				 * Returns the nearest geometry sample index for the specified reconstruction time.
				 *
				 * Throws exception if @a reconstruction_time is outside the time window's begin/end range.
				 */
				unsigned int
				get_geometry_sample_index(
						const double &reconstruction_time) const;
			};

			/**
			 * Typedef for a sequence of time windows.
			 *
			 * This is a list so that we can efficiently add to the front because doing so
			 * doesn't move existing windows in memory.
			 */
			typedef std::list<TimeWindow> time_window_seq_type;


			GPlatesModel::integer_plate_id_type d_reconstruction_plate_id;

			GPlatesMaths::GeometryType::Value d_geometry_type;

			time_window_seq_type d_time_windows;

			/**
			 * Is true if we've generated the deformation information that is accumulated
			 * going forward in time.
			 */
			bool d_have_initialised_forward_time_accumulated_deformation_information;


			GeometryTimeSpan(
					const ResolvedNetworkTimeSpan &resolved_network_time_span,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &feature_present_day_geometry,
					GPlatesModel::integer_plate_id_type feature_reconstruction_plate_id);

			/**
			 * Generate the time windows.
			 */
			void
			initialise_time_windows(
					const ResolvedNetworkTimeSpan &resolved_network_time_span,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const std::vector<GPlatesMaths::PointOnSphere> &present_day_geometry_points);

			/**
			 * Generate the deformation information that is accumulated going forward in time.
			 */
			void
			initialise_forward_time_accumulated_deformation_information();

			/**
			 * Returns the deformed geometry sample data as points at the specified time (which can be any time).
			 */
			void
			get_geometry_sample_data(
					const double &reconstruction_time,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
					boost::optional<std::vector<DeformationInfo> &> deformation_info_points = boost::none);

			/**
			 * Rigidly rotates the geometry points from present day to @a reconstruction_time.
			 */
			void
			rigid_reconstruct(
					std::vector<GPlatesMaths::PointOnSphere> &rotated_geometry_points,
					const std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const double &reconstruction_time) const;

			/**
			 * Rigidly rotates the geometry points from @a initial_time to @a final_time.
			 */
			void
			rigid_reconstruct(
					std::vector<GPlatesMaths::PointOnSphere> &rotated_geometry_points,
					const std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const double &initial_time,
					const double &final_time) const;

			/**
			 * Returns geometry points as a @a GeometryOnSphere of same type as the present day geometry.
			 */
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
			create_geometry_on_sphere(
					const std::vector<GPlatesMaths::PointOnSphere> &geometry_points) const;
		};
	}
}

#endif // GPLATES_APP_LOGIC_GEOMETRYDEFORMATION_H
