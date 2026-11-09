/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYNETWORKPARAMS_H
#define GPLATES_APP_LOGIC_TOPOLOGYNETWORKPARAMS_H

#include <cmath>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesAppLogic
{
	/**
	 * TopologyNetworkParams is used to store additional parameters for resolving topological networks
	 * and associated attributes in @a TopologyNetworkLayerTask layers.
	 */
	class TopologyNetworkParams :
			public boost::less_than_comparable<TopologyNetworkParams>,
			public boost::equality_comparable<TopologyNetworkParams>
	{
	public:

		enum StrainRateSmoothing
		{
			NO_SMOOTHING,
			BARYCENTRIC_SMOOTHING,
			NATURAL_NEIGHBOUR_SMOOTHING
		};


		/**
		 * Strain rate clamping parameters.
		 */
		struct StrainRateClamping :
				public boost::less_than_comparable<StrainRateClamping>,
				public boost::equality_comparable<StrainRateClamping>
		{
			StrainRateClamping();

			//! Equality comparison operator.
			bool
			operator==(
					const StrainRateClamping &rhs) const;

			//! Less than comparison operator.
			bool
			operator<(
					const StrainRateClamping &rhs) const;

			//! Is strain rate clamping enabled.
			bool enable_clamping;
			//! Maximum strain rate (if clamping is enabled).
			double max_total_strain_rate;

		private: // Transcribe for sessions/projects...

			friend class GPlatesScribe::Access;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);
		};


		/**
		 * Rift parameters for networks that are rifts.
		 *
		 * A network is a rift if the network feature has rift left/right plate IDs.
		 */
		struct RiftParams :
				public boost::less_than_comparable<RiftParams>,
				public boost::equality_comparable<RiftParams>
		{
			RiftParams();

			//! Equality comparison operator.
			bool
			operator==(
					const RiftParams &rhs) const;

			//! Less than comparison operator.
			bool
			operator<(
					const RiftParams &rhs) const;

			/**
			 * The range of @a edge_length_threshold_degrees.
			 *
			 * The threshold bounds the recursive subdivision of rift edges (its depth is about
			 * log2(edge length / threshold)), so zero would never stop and a tiny threshold is bounded
			 * but huge. The minimum (about 110 metres, 100 times finer than the default 0.1 degrees)
			 * limits the depth to about 18 levels.
			 */
			static constexpr double MIN_EDGE_LENGTH_THRESHOLD_DEGREES = 0.001;
			static constexpr double MAX_EDGE_LENGTH_THRESHOLD_DEGREES = 180.0;

			/**
			 * The largest @a exponential_stretching_constant (the smallest is any positive value).
			 *
			 * The same as GPlates' layer options allow. Beyond about 709, exp() overflows in the rift
			 * maths, which then gives NaN vertices.
			 */
			static constexpr double MAX_EXPONENTIAL_STRETCHING_CONSTANT = 100.0;

			/**
			 * Whether each parameter is in range, wherever it comes from (the pyGPlates API, or a
			 * network feature's own properties). Each test also rejects NaN.
			 */
			static
			bool
			is_valid_exponential_stretching_constant(
					const double &exponential_stretching_constant)
			{
				return exponential_stretching_constant > 0 &&
						exponential_stretching_constant <= MAX_EXPONENTIAL_STRETCHING_CONSTANT;
			}

			static
			bool
			is_valid_strain_rate_resolution(
					const double &strain_rate_resolution)
			{
				return strain_rate_resolution > 0 && std::isfinite(strain_rate_resolution);
			}

			static
			bool
			is_valid_edge_length_threshold_degrees(
					const double &edge_length_threshold_degrees)
			{
				return edge_length_threshold_degrees >= MIN_EDGE_LENGTH_THRESHOLD_DEGREES &&
						edge_length_threshold_degrees <= MAX_EDGE_LENGTH_THRESHOLD_DEGREES;
			}

			/**
			 * Default stretching profile is exp(exponential_stretching_constant * x).
			 *
			 * Must be positive (and at most MAX_EXPONENTIAL_STRETCHING_CONSTANT): the profile is
			 * (exp(c*t) - 1) / (exp(c) - 1), with t = 0 on the unstretched
			 * side and t = 1 at the rift axis, so its strain rate increases towards the axis for c > 0
			 * (as intended). As c approaches zero the strain rate becomes constant across the rift
			 * (which the profile exists to avoid), and c < 0 concentrates the strain away from the axis.
			 */
			double exponential_stretching_constant;
			//! Adjacent strain rates samples should resolved within this tolerance (in units 1/sec).
			double strain_rate_resolution;
			//! An edge should not be subdivided if it is shorter than this length (in degrees).
			double edge_length_threshold_degrees;

		private: // Transcribe for sessions/projects...

			friend class GPlatesScribe::Access;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);
		};


		TopologyNetworkParams();


		StrainRateSmoothing
		get_strain_rate_smoothing() const
		{
			return d_strain_rate_smoothing;
		}

		void
		set_strain_rate_smoothing(
				StrainRateSmoothing strain_rate_smoothing)
		{
			d_strain_rate_smoothing = strain_rate_smoothing;
		}


		const StrainRateClamping &
		get_strain_rate_clamping() const
		{
			return d_strain_rate_clamping;
		}

		void
		set_strain_rate_clamping(
				const StrainRateClamping &strain_rate_clamping)
		{
			d_strain_rate_clamping = strain_rate_clamping;
		}


		const RiftParams &
		get_rift_params() const
		{
			return d_rift_params;
		}

		void
		set_rift_params(
				const RiftParams &rift_params)
		{
			d_rift_params = rift_params;
		}


		//! Equality comparison operator.
		bool
		operator==(
				const TopologyNetworkParams &rhs) const;

		//! Less than comparison operator.
		bool
		operator<(
				const TopologyNetworkParams &rhs) const;

	private:

		/**
		 * Strain rates (units 1/second) are typically much smaller than GPlatesMaths::EPSILON and
		 * so we need to scale them before using the comparison functionality of GPlatesMaths::Real.
		 */
		static const double COMPARE_STRAIN_RATE_SCALE;

		/**
		 * Whether, and how, to smooth the deformation strain rates.
		 */
		StrainRateSmoothing d_strain_rate_smoothing;

		/**
		 * Whether, and how much, to clamp the deformation strain rates.
		 */
		StrainRateClamping d_strain_rate_clamping;

		/**
		 * Rift parameters for networks that are rifts.
		 */
		RiftParams d_rift_params;

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
			TopologyNetworkParams::StrainRateSmoothing &strain_rate_smoothing,
			bool transcribed_construct_data);
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYNETWORKPARAMS_H
