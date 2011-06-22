/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_UTILS_ANIMATIONSEQUENCEUTILS_H
#define GPLATES_UTILS_ANIMATIONSEQUENCEUTILS_H

#include <cstddef>	// std::size_t
#include "global/GPlatesException.h"


namespace GPlatesUtils
{
	/**
	 * This namespace contains functions for calculating various aspects of an
	 * animation sequence. It is the canonical location for such calculations,
	 * ensuring that ExportTemplateFilenameSequence can arrive at the same
	 * results as AnimationController when calculating number of frames, etc.
	 */
	namespace AnimationSequence
	{
		/**
		 * Typedef for frame indexes and sequence durations.
		 */
		typedef std::size_t size_type;

	
		/**
		 * Struct to act as the return value from the @a calculate_sequence()
		 * function.
		 */
		struct SequenceInfo
		{
			double desired_start_time;
			double desired_end_time;
			double abs_time_increment;
			double raw_time_increment;
			bool should_finish_exactly_on_end_time;
			
			size_type duration_in_frames;
			double duration_in_ma;
			bool includes_remainder_frame;
			double remainder_frame_length;
			double actual_start_time;
			double actual_end_time;
		};
		
		
		/**
		 * Exception thrown by @a calculate_sequence() when given
		 * time increment is zero.
		 */
		class TimeIncrementZero : public GPlatesGlobal::Exception
		{
		public:
			explicit
			TimeIncrementZero(
					const GPlatesUtils::CallStack::Trace &src) :
				Exception(src)
			{  }

		protected:
			virtual
			const char *
			exception_name() const
			{
				return "AnimationSequence::TimeIncrementZero";
			}
		};


		/**
		 * Calculates everything you might want to know about a given animation
		 * sequence in one handy pass.
		 *
		 * @throws TimeIncrementZero if reconstruction time increment is zero.
		 */
		SequenceInfo
		calculate_sequence(
				const double &start_time,
				const double &end_time,
				const double &abs_time_increment,
				bool should_finish_exactly_on_end_time);
		
		
		/**
		 * Adjusts an absolute-value time increment to be positive or negative,
		 * appropriate for iterating through the given range.
		 */
		double
		raw_time_increment(
				const double &start_time,
				const double &end_time,
				const double &abs_time_increment);


		/**
		 * Calculates the appropriate reconstruction time for the given
		 * SequenceInfo and frame index (starts at 0).
		 *
		 * This takes into account that the last frame may be shorter
		 * than the others.
		 */
		double
		calculate_time_for_frame(
				const SequenceInfo &sequence_info,
				const size_type &frame_index);



	}
}

#endif	// GPLATES_UTILS_ANIMATIONSEQUENCEUTILS_H

