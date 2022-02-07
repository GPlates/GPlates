/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONPARAMS_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONPARAMS_H

#include <boost/operators.hpp>
#include <boost/optional.hpp>

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesAppLogic
{
	/**
	 * ReconstructionParams is used to store additional parameters for calculating
	 * reconstruction trees in @a ReconstructionLayerTask layers.
	 */
	class ReconstructionParams :
			public boost::less_than_comparable<ReconstructionParams>,
			public boost::equality_comparable<ReconstructionParams>
	{
	public:

		ReconstructionParams();

		/**
		 * Whether each moving plate rotation sequence is extended back to the distant past such that
		 * reconstructed geometries are not snapped back to their present day positions.
		 */
		bool
		get_extend_total_reconstruction_poles_to_distant_past() const
		{
			return d_extend_total_reconstruction_poles_to_distant_past;
		}

		void
		set_extend_total_reconstruction_poles_to_distant_past(
				bool extend_total_reconstruction_poles_to_distant_past)
		{
			d_extend_total_reconstruction_poles_to_distant_past = extend_total_reconstruction_poles_to_distant_past;
		}


		//! Equality comparison operator.
		bool
		operator==(
				const ReconstructionParams &rhs) const;

		//! Less than comparison operator.
		bool
		operator<(
				const ReconstructionParams &rhs) const;

	private:
		bool d_extend_total_reconstruction_poles_to_distant_past;

	private: // Transcribe for sessions/projects...

		friend class GPlatesScribe::Access;

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONPARAMS_H
