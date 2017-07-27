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

#ifndef GPLATES_APP_LOGIC_VELOCITYDELTATIME_H
#define GPLATES_APP_LOGIC_VELOCITYDELTATIME_H

#include <utility>

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesAppLogic
{
	namespace VelocityDeltaTime
	{
		/**
		 * The time range (given a delta time) relative to a specific time that a velocity is calculated using.
		 */
		enum Type
		{
            T_PLUS_DELTA_T_TO_T,       // (t + delta_t -> t)
            T_TO_T_MINUS_DELTA_T,      // (t -> t - delta_t)
			T_PLUS_MINUS_HALF_DELTA_T, // (t + delta_t/2 -> t - delta_t/2)

			// NOTE: Any new values should also be added to @a transcribe.

			NUM_TYPES                  // this must be last
		};


		/**
		 * Returns the time range giving a time, delta time and delta time type.
		 *
		 * The first time in the returned pair is older (larger) than the second time.
		 *
		 * If @a allow_negative_range is true (the default) then the returned time range can include
		 * negative times, otherwise if the younger time is negative then the returned range is
		 * (delta_time, 0).
		 * In general it's probably better to allow negative times because if the rotation file does not
		 * include rotations for negative times then the velocities will be zero due to not finding
		 * the plate ID in rotation sequence for negative times. Also there may be some rare users
		 * who have rotations into the future (ie, negative reconstruction times).
		 */
		std::pair<double, double>
		get_time_range(
				Type delta_time_type,
				const double &time,
				const double &delta_time = 1.0,
				bool allow_negative_range = true);


		/**
		 * Transcribe for sessions/projects.
		 */
		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				Type &velocity_delta_time,
				bool transcribed_construct_data);
	}
}

#endif // GPLATES_APP_LOGIC_VELOCITYDELTATIME_H
