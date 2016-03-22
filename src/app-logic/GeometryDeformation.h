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

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructionTreeCreator.h"
#include "ResolvedTopologicalNetwork.h"
#include "TimeSpanUtils.h"
#include "VelocityDeltaTime.h"

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
		//! Typedef for a sequence of resolved topological networks.
		typedef std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> rtn_seq_type;

		/**
		 * A look up table of resolved topological networks over a time span.
		 *
		 * Each time sample is a @a rtn_seq_type (sequence of RTNs).
		 */
		typedef TimeSpanUtils::TimeSampleSpan<rtn_seq_type> resolved_network_time_span_type;


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

			/**
			 * Copies the accumulated values across from another DeformationInfo.
			 *
			 * This is useful between deformation periods where the instantaneous values are zero
			 * but the accumulated values are propagated across gaps between deformation periods.
			 */
			void
			set_accumulated_values(
					const DeformationInfo &deformation_info)
			{
				S22 = deformation_info.S22;
				S33 = deformation_info.S33;
				S23 = deformation_info.S23;

				S_DIR = deformation_info.S_DIR;
				S1 = deformation_info.S1;
				S2 = deformation_info.S2;
				dilitation_strain = deformation_info.dilitation_strain;
				second_invariant_strain = deformation_info.second_invariant_strain;
			}


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
					const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
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
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span);

		private:

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


				template <typename PointsForwardIter>
				GeometrySample(
						PointsForwardIter points_begin,
						PointsForwardIter points_end) :
					d_points(points_begin, points_end),
					d_have_initialised_instantaneous_deformation_information(false)
				{
					// Allocate deformation information - it'll get initialised later.
					d_deformation_infos.resize(d_points.size());
				}

				explicit
				GeometrySample(
					const std::vector<GPlatesMaths::PointOnSphere> &points) :
					d_points(points),
					d_have_initialised_instantaneous_deformation_information(false)
				{
					// Allocate deformation information - it'll get initialised later.
					d_deformation_infos.resize(d_points.size());
				}


				const std::vector<GPlatesMaths::PointOnSphere> &
				get_points() const
				{
					return d_points;
				}

				std::vector<GPlatesMaths::PointOnSphere> &
				get_points()
				{
					return d_points;
				}


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

			private:
				std::vector<GPlatesMaths::PointOnSphere> d_points;
				mutable std::vector<DeformationInfo> d_deformation_infos;
				mutable bool d_have_initialised_instantaneous_deformation_information;

				//! boost::none means all geometry points lie outside the delaunay triangulation.
				boost::optional<delaunay_face_opt_seq_type> d_delaunay_faces;

				//! Calculate instantaneous deformation values but not forward-time-accumulated values.
				void
				calc_instantaneous_deformation_information() const;
			};


			/**
			 * Typedef for a span of time windows.
			 */
			typedef TimeSpanUtils::TimeWindowSpan<GeometrySample> time_window_span_type;


			GPlatesModel::integer_plate_id_type d_reconstruction_plate_id;

			GPlatesMaths::GeometryType::Value d_geometry_type;

			time_window_span_type::non_null_ptr_type d_time_window_span;

			/**
			 * Is true if we've generated the deformation information that is accumulated
			 * going forward in time.
			 */
			bool d_have_initialised_forward_time_accumulated_deformation_information;


			GeometryTimeSpan(
					const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &feature_present_day_geometry,
					GPlatesModel::integer_plate_id_type feature_reconstruction_plate_id);

			/**
			 * Generate the time windows.
			 */
			void
			initialise_time_windows(
					const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
					const ReconstructionTreeCreator &reconstruction_tree_creator);

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
			 * Create a new GeometrySample from the closest younger sample by rigid rotation.
			 */
			GeometrySample
			create_rigid_geometry_sample(
					const double &reconstruction_time,
					const double &closest_younger_sample_time,
					const GeometrySample &closest_younger_sample,
					const ReconstructionTreeCreator &reconstruction_tree_creator);

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
