/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYNETROTATION_H
#define GPLATES_API_PYNETROTATION_H


#include <vector>
#include <boost/optional.hpp>
#include <QString>

#include "PyTopologicalModel.h"
#include "PyTopologicalSnapshot.h"

#include "app-logic/NetRotationUtils.h"
#include "app-logic/VelocityDeltaTime.h"

#include "global/python.h"

#include "scribe/ScribeLoadRef.h"
// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"

#include "utils/ReferenceCount.h"


namespace GPlatesApi
{
	/**
	 * Net rotation of dynamic plates and deforming networks associated with a topological snapshot (at a specific time).
	 */
	class NetRotationSnapshot :
			public GPlatesUtils::ReferenceCount<NetRotationSnapshot>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<NetRotationSnapshot> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const NetRotationSnapshot> non_null_ptr_to_const_type;


		//! Default number of grid points sampled along each meridian.
		static const unsigned int DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN = 180;


		/**
		 * Create a net rotation snapshot to be used with the specified topological snapshot.
		 */
		static
		non_null_ptr_type
		create(
				TopologicalSnapshot::non_null_ptr_type topological_snapshot,
				const double &velocity_delta_time,
				GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
				const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution);


		/**
		 * Get the topological snapshot.
		 */
		TopologicalSnapshot::non_null_ptr_type
		get_topological_snapshot() const
		{
			return d_topological_snapshot;
		}

		/**
		 * Return the net rotation calculator.
		 */
		const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator &
		get_net_rotation_calculator() const
		{
			return d_net_rotation_calculator;
		}

	private:

		//! Topological snapshot to obtain net rotation from at requested reconstruction times.
		TopologicalSnapshot::non_null_ptr_type d_topological_snapshot;
		//! Calculates net rotation at our reconstruction time.
		GPlatesAppLogic::NetRotationUtils::NetRotationCalculator d_net_rotation_calculator;


		NetRotationSnapshot(
				TopologicalSnapshot::non_null_ptr_type topological_snapshot,
				const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_boundary_seq_type &resolved_topological_boundaries,
				const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_network_seq_type &resolved_topological_networks,
				const double &velocity_delta_time,
				GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
				const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution);

	private: // Transcribe...

		friend class GPlatesScribe::Access;

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<NetRotationSnapshot> &net_rotation_snapshot);

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);

		static
		void
		save_construct_data(
				GPlatesScribe::Scribe &scribe,
				const NetRotationSnapshot &net_rotation_snapshot);

		static
		bool
		load_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::LoadRef<TopologicalSnapshot::non_null_ptr_type> &topological_snapshot,
				GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_boundary_seq_type &resolved_topological_boundaries,
				GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_network_seq_type &resolved_topological_networks,
				double &velocity_delta_time,
				GPlatesAppLogic::VelocityDeltaTime::Type &velocity_delta_time_type,
				GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution);
	};


	/**
	 * Net rotation of dynamic plates and deforming networks associated with a topological model (over all times).
	 */
	class NetRotationModel :
			public GPlatesUtils::ReferenceCount<NetRotationModel>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<NetRotationModel> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const NetRotationModel> non_null_ptr_to_const_type;


		/**
		 * Create a net rotation model to be used with the specified topological model.
		 */
		static
		non_null_ptr_type
		create(
				TopologicalModel::non_null_ptr_type topological_model,
				const double &velocity_delta_time,
				GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
				const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution);


		/**
		 * Get the topological model.
		 */
		TopologicalModel::non_null_ptr_type
		get_topological_model() const
		{
			return d_topological_model;
		}


		/**
		 * Get a net rotation snapshot at the specified time.
		 */
		NetRotationSnapshot::non_null_ptr_type
		create_net_rotation_snapshot(
				const double &reconstruction_time) const;

	private:

		/**
		 * Topological model to obtain net rotation from at requested reconstruction times.
		 */
		TopologicalModel::non_null_ptr_type d_topological_model;

		double d_velocity_delta_time;
		GPlatesAppLogic::VelocityDeltaTime::Type d_velocity_delta_time_type;
		//! How the points, to calculate net rotation, are distributed across the globe.
		GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type d_point_distribution;


		NetRotationModel(
				TopologicalModel::non_null_ptr_type topological_model,
				const double &velocity_delta_time,
				GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
				const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution);

	private: // Transcribe...

		friend class GPlatesScribe::Access;

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<NetRotationModel> &net_rotation);

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);

		static
		void
		save_construct_data(
				GPlatesScribe::Scribe &scribe,
				const NetRotationModel &net_rotation);

		static
		bool
		load_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::LoadRef<TopologicalModel::non_null_ptr_type> &topological_model,
				double &velocity_delta_time,
				GPlatesAppLogic::VelocityDeltaTime::Type &velocity_delta_time_type,
				GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution);
	};
}

#endif // GPLATES_API_PYNETROTATION_H
