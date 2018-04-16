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
			StrainRateClamping(
					// Strain rate clamping is disabled by default...
					bool enable_clamping_ = false,
					// Default maximum strain rate (if clamping is enabled)...
					const double &max_total_strain_rate_ = 5e-15) :
				enable_clamping(enable_clamping_),
				max_total_strain_rate(max_total_strain_rate_)
			{  }

			//! Equality comparison operator.
			bool
			operator==(
					const StrainRateClamping &rhs) const;

			//! Less than comparison operator.
			bool
			operator<(
					const StrainRateClamping &rhs) const;

			bool enable_clamping;
			double max_total_strain_rate;

		private:
			/**
			 * Strain rates (units 1/second) are typically much smaller than GPlatesMaths::EPSILON and
			 * so we need to scale them before using the comparison functionality of GPlatesMaths::Real.
			 */
			static const double COMPARE_STRAIN_RATE_SCALE;

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
		 * Whether, and how, to smooth the deformation strain rates.
		 */
		StrainRateSmoothing d_strain_rate_smoothing;

		/**
		 * Whether, and how much, to clamp the deformation strain rates.
		 */
		StrainRateClamping d_strain_rate_clamping;

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
