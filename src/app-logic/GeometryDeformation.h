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

#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "DeformationStrain.h"
#include "ReconstructionTreeCreator.h"
#include "ResolvedTopologicalNetwork.h"
#include "TimeSpanUtils.h"
#include "VelocityDeltaTime.h"

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
		//! Typedef for a sequence of resolved topological networks.
		typedef std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> rtn_seq_type;

		/**
		 * A look up table of resolved topological networks over a time span.
		 *
		 * Each time sample is a @a rtn_seq_type (sequence of RTNs).
		 */
		typedef TimeSpanUtils::TimeSampleSpan<rtn_seq_type> resolved_network_time_span_type;


		//! Typedef for an optional point location (delaunay face or rigid block) within a resolved network.
		typedef boost::optional<
				std::pair<
						ResolvedTriangulation::Network::point_location_type,
						ResolvedTopologicalNetwork::non_null_ptr_type> > network_point_location_opt_type;

		//! Typedef for a sequence of optional network point locations.
		typedef std::vector<network_point_location_opt_type> network_point_location_opt_seq_type;



		/**
		 * Deforms the specified geometry from present day to the specified reconstruction time -
		 * unless @a reverse_deform is true in which case the geometry is assumed to be
		 * the deformed geometry (at the reconstruction time) and the returned geometry will
		 * then be the present day geometry.
		 *
		 * This is mainly useful when you have a feature and are modifying its geometry at some
		 * reconstruction time (not present day). After each modification the geometry needs to be
		 * reverse deformed to present day before it can be attached back onto the feature
		 * because feature's typically store present day geometry in their geometry properties.
		 *
		 * The resolved network time span is used to deform the geometry within its time range.
		 * Outside that time range the geometry is rigidly reconstructed using the specified
		 * reconstruction plate id. Although within that time range the geometry can be rigidly
		 * reconstructed if it does not intersect any resolved networks at specific times).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		deform_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				GPlatesModel::integer_plate_id_type reconstruction_plate_id,
				const double &reconstruction_time,
				const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				bool reverse_deform = false);


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
			 * the feature is visible (and generates ReconstructedFeatureGeometry's).
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
			 * Also returns optional per-point deformation strain rates and total strains.
			 */
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
			get_geometry(
					const double &reconstruction_time,
					boost::optional<std::vector<DeformationStrain> &> deformation_strain_rates = boost::none,
					boost::optional<std::vector<DeformationStrain> &> deformation_total_strains = boost::none);

			/**
			 * Same as @a get_geometry except returns geometry as points.
			 */
			void
			get_geometry_points(
					std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
					const double &reconstruction_time,
					boost::optional<std::vector<DeformationStrain> &> deformation_strain_rates = boost::none,
					boost::optional<std::vector<DeformationStrain> &> deformation_total_strains = boost::none);

			/**
			 * Calculate velocities at the geometry (domain) points at the specified time (which can
			 * be any time within the valid time period of the geometry's feature - it's up to the
			 * caller to check that).
			 *
			 * @a surfaces returns the resolved network (or network interior rigid block) that
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
					VelocityDeltaTime::Type velocity_delta_time_type);

		private:

			/**
			 * A geometry snapshot consisting of geometry points and associated per-point info.
			 */
			class GeometrySample
			{
			public:

				template <typename PointsForwardIter>
				GeometrySample(
						PointsForwardIter points_begin,
						PointsForwardIter points_end) :
					d_points(points_begin, points_end),
					d_have_initialised_deformation_strain_rates(false)
				{
					// Allocate deformation information - it'll get initialised later.
					d_deformation_strain_rates.resize(d_points.size());
					d_deformation_total_strains.resize(d_points.size());
					d_network_point_locations.resize(d_points.size());
				}

				explicit
				GeometrySample(
						const std::vector<GPlatesMaths::PointOnSphere> &points) :
					d_points(points),
					d_have_initialised_deformation_strain_rates(false)
				{
					// Allocate deformation information - it'll get initialised later.
					d_deformation_strain_rates.resize(d_points.size());
					d_deformation_total_strains.resize(d_points.size());
					d_network_point_locations.resize(d_points.size());
				}

				/**
				 * This is currently used when interpolating two geometry samples.
				 * In this case the interpolated deformation strain rates and total strains are also provided -
				 * unfortunately this means we don't delay the initialisation of the deformation information.
				 */
				GeometrySample(
						const std::vector<GPlatesMaths::PointOnSphere> &points,
						const std::vector<DeformationStrain> &deformation_strain_rates,
						const std::vector<DeformationStrain> &deformation_total_strains,
						const network_point_location_opt_seq_type &network_point_locations) :
					d_points(points),
					d_deformation_strain_rates(deformation_strain_rates),
					d_deformation_total_strains(deformation_total_strains),
					d_have_initialised_deformation_strain_rates(true),
					d_network_point_locations(network_point_locations)
				{
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							points.size() == deformation_strain_rates.size() &&
								points.size() == deformation_total_strains.size() &&
								points.size() == network_point_locations.size(),
							GPLATES_ASSERTION_SOURCE);
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


				const std::vector<DeformationStrain> &
				get_deformation_strain_rates() const
				{
					if (!d_have_initialised_deformation_strain_rates)
					{
						calc_deformation_strain_rates();
					}
					return d_deformation_strain_rates;
				}

				std::vector<DeformationStrain> &
				get_deformation_strain_rates()
				{
					return const_cast<std::vector<DeformationStrain> &>(
							static_cast<const GeometrySample *>(this)->get_deformation_strain_rates());
				}


				const std::vector<DeformationStrain> &
				get_deformation_total_strains() const
				{
					return d_deformation_total_strains;
				}

				std::vector<DeformationStrain> &
				get_deformation_total_strains()
				{
					return d_deformation_total_strains;
				}

				/**
				 * Get the network point locations (if any) that each geometry point lies within.
				 */
				const network_point_location_opt_seq_type &
				get_network_point_locations() const
				{
					return d_network_point_locations;
				}

				/**
				 * Set the network point locations (if any) that each geometry point lies within.
				 */
				void
				set_network_point_locations(
						const network_point_location_opt_seq_type &network_point_locations)
				{
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							d_points.size() == network_point_locations.size(),
							GPLATES_ASSERTION_SOURCE);

					d_network_point_locations = network_point_locations;
				}

			private:
				std::vector<GPlatesMaths::PointOnSphere> d_points;
				mutable std::vector<DeformationStrain> d_deformation_strain_rates;
				mutable std::vector<DeformationStrain> d_deformation_total_strains;
				mutable bool d_have_initialised_deformation_strain_rates;

				/**
				 * As an optimisation, store the network point location containing the point so
				 * we don't have to locate the face a second time if we're asked to look up the strain.
				 */
				network_point_location_opt_seq_type d_network_point_locations;

				//! Calculate instantaneous deformation strain rates, but not forward-time-accumulated values.
				void
				calc_deformation_strain_rates() const;
			};


			/**
			 * Typedef for a span of time windows.
			 */
			typedef TimeSpanUtils::TimeWindowSpan<GeometrySample> time_window_span_type;


			GPlatesModel::integer_plate_id_type d_reconstruction_plate_id;

			GPlatesMaths::GeometryType::Value d_geometry_type;

			time_window_span_type::non_null_ptr_type d_time_window_span;


			// Used to generate reconstructed/deformed points and velocities.
			ReconstructionTreeCreator d_reconstruction_tree_creator;
			resolved_network_time_span_type::non_null_ptr_to_const_type d_resolved_network_time_span;


			/**
			 * Is true if we've generated the deformation total strains (accumulated going forward in time).
			 */
			bool d_have_initialised_deformation_total_strains;


			GeometryTimeSpan(
					const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &feature_present_day_geometry,
					GPlatesModel::integer_plate_id_type feature_reconstruction_plate_id);

			/**
			 * Generate the time windows.
			 */
			void
			initialise_time_windows();

			/**
			 * Generate the deformation accumulated/total strains (accumulated going forward in time).
			 */
			void
			initialise_deformation_total_strains();

			/**
			 * Calculate velocities at the specified domain points.
			 */
			void
			calc_velocities(
					const std::vector<GPlatesMaths::PointOnSphere> &domain_points,
					std::vector<GPlatesMaths::Vector3D> &velocities,
					std::vector< boost::optional<const ReconstructionGeometry *> > &surfaces,
					const boost::optional<PlateVelocityUtils::TopologicalNetworksVelocities> &resolved_networks_query,
					const double &reconstruction_time,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type);

			/**
			 * Returns the deformed geometry sample data as points at the specified time (which can be any time).
			 */
			void
			get_geometry_sample_data(
					std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
					const double &reconstruction_time,
					boost::optional<std::vector<DeformationStrain> &> deformation_strain_rates = boost::none,
					boost::optional<std::vector<DeformationStrain> &> deformation_total_strains = boost::none,
					boost::optional<network_point_location_opt_seq_type &> network_point_locations = boost::none);

			/**
			 * Create a new GeometrySample from the closest younger sample by rigid rotation.
			 */
			GeometrySample
			create_rigid_geometry_sample(
					const double &reconstruction_time,
					const double &closest_younger_sample_time,
					const GeometrySample &closest_younger_sample);

			/**
			 * Interpolate two geometry samples in adjacent time slots.
			 */
			GeometrySample
			interpolate_geometry_sample(
					const double &interpolate_position,
					const double &first_geometry_time,
					const double &second_geometry_time,
					const GeometrySample &first_geometry_sample,
					const GeometrySample &second_geometry_sample);
		};
	}
}

#endif // GPLATES_APP_LOGIC_GEOMETRYDEFORMATION_H
