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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYRECONSTRUCT_H
#define GPLATES_APP_LOGIC_TOPOLOGYRECONSTRUCT_H

#include <map>
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>

#include "DeformationStrain.h"
#include "DeformationStrainRate.h"
#include "ReconstructionTreeCreator.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "TimeSpanUtils.h"
#include "TopologyPointLocation.h"
#include "TopologyReconstruct.h"
#include "VelocityDeltaTime.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/AngularExtent.h"
#include "maths/FiniteRotation.h"
#include "maths/GeometryOnSphere.h"
#include "maths/GeometryType.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometry;

	/**
	 * Uses topologies (rigid and deforming plates) to incrementally reconstruct geometries over time.
	 */
	class TopologyReconstruct :
			public GPlatesUtils::ReferenceCount<TopologyReconstruct>
	{
	public:
		class GeometryTimeSpan;

		//! A convenience typedef for a shared pointer to a non-const @a TopologyReconstruct.
		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyReconstruct> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a TopologyReconstruct.
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyReconstruct> non_null_ptr_to_const_type;


		//! Typedef for a sequence of resolved topological boundaries.
		typedef std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> rtb_seq_type;

		//! Typedef for a sequence of resolved topological networks.
		typedef std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> rtn_seq_type;


		/**
		 * A look up table of resolved topological boundaries over a time span.
		 *
		 * Each time sample is a @a rtb_seq_type (sequence of RTBs).
		 */
		typedef TimeSpanUtils::TimeSampleSpan<rtb_seq_type> resolved_boundary_time_span_type;

		/**
		 * A look up table of resolved topological networks over a time span.
		 *
		 * Each time sample is a @a rtn_seq_type (sequence of RTNs).
		 */
		typedef TimeSpanUtils::TimeSampleSpan<rtn_seq_type> resolved_network_time_span_type;


		/**
		 * Interface for deactivating geometry points as a geometry is reconstructed forward and backward
		 * from its geometry import time.
		 */
		class DeactivatePoint :
				public GPlatesUtils::ReferenceCount<DeactivatePoint>
		{
		public:

			typedef GPlatesUtils::non_null_intrusive_ptr<DeactivatePoint> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const DeactivatePoint> non_null_ptr_to_const_type;

			virtual
			~DeactivatePoint()
			{  }

			/**
			 * Return true if the current point should be deactivated.
			 */
			virtual
			bool
			deactivate(
					const GPlatesMaths::PointOnSphere &prev_point,
					const TopologyPointLocation &prev_location,
					const GPlatesMaths::PointOnSphere &current_point,
					const TopologyPointLocation &current_location,
					const double &current_time,
					bool reverse_reconstruct) const = 0;
		};

		/**
		 * Default implementation for deactivating geometry points.
		 */
		class DefaultDeactivatePoint :
				public DeactivatePoint
		{
		public:

			typedef GPlatesUtils::non_null_intrusive_ptr<DefaultDeactivatePoint> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const DefaultDeactivatePoint> non_null_ptr_to_const_type;

			// Default parameters.
			static constexpr double DEFAULT_THRESHOLD_VELOCITY_DELTA = 0.7;
			static constexpr double DEFAULT_THRESHOLD_DISTANCE_TO_BOUNDARY_IN_KMS_PER_MY = 10.0;
			static constexpr bool DEFAULT_DEACTIVATE_POINTS_THAT_FALL_OUTSIDE_A_NETWORK = false;

			/**
			 * NOTE: Ensure that @a time_increment matches that in the time range of @a TopologyReconstruct::create.
			 */
			static
			non_null_ptr_type
			create(
					const double &time_increment,
					const double &threshold_velocity_delta = DEFAULT_THRESHOLD_VELOCITY_DELTA,
					const double &threshold_distance_to_boundary_in_kms_per_my = DEFAULT_THRESHOLD_DISTANCE_TO_BOUNDARY_IN_KMS_PER_MY,
					bool deactivate_points_that_fall_outside_a_network = DEFAULT_DEACTIVATE_POINTS_THAT_FALL_OUTSIDE_A_NETWORK);

			/**
			 * Return true if the current point should be deactivated.
			 */
			virtual
			bool
			deactivate(
					const GPlatesMaths::PointOnSphere &prev_point,
					const TopologyPointLocation &prev_location,
					const GPlatesMaths::PointOnSphere &current_point,
					const TopologyPointLocation &current_location,
					const double &current_time,
					bool reverse_reconstruct) const;

		private:
			//! Typedef for map used to keep track of stage rotations by plate ID.
			typedef std::map<GPlatesModel::integer_plate_id_type, GPlatesMaths::FiniteRotation> plate_id_to_stage_rotation_map_type;

			// Threshold parameters used to determine when to deactivate geometry points in a geometry sample.
			double d_time_increment;
			double d_threshold_velocity_delta; // cms/yr
			GPlatesMaths::AngularExtent d_min_distance_threshold_radians;
			bool d_deactivate_points_that_fall_outside_a_network;

			// Keep track of the stage rotations of resolved boundaries as we encounter them.
			// This is an optimisation that saves a few seconds (for a large number of points in geometry)
			// since many points will be inside the same resolved boundary.
			mutable plate_id_to_stage_rotation_map_type d_velocity_stage_rotation_map;
			// Only cache stage rotations for a specific reconstruction time.
			// We clear it when we move onto a different reconstruction time.
			mutable GPlatesMaths::real_t d_velocity_stage_rotation_time;

			DefaultDeactivatePoint(
					const double &time_increment,
					const double &threshold_velocity_delta,
					const double &threshold_distance_to_boundary_in_kms_per_my,
					bool deactivate_points_that_fall_outside_a_network);

			/**
			 * Returns true if the delta velocity of the previous point is large enough, or it is
			 * close enough to the previous boundary to reach it (at its previous relative velocity).
			 */
			bool
			is_delta_velocity_large_enough_or_point_close_to_boundary(
					const GPlatesMaths::Vector3D &delta_velocity,
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &prev_topology_boundary,
					const GPlatesMaths::PointOnSphere &prev_point) const;

			/**
			 * Calculate a stage rotation for velocity calculations.
			 *
			 * Note that the stage rotation is going forward in time (old to young).
			 *
			 * This avoids re-calculating the stage rotation for the same plate ID.
			 */
			const GPlatesMaths::FiniteRotation &
			get_or_create_velocity_stage_rotation(
					GPlatesModel::integer_plate_id_type reconstruction_plate_id,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const double &reconstruction_time,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type) const;
		};


		/**
		 * Creates a new @a TopologyReconstruct.
		 */
		static
		non_null_ptr_type
		create(
				const TimeSpanUtils::TimeRange &time_range,
				const resolved_boundary_time_span_type::non_null_ptr_to_const_type &resolved_boundary_time_span,
				const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
				const ReconstructionTreeCreator &reconstruction_tree_creator)
		{
			return non_null_ptr_type(
					new TopologyReconstruct(
							time_range,
							resolved_boundary_time_span,
							resolved_network_time_span,
							reconstruction_tree_creator));
		}

		/**
		 * Returns the time range over which to reconstruct using the resolved boundaries/networks.
		 *
		 * This is the time range of all geometry time spans created by us.
		 */
		const TimeSpanUtils::TimeRange &
		get_time_range() const
		{
			return d_time_range;
		}

		/**
		 * Returns the time span of resolved topological boundaries.
		 */
		const resolved_boundary_time_span_type::non_null_ptr_to_const_type &
		get_resolved_boundary_time_span() const
		{
			return d_resolved_boundary_time_span;
		}

		/**
		 * Returns the time span of resolved topological networks.
		 */
		const resolved_network_time_span_type::non_null_ptr_to_const_type &
		get_resolved_network_time_span() const
		{
			return d_resolved_network_time_span;
		}

		/**
		 * Returns the reconstruction tree creator.
		 *
		 * This is only used when a geometry point is not inside a resolved boundary/network,
		 * otherwise the reconstruction tree creator of the resolved boundary/network is used.
		 */
		const ReconstructionTreeCreator &
		get_reconstruction_tree_creator() const
		{
			return d_reconstruction_tree_creator;
		}


		/**
		 * Creates a time span for the specified present day geometry.
		 *
		 * The internal resolved boundary/network time spans are used to reconstruct the geometry (within the time range).
		 * Outside that time range the geometry is rigidly reconstructed using the specified
		 * reconstruction plate id. Although within that time range the geometry can be rigidly
		 * reconstructed if it does not intersect any resolved boundaries/networks at specific times.
		 *
		 * If @a geometry_import_time is specified then the present day geometry is rigidly reconstructed
		 * (using @a reconstruction_plate_id) to the geometry import time. That geometry is then
		 * reconstructed forward and/or backward in time as necessary to fill the time range.
		 * This enables paleo-geometries to be used (eg, fracture zones prior to subduction) and masked by topologies
		 * through time (ie, masked by mid-ocean ridges going backward in time and subduction zones going forward in time).
		 *
		 * If @a deactivate_points is specified then geometry points can be deactivated as reconstruction progresses
		 * forward and backward in time beginning at the geometry import time, otherwise geometry points always active.
		 *
		 * If @a max_poly_segment_angular_extent_radians is specified and @a geometry is a polyline or polygon,
		 * then the geometry is tessellated such that the subdivided arcs have a maximum angular extent of
		 * @a max_poly_segment_angular_extent_radians radians. All geometry types are converted to multi-point
		 * anyway so this just ensures that when a polyline or polygon is converted to a multi-point that it
		 * has enough points that it at least still resembles a polyline or polygon.
		 * Note that, with tessellation, the number of points generated by @a get_all_geometry_data can be different
		 * from the number of points in @a geometry here.
		 *
		 * If @a deformation_uses_natural_neighbour_interpolation is true then use natural neighbour coordinates
		 * when deforming points are in topological networks, otherwise use barycentric interpolation.
		 *
		 * NOTE: If the feature does not exist for the entire time span we still reconstruct it using topologies.
		 * This is an issue to do with storing feature geometry in present day coordinates.
		 * We need to be able to change the feature's end time without having it change the position
		 * of the feature's topology-reconstructed geometry prior to the feature's end (disappearance) time.
		 * Changing the feature's begin/end time then only changes the time window within which
		 * the feature is visible (and generates ReconstructedFeatureGeometry's).
		 */
		/*GeometryTimeSpan::non_null_ptr_type*/GPlatesUtils::non_null_intrusive_ptr<GeometryTimeSpan>
		create_geometry_time_span(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				GPlatesModel::integer_plate_id_type reconstruction_plate_id,
				const double &geometry_import_time = 0.0,
				boost::optional<DeactivatePoint::non_null_ptr_to_const_type> deactivate_points = boost::none,
				boost::optional<double> max_poly_segment_angular_extent_radians = boost::none,
				bool deformation_uses_natural_neighbour_interpolation = true) const;


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
			 * Returns true if the geometry is active at the specified reconstruction time.
			 *
			 * If all geometry points subduct (going forward in time) or are consumed by mid-ocean ridges
			 * (going backward in time) or both, then the time range in which the geometry is valid
			 * will be reduced from the normal unlimited range (ie, [-inf, inf]).
			 * Note that this is different than the *feature* time of appearance/disappearance which
			 * is outside the scope of this class (and must be handled by the caller).
			 */
			bool
			is_valid(
					const double &reconstruction_time) const;

			/**
			 * Returns the geometry as a multi-point (or a single point) at the specified time.
			 *
			 * Note: The returned geometry consists of active points only (which means the number of
			 * points in the returned geometry could be less than @a get_num_all_geometry_points).
			 *
			 * Returns none if @a is_valid returns false.
			 */
			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
			get_geometry(
					const double &reconstruction_time) const;


			/**
			 * Returns the requested geometry data at the specified time.
			 *
			 * Note: Only data at *active* points are returned (which means the number of
			 * points in the returned data could be less than @a get_num_all_geometry_points).
			 *
			 * Returns false if @a is_valid returns false (in which case the data arrays are unmodified).
			 */
			bool
			get_geometry_data(
					const double &reconstruction_time,
					boost::optional< std::vector<GPlatesMaths::PointOnSphere> &> points = boost::none,
					boost::optional< std::vector<TopologyPointLocation> &> point_locations = boost::none,
					boost::optional< std::vector<DeformationStrainRate> &> strain_rates = boost::none,
					boost::optional< std::vector<DeformationStrain> &> strains = boost::none) const;

			/**
			 * Returns the requested geometry data at *all* points at the specified time (including inactive points).
			 *
			 * Inactive points will store 'none' (such that the size of the returned data
			 * will always be @a get_num_all_geometry_points).
			 *
			 * Returns false if @a is_valid returns false (in which case the data arrays are unmodified).
			 */
			bool
			get_all_geometry_data(
					const double &reconstruction_time,
					boost::optional< std::vector< boost::optional<GPlatesMaths::PointOnSphere> > &> points = boost::none,
					boost::optional< std::vector< boost::optional<TopologyPointLocation> > &> point_locations = boost::none,
					boost::optional< std::vector< boost::optional<DeformationStrainRate> > &> strain_rates = boost::none,
					boost::optional< std::vector< boost::optional<DeformationStrain> > &> strains = boost::none) const;

			/**
			 * Returns the number of points returned by @a get_all_geometry_data.
			 *
			 * Note that this can be different from the number of original geometry points passed
			 * into TopologyReconstruct::create_geometry_time_span() if the geometry was tessellated
			 * (thus introducing new points).
			 */
			unsigned int
			get_num_all_geometry_points() const
			{
				return d_time_window_span->get_present_day_sample()->get_num_geometry_points();
			}

			/**
			 * Returns whether each point (of *all* points at the specified time) is active or not.
			 *
			 * The same could be achieved by calling 'get_all_geometry_data(reconstruction_time, points)' and
			 * then testing the boost::optional of each point for its active status, but this method is easier.
			 *
			 * Returns false if @a is_valid returns false (in which case @a is_active is not modified).
			 */
			bool
			get_is_active_data(
					const double &reconstruction_time,
					std::vector<bool> &is_active) const;


			/**
			 * Calculate velocities at the geometry (domain) points at the specified time.
			 *
			 * Optional @a domain_points returns the domain points (at the reconstruction time).
			 *
			 * Optional @a domain_point_locations returns the resolved network (or network interior rigid block) or resolved plate boundary
			 * that each domain point intersects (if any).
			 *
			 * The sizes of @a velocities, @a domain_points and @a domain_point_locations are the same and match the number
			 * of original geometry points that have not been subducted/consumed at the reconstruction time.
			 *
			 * Returns false if @a is_valid returns false.
			 */
			bool
			get_velocities(
					std::vector<GPlatesMaths::Vector3D> &velocities,
					const double &reconstruction_time,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type,
					boost::optional< std::vector<GPlatesMaths::PointOnSphere> &> domain_points = boost::none,
					boost::optional< std::vector<TopologyPointLocation> &> domain_point_locations = boost::none) const;


			//
			// Interface used by ScalarCoverageDeformation::ScalarCoverageTimeSpan...
			//

			/**
			 * The time range of this geometry time span.
			 *
			 * This is the time range in which a history of geometry data is stored.
			 * It's possible for geometry data to be valid outside this time range
			 * (at these times the geometry is rigidly rotated from the beginning or
			 * ending of the time range depending on which side its on). See also @a is_valid.
			 */
			const TimeSpanUtils::TimeRange &
			get_time_range() const
			{
				return d_time_range;
			}

			/**
			 * The time that we started topology reconstruction of the initial geometry from.
			 */
			const double &
			get_geometry_import_time() const
			{
				return d_geometry_import_time;
			}

			/**
			 * Each point in the original geometry (at the geometry import time) can be considered
			 * to be an interpolation of two adjacent points of an original polyline segment.
			 *
			 * This is useful when tessellating the original polyline/polygon geometry and the
			 * scalar values associated with the points will need to generate interpolated scalars
			 * at the tessellated points.
			 *
			 * This only applies to polyline and polygon geometries (multi-points and points aren't tessellated).
			 */
			struct InterpolateOriginalPoints
			{
				InterpolateOriginalPoints(
						const double &interpolate_ratio_,
						unsigned int first_point_index_,
						unsigned int second_point_index_) :
					interpolate_ratio(interpolate_ratio_),
					first_point_index(first_point_index_),
					second_point_index(second_point_index_)
				{  }

				/**
				 * Value in range [0, 1] where 0 represents the first point and 1 the second point.
				 */
				double interpolate_ratio;

				unsigned int first_point_index;
				unsigned int second_point_index;
			};

			//! Typedef for a sequence of @a InterpolateOriginalPoints.
			typedef std::vector<InterpolateOriginalPoints> interpolate_original_points_seq_type;

			const interpolate_original_points_seq_type &
			get_interpolate_original_points() const
			{
				return d_interpolate_original_points;
			}

		private:

			//! RAII class in whose scope we are accessing strain rates.
			class AccessingStrainRates
			{
			public:
				AccessingStrainRates(
						const GeometryTimeSpan &geometry_time_span) :
					d_geometry_time_span(geometry_time_span)
				{
					++d_geometry_time_span.d_accessing_strain_rates;
				}

				~AccessingStrainRates()
				{
					--d_geometry_time_span.d_accessing_strain_rates;
				}

			private:
				const GeometryTimeSpan &d_geometry_time_span;
			};

			//! RAII class in whose scope we are accessing (total) strains.
			class AccessingStrains
			{
			public:
				AccessingStrains(
						const GeometryTimeSpan &geometry_time_span) :
					d_geometry_time_span(geometry_time_span)
				{
					++d_geometry_time_span.d_accessing_strains;
				}

				~AccessingStrains()
				{
					--d_geometry_time_span.d_accessing_strains;
				}

			private:
				const GeometryTimeSpan &d_geometry_time_span;
			};


			/**
			 * A point (and associated location, strain rate and strain) in a geometry sample.
			 */
			struct GeometryPoint
			{
				explicit
				GeometryPoint(
						const GPlatesMaths::UnitVector3D &point) :
					position(point),
					strain_rate(NULL),
					strain(NULL)
				{  }

				explicit
				GeometryPoint(
						const GPlatesMaths::PointOnSphere &point) :
					position(point.position_vector()),
					strain_rate(NULL),
					strain(NULL)
				{  }

				GeometryPoint(
						const GPlatesMaths::PointOnSphere &point,
						const TopologyPointLocation &location_) :
					position(point.position_vector()),
					location(location_),
					strain_rate(NULL),
					strain(NULL)
				{  }

				// Using a UnitVector3D saves 8 bytes over a PointOnSphere...
				GPlatesMaths::UnitVector3D position;
				TopologyPointLocation location;
				DeformationStrainRate *strain_rate; // NULL means zero strain rate
				DeformationStrain *strain;      // NULL means zero strain
			};


			/**
			 * Allocates objects of type @a GeometryPoint and @a DeformationStrain from boost::object_pool.
			 */
			class PoolAllocator :
					public GPlatesUtils::ReferenceCount<PoolAllocator>
			{
			public:
				typedef GPlatesUtils::non_null_intrusive_ptr<PoolAllocator> non_null_ptr_type;
				typedef GPlatesUtils::non_null_intrusive_ptr<const PoolAllocator> non_null_ptr_to_const_type;

				static
				non_null_ptr_type
				create()
				{
					return non_null_ptr_type(new PoolAllocator());
				}


				boost::object_pool<GeometryPoint> geometry_point_pool;
				boost::object_pool<DeformationStrain> deformation_strain_pool;
				boost::object_pool<DeformationStrainRate> deformation_strain_rate_pool;

			private:
				PoolAllocator()
				{  }
			};


			/**
			 * A geometry snapshot consisting of geometry points and associated per-point info.
			 */
			class GeometrySample :
					public GPlatesUtils::ReferenceCount<GeometrySample>
			{
			public:
				typedef GPlatesUtils::non_null_intrusive_ptr<GeometrySample> non_null_ptr_type;
				typedef GPlatesUtils::non_null_intrusive_ptr<const GeometrySample> non_null_ptr_to_const_type;


				/**
				 * Create from a sequence of points.
				 *
				 * All points are active.
				 *
				 * @a pool_allocator is used to allocate the associated @a GeometryPoint objects.
				 */
				static
				non_null_ptr_type
				create(
						const std::vector<GPlatesMaths::PointOnSphere> &points,
						const PoolAllocator::non_null_ptr_type &pool_allocator)
				{
					return non_null_ptr_type(new GeometrySample(points, pool_allocator));
				}

				/**
				 * Create from a sequence of @a GeometryPoint objects that were allocated using @a pool_allocator.
				 *
				 * Set @a have_initialised_strain_rates to false if the strain rates have not been initialised.
				 * They will get initialised later if @a get_geometry_points is called with its
				 * @a accessing_strain_rates set to true (and the @a TopologyPointLocation in each
				 * geometry point has been initialised.
				 *
				 * Note: The contents of @a geometry_points are swapped internally leaving it empty upon returning.
				 */
				static
				non_null_ptr_type
				create_swap(
						std::vector<GeometryPoint *> &geometry_points,
						const PoolAllocator::non_null_ptr_type &pool_allocator,
						bool have_initialised_strain_rates = false)
				{
					return non_null_ptr_type(
							new GeometrySample(geometry_points, pool_allocator, have_initialised_strain_rates));
				}


				/**
				 * Returns the geometry points.
				 *
				 * Set @a accessing_strain_rates to true if you will be accessing the strain rates.
				 * This ensures they are first calculated (if haven't already been).
				 * But make sure the @a TopologyPointLocation in each geometry point has been initialised first
				 * since these are used to determine the strain rates.
				 */
				std::vector<GeometryPoint *> &
				get_geometry_points(
						bool accessing_strain_rates)
				{
					if (accessing_strain_rates &&
						!d_have_initialised_strain_rates)
					{
						calc_deformation_strain_rates();
					}

					return d_geometry_points;
				}


				/**
				 * Number of geometry points.
				 */
				unsigned int
				get_num_geometry_points() const
				{
					return d_geometry_points.size();
				}

			private:

				/**
				 * Geometry points (NULL indicates point not active).
				 */
				std::vector<GeometryPoint *> d_geometry_points;

				/**
				 * The pool allocator used to allocate @a GeometryPoint data.
				 */
				PoolAllocator::non_null_ptr_type d_pool_allocator;

				bool d_have_initialised_strain_rates;


				explicit
				GeometrySample(
						const std::vector<GPlatesMaths::PointOnSphere> &points,
						const PoolAllocator::non_null_ptr_type &pool_allocator) :
					d_pool_allocator(pool_allocator),
					d_have_initialised_strain_rates(false)
				{
					const unsigned int num_points = points.size();
					d_geometry_points.reserve(num_points);
					for (unsigned int n = 0; n < num_points; ++n)
					{
						d_geometry_points.push_back(
								d_pool_allocator->geometry_point_pool.construct(points[n]));
					}
				}

				// Swap constructor - swaps @a geometry_points with @a d_geometry_points.
				explicit
				GeometrySample(
						std::vector<GeometryPoint *> &geometry_points,
						const PoolAllocator::non_null_ptr_type &pool_allocator,
						bool have_initialised_strain_rates) :
					d_pool_allocator(pool_allocator),
					d_have_initialised_strain_rates(have_initialised_strain_rates)
				{
					d_geometry_points.swap(geometry_points);
				}

				//! Calculate instantaneous deformation strain rates, but not forward-time-accumulated values.
				void
				calc_deformation_strain_rates();
			};

			/**
			 * Typedef for map used to keep track of stage rotations by plate ID.
			 */
			typedef std::map<GPlatesModel::integer_plate_id_type, GPlatesMaths::FiniteRotation> plate_id_to_stage_rotation_map_type;

			/**
			 * Typedef for a span of time windows.
			 */
			typedef TimeSpanUtils::TimeWindowSpan<GeometrySample::non_null_ptr_type> time_window_span_type;


			TopologyReconstruct::non_null_ptr_to_const_type d_topology_reconstruct;
			TimeSpanUtils::TimeRange d_time_range;

			/**
			 * The pool allocator used to allocate @a GeometryPoint data.
			 */
			PoolAllocator::non_null_ptr_type d_pool_allocator;

			GPlatesModel::integer_plate_id_type d_reconstruction_plate_id;
			double d_geometry_import_time;
			bool d_deformation_uses_natural_neighbour_interpolation;
			interpolate_original_points_seq_type d_interpolate_original_points;
			time_window_span_type::non_null_ptr_type d_time_window_span;

			//! The first time slot that the geometry becomes active (if was even de-activated going backward in time).
			boost::optional<unsigned int> d_time_slot_of_appearance;
			//! The last time slot that the geometry remains active (if was even de-activated going forward in time).
			boost::optional<unsigned int> d_time_slot_of_disappearance;

			/**
			 * Used to determine when to deactivate geometry points.
			 *
			 * If none then points are never deactivated.
			 */
			boost::optional<DeactivatePoint::non_null_ptr_to_const_type> d_deactivate_points;

			/**
			 * Are we currently accessing strain rates (ie, is non-zero integer) ?
			 *
			 * This is used when accessing strain rates in calls to 'GeometrySample::get_geometry_points()'.
			 *
			 * See @a AccessingStrainRates.
			 */
			mutable int d_accessing_strain_rates;

			/**
			 * Are we currently accessing (total) strains (ie, is non-zero integer) ?
			 *
			 * See @a AccessingStrains.
			 */
			mutable int d_accessing_strains;

			/**
			 * Is true if we've generated the deformation total strains (accumulated going forward in time).
			 */
			mutable bool d_have_initialised_strains;


			friend class TopologyReconstruct;

			static
			GeometrySample::non_null_ptr_type
			create_import_sample(
					interpolate_original_points_seq_type &interpolate_original_points,
					const GPlatesMaths::GeometryOnSphere &geometry,
					const PoolAllocator::non_null_ptr_type &pool_allocator,
					boost::optional<double> max_poly_segment_angular_extent_radians);

			template <typename GreatCircleArcIter>
			static
			GeometrySample::non_null_ptr_type
			create_tessellated_poly_import_sample(
					interpolate_original_points_seq_type &interpolate_original_points,
					GreatCircleArcIter great_circle_arcs_begin,
					GreatCircleArcIter great_circle_arcs_end,
					bool is_polygon,
					const PoolAllocator::non_null_ptr_type &pool_allocator,
					const double &max_poly_segment_angular_extent_radians);


			GeometryTimeSpan(
					TopologyReconstruct::non_null_ptr_to_const_type topology_reconstruct,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
					GPlatesModel::integer_plate_id_type reconstruction_plate_id,
					const double &geometry_import_time,
					boost::optional<DeactivatePoint::non_null_ptr_to_const_type> deactivate_points,
					boost::optional<double> max_poly_segment_angular_extent_radians,
					bool deformation_uses_natural_neighbour_interpolation);

			/**
			 * Generate the time windows.
			 */
			void
			initialise_time_windows();

			/**
			 * Reconstructs over a range of time slots from @a start_time_slot to @a end_time_slot (not included).
			 *
			 * Returns none if all the geometry sample's points were deactivated before reaching the end time slot.
			 * This happens when all points get subducted/consumed.
			 * Otherwise returns the geometry sample at the end time slot.
			 */
			boost::optional<GeometrySample::non_null_ptr_type>
			reconstruct_time_steps(
					const GeometrySample::non_null_ptr_type &start_geometry_sample,
					unsigned int start_time_slot,
					unsigned int end_time_slot);

			/**
			 * Same as @a reconstruct_intermediate_time_step except does not test if current geometry sample
			 * is active or not (since need a previous sample to do that).
			 */
			GeometrySample::non_null_ptr_type
			reconstruct_first_time_step(
					const GeometrySample::non_null_ptr_type &current_geometry_sample,
					unsigned int current_time_slot,
					unsigned int next_time_slot);

			/**
			 * Reconstructs @a current_geometry_sample by a single time step from @a current_time_slot to @a next_time_slot.
			 *
			 * Reconstruction can be backward (next_time_slot < current_time_slot) or forward in time (next_time_slot > current_time_slot).
			 *
			 * Rigidly reconstructs using reconstruction plate ID if none of the geometry points intersect topology boundaries/networks.
			 * In other words if the time step is a rigid rotation for all geometry points.
			 *
			 * Returns none if the *current* geometry sample is no longer active.
			 * This happens when all its points get subducted/consumed.
			 * Otherwise returns the next geometry sample.
			 */
			boost::optional<GeometrySample::non_null_ptr_type>
			reconstruct_intermediate_time_step(
					const GeometrySample::non_null_ptr_type &prev_geometry_sample,
					const GeometrySample::non_null_ptr_type &current_geometry_sample,
					unsigned int current_time_slot,
					unsigned int next_time_slot);

			/**
			 * Same as @a reconstruct_intermediate_time_step except does not advance the current geometry sample
			 * to the next time since the current sample is already at one end of the time range.
			 *
			 * Returns false if the *current* geometry sample is no longer active.
			 * This happens when all its points get subducted/consumed.
			 */
			bool
			reconstruct_last_time_step(
					boost::optional<GeometrySample::non_null_ptr_type> prev_geometry_sample,
					const GeometrySample::non_null_ptr_type &current_geometry_sample,
					unsigned int current_time_slot,
					const double &time_increment,
					bool reverse_reconstruct);

			/**
			 * Deforms the specified point in the specified resolved networks.
			 *
			 * Also stores location of point (if successful) and reorders sequence of resolved networks
			 * such that the network containing the point is at the front of the sequence.
			 *
			 * Returns none if point is not in any resolved networks.
			 */
			boost::optional<GPlatesMaths::PointOnSphere>
			reconstruct_point_using_resolved_networks(
					const GPlatesMaths::PointOnSphere &point,
					TopologyPointLocation &location,
					rtn_seq_type &resolved_networks,
					const double &time_increment,
					bool reverse_reconstruct);

			/**
			 * Same as @a reconstruct_point_using_resolved_networks except does not advance the current point
			 * to the next time since the current point is already at one end of the time range.
			 */
			bool
			reconstruct_last_point_using_resolved_networks(
					const GPlatesMaths::PointOnSphere &point,
					TopologyPointLocation &location,
					rtn_seq_type &resolved_networks);

			/**
			 * Reconstructs the specified point in the specified resolved boundaries.
			 *
			 * Also stores location of point (if successful) and reorders sequence of resolved boundaries
			 * such that the boundary containing the point is at the front of the sequence.
			 *
			 * @a resolved_boundary_stage_rotation_map is used as an optimisation to avoid repeating the
			 * calculation of stage rotation when many points are in the same resolved boundary.
			 * 
			 *
			 * Returns none if point is not in any resolved boundaries.
			 */
			boost::optional<GPlatesMaths::PointOnSphere>
			reconstruct_point_using_resolved_boundaries(
					const GPlatesMaths::PointOnSphere &point,
					TopologyPointLocation &location,
					rtb_seq_type &resolved_boundaries,
					plate_id_to_stage_rotation_map_type &resolved_boundary_stage_rotation_map,
					const double &current_time,
					const double &next_time);

			/**
			 * Same as @a reconstruct_point_using_resolved_boundaries except does not advance the current point
			 * to the next time since the current point is already at one end of the time range.
			 */
			bool
			reconstruct_last_point_using_resolved_boundaries(
					const GPlatesMaths::PointOnSphere &point,
					TopologyPointLocation &location,
					rtb_seq_type &resolved_boundaries);

			/**
			 * Return the resolved boundaries/networks in the specified time slot.
			 *
			 * Also removes/culls those resolved boundaries/networks that the specified geometry sample does not intersect.
			 * This is an optimisation that just removes those resolved boundaries/networks that can't possibly intersect.
			 * It doesn't mean the remaining resolved boundaries/networks will definitely intersect though.
			 *
			 * Returns false if there are no resolved boundaries/networks (or all have been culled).
			 */
			bool
			get_resolved_topologies(
					rtb_seq_type &resolved_boundaries,
					rtn_seq_type &resolved_networks,
					unsigned int time_slot,
					const GeometrySample::non_null_ptr_type &geometry_sample) const;

			/**
			 * Calculate the stage rotation from @a initial_time to @a final_time, or
			 * re-use an existing calculation in plate ID map.
			 *
			 * The stage rotation can go forward or backward in time.
			 *
			 * This avoids re-calculating the stage rotation for the same plate ID.
			 */
			const GPlatesMaths::FiniteRotation &
			get_or_create_stage_rotation(
					GPlatesModel::integer_plate_id_type reconstruction_plate_id,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const double &initial_time,
					const double &final_time,
					plate_id_to_stage_rotation_map_type &stage_rotation_map) const;

			/**
			 * Similar to @a get_or_create_stage_rotation except returns a *forward* rotation
			 * used for velocity calculations.
			 *
			 * Note that the stage rotation is going forward in time (old to young).
			 */
			const GPlatesMaths::FiniteRotation &
			get_or_create_velocity_stage_rotation(
					GPlatesModel::integer_plate_id_type reconstruction_plate_id,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const double &reconstruction_time,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type,
					plate_id_to_stage_rotation_map_type &stage_rotation_map) const;

			/**
			 * Rigidly rotates the internal geometry points from present day to @a reconstruction_time, or
			 * vice versa if @a reverse_reconstruct is true.
			 *
			 * If @a pool_alocator is not specified then a new one is created and used for the returned geometry sample.
			 */
			GeometrySample::non_null_ptr_type
			rigid_reconstruct(
					const GeometrySample::non_null_ptr_type &geometry_sample,
					const double &reconstruction_time,
					bool reverse_reconstruct = false,
					boost::optional<PoolAllocator::non_null_ptr_type> pool_allocator = boost::none) const;

			/**
			 * Rigidly rotates the internal geometry points from @a initial_time to @a final_time.
			 *
			 * If @a pool_alocator is not specified then a new one is created and used for the returned geometry sample.
			 */
			GeometrySample::non_null_ptr_type
			rigid_stage_reconstruct(
					const GeometrySample::non_null_ptr_type &geometry_sample,
					const double &initial_time,
					const double &final_time,
					boost::optional<PoolAllocator::non_null_ptr_type> pool_allocator = boost::none) const;

			/**
			 * Rigidly rotates the internal geometry points using @a rotation.
			 *
			 * If @a pool_alocator is not specified then a new one is created and used for the returned geometry sample.
			 */
			GeometrySample::non_null_ptr_type
			rotate_geometry_sample(
					const GeometrySample::non_null_ptr_type &geometry_sample,
					const GPlatesMaths::FiniteRotation &rotation,
					boost::optional<PoolAllocator::non_null_ptr_type> pool_allocator = boost::none) const;

			/**
			 * Generate the deformation accumulated/total strains (accumulated going forward in time).
			 */
			void
			initialise_deformation_total_strains() const;

			/**
			 * Calculate velocities for the specified domain geometry sample.
			 */
			void
			calc_velocities(
					const GeometrySample::non_null_ptr_type &domain_geometry_sample,
					std::vector<GPlatesMaths::Vector3D> &velocities,
					const double &reconstruction_time,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type,
					boost::optional< std::vector<GPlatesMaths::PointOnSphere> &> domain_points,
					boost::optional< std::vector<TopologyPointLocation> &> domain_point_locations) const;

			/**
			 * Returns the geometry sample at the specified time (which can be any time).
			 *
			 * Returns none if all points in the geometry sample have been subducted/consumed
			 * at the reconstruction time (ie, if @a is_valid return false).
			 *
			 * Note that this should be the central access function for geometry samples since
			 * it will ensure deformation (total) strains are initialised (if they're being accessed).
			 */
			boost::optional<GeometrySample::non_null_ptr_type>
			get_geometry_sample(
					const double &reconstruction_time) const;

			/**
			 * Create a new GeometrySample from the closest younger sample by rigid rotation.
			 */
			GeometrySample::non_null_ptr_type
			create_rigid_geometry_sample(
					const double &reconstruction_time,
					const double &closest_younger_sample_time,
					const GeometrySample::non_null_ptr_type &closest_younger_sample) const;

			/**
			 * Interpolate two geometry samples in adjacent time slots.
			 */
			GeometrySample::non_null_ptr_type
			interpolate_geometry_sample(
					const double &interpolate_position,
					const double &first_geometry_time,
					const double &second_geometry_time,
					const GeometrySample::non_null_ptr_type &first_geometry_sample,
					const GeometrySample::non_null_ptr_type &second_geometry_sample) const;
		};

	private:

		TimeSpanUtils::TimeRange d_time_range;
		resolved_boundary_time_span_type::non_null_ptr_to_const_type d_resolved_boundary_time_span;
		resolved_network_time_span_type::non_null_ptr_to_const_type d_resolved_network_time_span;
		ReconstructionTreeCreator d_reconstruction_tree_creator;


		TopologyReconstruct(
				const TimeSpanUtils::TimeRange &time_range,
				const resolved_boundary_time_span_type::non_null_ptr_to_const_type &resolved_boundary_time_span,
				const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
				const ReconstructionTreeCreator &reconstruction_tree_creator) :
			d_time_range(time_range),
			d_resolved_boundary_time_span(resolved_boundary_time_span),
			d_resolved_network_time_span(resolved_network_time_span),
			d_reconstruction_tree_creator(reconstruction_tree_creator)
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYRECONSTRUCT_H
