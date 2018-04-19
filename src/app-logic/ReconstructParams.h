/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTPARAMS_H
#define GPLATES_APP_LOGIC_RECONSTRUCTPARAMS_H

#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "maths/types.h"

#include "property-values/GeoTimeInstant.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesAppLogic
{
	/**
	 * ReconstructParams is used to store additional parameters for reconstructing
	 * features into @a ReconstructedFeatureGeometry objects.
	 */
	class ReconstructParams :
			public boost::less_than_comparable<ReconstructParams>,
			public boost::equality_comparable<ReconstructParams>
	{
	public:

		enum VGPVisibilitySetting
		{
			ALWAYS_VISIBLE, /**< all vgps are displayed at all times */
			TIME_WINDOW, /**< all vgps are displayed between a specified time interval */
			DELTA_T_AROUND_AGE /**< vgps are displayed if the reconstruction time is within a time window around the VGP's age */

			// NOTE: Any new values should also be added to @a transcribe.
		};

		ReconstructParams();


		/**
		 * Sets whether we reconstruct by-plate-id outside the feature's active time period.
		 *
		 * The constructor defaults the value to 'false'.
		 *
		 * If this is set to 'true' then features that are reconstructed by-plate-id (ie, using
		 * ReconstructMethod::BY_PLATE_ID) will be reconstructed *both* inside and outside their
		 * active time period - the time range over which the geological feature actually exists.
		 *
		 * This is currently used by raster reconstruction when assisted by an age grid because
		 * we don't want the ocean floor polygons to disappear (because the age grid determines
		 * when the ocean floor disappears, but we still need the polygon regions and hence still
		 * need the reconstructed polygons).
		 *
		 * This might also be useful for seed/target data co-registration to co-register data
		 * that has already been subducted.
		 *
		 * TODO: Add an expansion of the active time range (eg, [5,10] -> [4,11] if expanding +/-1).
		 */
		void
		set_reconstruct_by_plate_id_outside_active_time_period(
				bool reconstruct_outside_active_time_period)
		{
			d_reconstruct_by_plate_id_outside_active_time_period = reconstruct_outside_active_time_period;
		}

		bool
		get_reconstruct_by_plate_id_outside_active_time_period() const
		{
			return d_reconstruct_by_plate_id_outside_active_time_period;
		}

		VGPVisibilitySetting
		get_vgp_visibility_setting() const
		{
			return d_vgp_visibility_setting;
		}

		void
		set_vgp_visibility_setting(
				VGPVisibilitySetting setting)
		{
			d_vgp_visibility_setting = setting;
		}

		const GPlatesPropertyValues::GeoTimeInstant &
		get_vgp_earliest_time() const
		{
			return d_vgp_earliest_time;	
		};

		void
		set_vgp_earliest_time(
				const GPlatesPropertyValues::GeoTimeInstant &earliest_time)
		{	
			d_vgp_earliest_time = earliest_time;
		}

		const GPlatesPropertyValues::GeoTimeInstant &
		get_vgp_latest_time() const
		{
			return d_vgp_latest_time;	
		};

		void
		set_vgp_latest_time(
				const GPlatesPropertyValues::GeoTimeInstant &latest_time)
		{	
			d_vgp_latest_time = latest_time;
		};

		double
		get_vgp_delta_t() const
		{
			return d_vgp_delta_t.dval();
		}

		void
		set_vgp_delta_t(
				double vgp_delta_t)
		{	
			d_vgp_delta_t = vgp_delta_t;
		}

		bool
		should_draw_vgp(
				double current_time,
				const boost::optional<double> &age) const;


		bool
		get_reconstruct_using_topologies() const
		{
			return d_reconstruct_using_topologies;
		}

		void
		set_reconstruct_using_topologies(
				bool reconstruct_using_topologies)
		{
			d_reconstruct_using_topologies = reconstruct_using_topologies;
		}


		//
		// Methods for topology-reconstructed parameters.
		//

		double 
		get_topology_reconstruction_end_time() const
		{
			return d_topology_reconstruction_end_time.dval();
		}

		void
		set_topology_reconstruction_end_time(
				double deformation_end_time)
		{
			d_topology_reconstruction_end_time = deformation_end_time;
		}


		double 
		get_topology_reconstruction_begin_time() const
		{
			return d_topology_reconstruction_begin_time.dval();
		}

		void
		set_topology_reconstruction_begin_time(
				double deformation_begin_time)
		{
			d_topology_reconstruction_begin_time = deformation_begin_time;
		}


		double 
		get_topology_reconstruction_time_increment() const
		{
			return d_topology_reconstruction_time_increment.dval();
		}

		void
		set_topology_reconstruction_time_increment(
				double time_inc)
		{
			d_topology_reconstruction_time_increment = time_inc;
		}


		bool
		get_topology_deformation_use_natural_neighbour_interpolation() const
		{
			return d_topology_deformation_use_natural_neighbour_interpolation;
		}

		void
		set_topology_deformation_use_natural_neighbour_interpolation(
				bool use_natural_neighbour_interpolation)
		{
			d_topology_deformation_use_natural_neighbour_interpolation = use_natural_neighbour_interpolation;
		}


		/**
		 * Use a feature's time of appearance instead of its 'gpml:geometryImportTime' as the starting
		 * time for topology reconstruction.
		 */
		bool
		get_topology_reconstruction_use_time_of_appearance() const
		{
			return d_topology_reconstruction_use_time_of_appearance;
		}

		void
		set_topology_reconstruction_use_time_of_appearance(
				bool use_time_of_appearance)
		{
			d_topology_reconstruction_use_time_of_appearance = use_time_of_appearance;
		}


		bool
		get_topology_reconstruction_enable_line_tessellation() const
		{
			return d_topology_reconstruction_enable_line_tessellation;
		}

		void
		set_topology_reconstruction_enable_line_tessellation(
				bool enable_line_tessellation)
		{
			d_topology_reconstruction_enable_line_tessellation = enable_line_tessellation;
		}

		const double &
		get_topology_reconstruction_line_tessellation_degrees() const
		{
			return d_topology_reconstruction_line_tessellation_degrees.dval();
		}

		void
		set_topology_reconstruction_line_tessellation_degrees(
				const double &line_tessellation_degrees)
		{
			d_topology_reconstruction_line_tessellation_degrees = line_tessellation_degrees;
		}


		bool
		get_topology_reconstruction_enable_lifetime_detection() const
		{
			return d_topology_reconstruction_enable_lifetime_detection;
		}

		void
		set_topology_reconstruction_enable_lifetime_detection(
				bool enable_lifetime_detection)
		{
			d_topology_reconstruction_enable_lifetime_detection = enable_lifetime_detection;
		}

		const double &
		get_topology_reconstruction_lifetime_detection_threshold_velocity_delta() const
		{
			return d_topology_reconstruction_lifetime_detection_threshold_velocity_delta.dval();
		}

		void
		set_topology_reconstruction_lifetime_detection_threshold_velocity_delta(
				const double &lifetime_detection_threshold_velocity_delta)
		{
			d_topology_reconstruction_lifetime_detection_threshold_velocity_delta = lifetime_detection_threshold_velocity_delta;
		}

		const double &
		get_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary() const
		{
			return d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary.dval();
		}

		void
		set_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary(
				const double &lifetime_detection_threshold_distance_to_boundary)
		{
			d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary = lifetime_detection_threshold_distance_to_boundary;
		}

		bool
		get_topology_reconstruction_deactivate_points_that_fall_outside_a_network() const
		{
			return d_topology_reconstruction_deactivate_points_that_fall_outside_a_network;
		}

		void
		set_topology_reconstruction_deactivate_points_that_fall_outside_a_network(
				bool deactivate_points_that_fall_outside_a_network)
		{
			d_topology_reconstruction_deactivate_points_that_fall_outside_a_network = deactivate_points_that_fall_outside_a_network;
		}


		//! Equality comparison operator.
		bool
		operator==(
				const ReconstructParams &rhs) const;

		//! Less than comparison operator.
		bool
		operator<(
				const ReconstructParams &rhs) const;

	private:

		static const double INITIAL_VGP_DELTA_T;
		static const double INITIAL_TIME_RANGE_END;
		static const double INITIAL_TIME_RANGE_BEGIN;
		static const double INITIAL_TIME_RANGE_INCREMENT;
		static const double INITIAL_LINE_TESSELLATION_DEGREES;


		/**
		 * Do we reconstruct by-plate-id outside the feature's active time period.
		 */
		bool d_reconstruct_by_plate_id_outside_active_time_period;

		/**
		 * Enum indicating what sort of VGP visibility we have.                                                                
		 */
		VGPVisibilitySetting d_vgp_visibility_setting;

		/**
		 * Begin time used when the TIME_WINDOW VGPVisibilitySetting is selected.                                                                    
		 */
		GPlatesPropertyValues::GeoTimeInstant d_vgp_earliest_time;

		/**
		 * End time used when the TIME_WINDOW VGPVisibilitySetting is selected.                                                                    
		 */
		GPlatesPropertyValues::GeoTimeInstant d_vgp_latest_time;

		/**
		 * Delta used for time window around VGP age.                                                                     
		 */
		GPlatesMaths::real_t d_vgp_delta_t;

		/**
		 * Whether to reconstruct using topologies.
		 */
		bool d_reconstruct_using_topologies;

		//
		// Topology reconstruction parameters.
		//

		GPlatesMaths::real_t d_topology_reconstruction_end_time;
		GPlatesMaths::real_t d_topology_reconstruction_begin_time;
		GPlatesMaths::real_t d_topology_reconstruction_time_increment;

		bool d_topology_deformation_use_natural_neighbour_interpolation;
		bool d_topology_reconstruction_use_time_of_appearance;

		bool d_topology_reconstruction_enable_line_tessellation;
		GPlatesMaths::real_t d_topology_reconstruction_line_tessellation_degrees;

		bool d_topology_reconstruction_enable_lifetime_detection;
		GPlatesMaths::real_t d_topology_reconstruction_lifetime_detection_threshold_velocity_delta;
		GPlatesMaths::real_t d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary;
		bool d_topology_reconstruction_deactivate_points_that_fall_outside_a_network;

	private: // Transcribe for sessions/projects...

		friend class GPlatesScribe::Access;

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};


	/**
	 * Transcribe for sessions/projects.
	 */
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			ReconstructParams::VGPVisibilitySetting &vgp_visibility_setting,
			bool transcribed_construct_data);
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTPARAMS_H
