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

#ifndef GPLATES_APP_LOGIC_TIMESPANUTILS_H
#define GPLATES_APP_LOGIC_TIMESPANUTILS_H

#include <cmath>
#include <deque>
#include <list>
#include <vector>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	namespace TimeSpanUtils
	{
		/**
		 * A time range consisting of time slots where the following constraints hold:
		 *
		 *   begin_time = end_time + num_time_slots * time_increment
		 *   num_time_slots >= 2
		 */
		class TimeRange
		{
		public:

			/**
			 * Whether to adjust begin time, end time or time increment such that the constraints hold:
			 *
			 *   begin_time = end_time + num_time_slots * time_increment
			 *   num_time_slots >= 2
			 */
			enum Adjust
			{
				ADJUST_BEGIN_TIME,
				ADJUST_END_TIME,
				ADJUST_TIME_INCREMENT
			};


			/**
			 * Create a time range and adjust, if necessary, @a begin_time, @a end_time or
			 * @a time_increment depending on @a adjust.
			 *
			 * Throws exception if @a time_increment is zero or negative, or if
			 * @a end_time is less than or equal to @a begin_time.
			 */
			TimeRange(
					const double &begin_time,
					const double &end_time,
					const double &time_increment,
					Adjust adjust);

			/**
			 * Create a time range where the time increment is:
			 *
			 *   time_increment = (begin_time - end_time) / num_time_slots
			 *
			 * Throws exception if @a num_time_slots is less than two, or if
			 * @a end_time is less than or equal to @a begin_time.
			 */
			TimeRange(
					const double &begin_time,
					const double &end_time,
					unsigned int num_time_slots);


			//! Returns the begin time of the time range.
			const double &
			get_begin_time() const
			{
				return d_begin_time;
			}


			//! Returns the end time of the time range.
			const double &
			get_end_time() const
			{
				return d_end_time;
			}


			//! Returns the time increment of the time range.
			const double &
			get_time_increment() const
			{
				return d_time_increment;
			}


			/**
			 * Returns the number of time slots in the time range.
			 *
			 * The begin time and end time each correspond to a time slot.
			 *
			 * NOTE: There will always be at least two times slots (one time interval).
			 */
			unsigned int
			get_num_time_slots() const
			{
				return d_num_time_slots;
			}


			/**
			 * Returns the number of time intervals in the time range.
			 *
			 * This is the number of time slots minus one.
			 *
			 * NOTE: There will always be at least one time interval.
			 */
			unsigned int
			get_num_time_intervals() const
			{
				return d_num_time_slots - 1;
			}


			/**
			 * Returns the time associated with the specified time slot.
			 *
			 * Time slots begin at the begin time and end at the end time.
			 */
			double
			get_time(
					unsigned int time_slot) const;


			/**
			 * Returns the matching time slot if the specified time matches (within epsilon) the time of a time slot.
			 */
			boost::optional<unsigned int>
			get_time_slot(
					const double &time) const;


			/**
			 * Returns the nearest time slot for the specified time.
			 *
			 * Returns none if @a time is outside the time range [@a get_begin_time, @a get_end_time].
			 */
			boost::optional<unsigned int>
			get_nearest_time_slot(
					const double &time) const;

		private:

			double d_begin_time;
			double d_end_time;
			double d_time_increment;
			unsigned int d_num_time_slots;


			/**
			 * Returns the number of time slots rounded up to the nearest integer.
			 */
			static
			unsigned int
			calc_num_time_slots(
					const double &begin_time,
					const double &end_time,
					const double &time_increment);
		};


		/**
		 * Interface to look samples of 'T' over a time range.
		 */
		template <typename T>
		class TimeSpan :
				public GPlatesUtils::ReferenceCount< TimeSpan<T> >
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a TimeSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<TimeSpan> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a TimeSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<const TimeSpan> non_null_ptr_to_const_type;


			//! Typedef for the object type in each sample.
			typedef T sample_type;


			virtual
			~TimeSpan()
			{  }


			/**
			 * Returns the time range of the time span.
			 */
			virtual
			TimeRange
			get_time_range() const = 0;


			/**
			 * Returns true if @a set_sample_in_time_slot has not been called for any time slots.
			 */
			virtual
			bool
			empty() const = 0;


			/**
			 * Set the sample for the specified time slot.
			 *
			 * The number of time slots is available in the TimeRange returned by @a get_time_range.
			 *
			 * Throws exception if time_slot >= get_time_range().get_num_time_slots()
			 */
			virtual
			void
			set_sample_in_time_slot(
					const T &sample,
					unsigned int time_slot) = 0;


			/**
			 * Get the sample for the specified time slot.
			 *
			 * Returns none if @a set_sample_in_time_slot has not yet been called for @a time_slot.
			 *
			 * The number of time slots is available in the TimeRange returned by @a get_time_range.
			 *
			 * Throws exception if time_slot >= get_time_range().get_num_time_slots()
			 */
			virtual
			boost::optional<const T &>
			get_sample_in_time_slot(
					unsigned int time_slot) const = 0;


			/**
			 * Non-const overload.
			 *
			 * Note: Returns none if @a set_sample_in_time_slot has not yet been called for @a time_slot.
			 */
			virtual
			boost::optional<T &>
			get_sample_in_time_slot(
					unsigned int time_slot) = 0;


			/**
			 * Get the sample in the nearest time slot for the specified time.
			 *
			 * Returns none if @a time is outside the range of the TimeRange returned by @a get_time_range.
			 */
			boost::optional<const T &>
			get_nearest_sample_at_time(
					const double &time) const;


			/**
			 * Non-const overload.
			 */
			boost::optional<T &>
			get_nearest_sample_at_time(
					const double &time);

		};


		/**
		 * A look up table of samples of 'T' over a time span.
		 */
		template <typename T>
		class TimeSampleSpan :
				public TimeSpan<T>
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a TimeSampleSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<TimeSampleSpan> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a TimeSampleSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<const TimeSampleSpan> non_null_ptr_to_const_type;


			/**
			 * Allocate a look up table with as many slots as there are in @a time_range.
			 *
			 * Note: Each time slot is initialised as T(), so type T must have a default constructor
			 * (eg, std::vector, boost::optional, etc).
			 */
			static
			non_null_ptr_type
			create(
					const TimeRange &time_range)
			{
				return non_null_ptr_type(new TimeSampleSpan(time_range));
			}


			/**
			 * Returns the time range of the time span.
			 */
			virtual
			TimeRange
			get_time_range() const
			{
				return d_time_range;
			}


			/**
			 * Returns true if @a set_sample_in_time_slot has not been called for any time slots.
			 */
			virtual
			bool
			empty() const
			{
				return d_is_empty;
			}


			/**
			 * Set the sample for the specified time slot.
			 *
			 * The number of time slots is available in the TimeRange returned by @a get_time_range.
			 *
			 * Throws exception if time_slot >= get_time_range().get_num_time_slots()
			 */
			virtual
			void
			set_sample_in_time_slot(
					const T &sample,
					unsigned int time_slot);


			/**
			 * Get the sample for the specified time slot.
			 *
			 * Returns none if @a set_sample_in_time_slot has not yet been called for @a time_slot.
			 *
			 * The number of time slots is available in the TimeRange returned by @a get_time_range.
			 *
			 * Throws exception if time_slot >= get_time_range().get_num_time_slots()
			 */
			virtual
			boost::optional<const T &>
			get_sample_in_time_slot(
					unsigned int time_slot) const;


			/**
			 * Non-const overload.
			 *
			 * Note: Returns none if @a set_sample_in_time_slot has not yet been called for @a time_slot.
			 */
			virtual
			boost::optional<T &>
			get_sample_in_time_slot(
					unsigned int time_slot);

		private:

			//! Typedef for a time sequence of samples.
			typedef std::vector< boost::optional<T> > sample_time_seq_type;


			TimeRange d_time_range;
			sample_time_seq_type d_sample_time_sequence;
			bool d_is_empty;


			explicit
			TimeSampleSpan(
					const TimeRange &time_range) :
				d_time_range(time_range),
				// Allocate and initialise empty slots...
				d_sample_time_sequence(time_range.get_num_time_slots()),
				d_is_empty(true)
			{  }
		};


		/**
		 * A look up table of samples of 'T' over a time span implemented using time windows.
		 *
		 * Time windows are used internally to efficiently deal with missing time slot samples.
		 *
		 * Additionally a sample can be obtained for any non-negative time (ie, not restricted
		 * to the time range) by providing a function to create samples for times that are either
		 * outside the time range or that do not correspond to initialised time slots within the range.
		 */
		template <typename T>
		class TimeWindowSpan :
				public TimeSpan<T>
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a TimeWindowSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<TimeWindowSpan> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a TimeWindowSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<const TimeWindowSpan> non_null_ptr_to_const_type;


			/**
			 * Convenience typedef for a function that creates a sample from another sample.
			 *
			 * The function takes the following arguments:
			 * - The time of the sample being created,
			 * - The time of the source sample used to create the returned sample,
			 * - The source sample used to create the returned sample.
			 */
			typedef boost::function<
					T (
							const double &,
							const double &,
							const T &)>
									sample_creator_function_type;


			/**
			 * Create a @a TimeWindowSpan.
			 *
			 * The sample creator function @a sample_creator_function is only used by the
			 * @a get_or_create_sample method - it is used when @a get_or_create_sample is called
			 * for a time that does not correspond to an initialised time slot
			 * (ie, a time slot where @a get_sample_in_time_slot returns none).
			 * This is useful for samples that don't need to be stored in this look up table and
			 * for samples outside the time range.
			 * This saves memory usage which is the main purpose of this class - otherwise
			 * class @a TimeSampleSpan can be used instead.
			 *
			 * Providing a present-day sample enables the sample creator function to generate
			 * samples at times between the end of the time range and present day.
			 */
			static
			non_null_ptr_type
			create(
					const TimeRange &time_range,
					const sample_creator_function_type &sample_creator_function,
					const T &present_day_sample)
			{
				return non_null_ptr_type(
						new TimeWindowSpan(time_range, sample_creator_function, present_day_sample));
			}


			/**
			 * Returns the time range of the time span.
			 */
			virtual
			TimeRange
			get_time_range() const
			{
				return d_time_range;
			}


			/**
			 * Returns true if @a set_sample_in_time_slot has not been called for any time slots.
			 */
			virtual
			bool
			empty() const
			{
				return d_time_windows.empty();
			}


			/**
			 * Set the sample for the specified time slot.
			 *
			 * The number of time slots is available in the TimeRange returned by @a get_time_range.
			 *
			 * Throws exception if time_slot >= get_time_range().get_num_time_slots()
			 */
			virtual
			void
			set_sample_in_time_slot(
					const T &sample,
					unsigned int time_slot);


			/**
			 * Get the sample for the specified time slot.
			 *
			 * Returns none if @a set_sample_in_time_slot has not yet been called for @a time_slot.
			 *
			 * The number of time slots is available in the TimeRange returned by @a get_time_range.
			 *
			 * Throws exception if time_slot >= get_time_range().get_num_time_slots()
			 */
			virtual
			boost::optional<const T &>
			get_sample_in_time_slot(
					unsigned int time_slot) const;


			/**
			 * Non-const overload.
			 *
			 * Note: Returns none if @a set_sample_in_time_slot has not yet been called for @a time_slot.
			 */
			virtual
			boost::optional<T &>
			get_sample_in_time_slot(
					unsigned int time_slot);


			/**
			 * Returns the sample associated with the time slot of the specified time, or
			 * creates a sample if the specified time does not correspond to an initialised
			 * time slot (ie, a time slot where @a get_sample_in_time_slot returns none).
			 *
			 * The specified time can be any non-negative time (including present day 0Ma).
			 *
			 * This is the only method that uses the sample creator function @a sample_creator_function_type.
			 */
			T
			get_or_create_sample(
					const double &time) const;


			/**
			 * Returns the present-day sample.
			 */
			const T &
			get_present_day_sample() const
			{
				return d_present_day_sample;
			}

			//! Non-const overload.
			T &
			get_present_day_sample()
			{
				return d_present_day_sample;
			}

		private:

			/**
			 * A time window containing a contiguous time span of samples.
			 *
			 * Time windows can be separated by time periods containing no time samples.
			 * In other words time windows do not have to abut (or touch) each other.
			 */
			struct TimeWindow
			{
				/**
				 * Create a time window with one sample.
				 */
				TimeWindow(
						const T &sample,
						unsigned int time_slot) :
					begin_time_slot(time_slot),
					end_time_slot(time_slot)
				{
					sample_time_span.push_back(sample);
				}


				/**
				 * Typedef for a sequence of samples over a contiguous sequence of time slots.
				 *
				 * This is a deque so that we can efficiently add to the front and back since doing so
				 * does not move samples in memory. And we can still access using indices.
				 */
				typedef std::deque<T> sample_time_span_type;


				unsigned int begin_time_slot;
				unsigned int end_time_slot;

				/**
				 * The samples ordered least recent to most recent with the last sample associated with the end time.
				 */
				sample_time_span_type sample_time_span;
			};


			/**
			 * Typedef for a sequence of time windows.
			 *
			 * This is a list so that we can efficiently insert and erase because doing so
			 * doesn't move existing windows in memory.
			 */
			typedef std::list<TimeWindow> time_window_seq_type;


			TimeRange d_time_range;
			sample_creator_function_type d_sample_creator_function;
			T d_present_day_sample;
			time_window_seq_type d_time_windows;


			explicit
			TimeWindowSpan(
					const TimeRange &time_range,
					const sample_creator_function_type &sample_creator_function,
					const T &present_day_sample) :
				d_time_range(time_range),
				d_sample_creator_function(sample_creator_function),
				d_present_day_sample(present_day_sample)
			{  }
		};
	}
}

//
// Implementation
//

namespace GPlatesAppLogic
{
	namespace TimeSpanUtils
	{
		template <typename T>
		boost::optional<const T &>
		TimeSpan<T>::get_nearest_sample_at_time(
				const double &time) const
		{
			boost::optional<unsigned int> time_slot = get_time_range().get_nearest_time_slot(time);
			if (!time_slot)
			{
				return boost::none;
			}

			return get_sample_in_time_slot(time_slot.get());
		}


		template <typename T>
		boost::optional<T &>
		TimeSpan<T>::get_nearest_sample_at_time(
				const double &time)
		{
			boost::optional<unsigned int> time_slot = get_time_range().get_nearest_time_slot(time);
			if (!time_slot)
			{
				return boost::none;
			}

			return get_sample_in_time_slot(time_slot.get());
		}


		template <typename T>
		void
		TimeSampleSpan<T>::set_sample_in_time_slot(
				const T &sample,
				unsigned int time_slot)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					time_slot < d_sample_time_sequence.size(),
					GPLATES_ASSERTION_SOURCE);

			d_sample_time_sequence[time_slot] = sample;

			d_is_empty = false;
		}


		template <typename T>
		boost::optional<const T &>
		TimeSampleSpan<T>::get_sample_in_time_slot(
				unsigned int time_slot) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					time_slot < d_sample_time_sequence.size(),
					GPLATES_ASSERTION_SOURCE);

			const boost::optional<T> &sample = d_sample_time_sequence[time_slot];
			if (!sample)
			{
				return boost::none;
			}

			return sample.get();
		}


		template <typename T>
		boost::optional<T &>
		TimeSampleSpan<T>::get_sample_in_time_slot(
				unsigned int time_slot)
		{
			// Re-use the 'const' version of this method.
			boost::optional<const T &> sample =
					static_cast<const TimeSampleSpan *>(this)->get_sample_in_time_slot(time_slot);
			if (!sample)
			{
				return boost::none;
			}

			return const_cast<T &>(sample.get());
		}


		template <typename T>
		void
		TimeWindowSpan<T>::set_sample_in_time_slot(
				const T &sample,
				unsigned int time_slot)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					time_slot < d_time_range.get_num_time_slots(),
					GPLATES_ASSERTION_SOURCE);

			// Iterate through the time windows.
			// Note that the time windows are ordered moving forward in time.
			// In other words pretty much everything is going forward in time from earliest (or least recent)
			// to latest (or most recent).
			boost::optional<typename time_window_seq_type::iterator> prev_time_window_iter;
			typename time_window_seq_type::iterator time_windows_iter = d_time_windows.begin();
			typename time_window_seq_type::iterator time_windows_end = d_time_windows.end();
			for ( ; time_windows_iter != time_windows_end; ++time_windows_iter)
			{
				TimeWindow &time_window = *time_windows_iter;

				// Skip to the next time window if necessary.
				if (time_slot > time_window.end_time_slot)
				{
					prev_time_window_iter = time_windows_iter;
					continue;
				}

				if (time_slot >= time_window.begin_time_slot)
				{
					// We've found a time window containing the time slot.
					// So overwrite the existing sample.
					time_window.sample_time_span[time_slot - time_window.begin_time_slot] = sample;
				}
				else if (time_slot == time_window.begin_time_slot - 1)
				{
					// Expand the current window by one sample.
					time_window.sample_time_span.push_front(sample);
					--time_window.begin_time_slot;

					// See if need to merge with the previous time window.
					if (prev_time_window_iter &&
						time_slot == prev_time_window_iter.get()->end_time_slot + 1)
					{
						// Merge the previous time window.
						const TimeWindow &prev_time_window = *prev_time_window_iter.get();
						time_window.sample_time_span.insert(
								time_window.sample_time_span.begin(),
								prev_time_window.sample_time_span.begin(),
								prev_time_window.sample_time_span.end());
						time_window.begin_time_slot = prev_time_window.begin_time_slot;
						d_time_windows.erase(prev_time_window_iter.get());
					}
				}
				else if (prev_time_window_iter &&
						time_slot == prev_time_window_iter.get()->end_time_slot + 1)
				{
					// Expand the previous time window by one sample.
					TimeWindow &prev_time_window = *prev_time_window_iter.get();
					prev_time_window.sample_time_span.push_back(sample);
					++prev_time_window.end_time_slot;
				}
				else
				{
					// Insert a new time window before the current time window.
					d_time_windows.insert(
							time_windows_iter,
							TimeWindow(sample, time_slot));
				}

				return;
			}

			if (prev_time_window_iter &&
					time_slot == prev_time_window_iter.get()->end_time_slot + 1)
			{
				// Expand the previous time window by one sample.
				TimeWindow &prev_time_window = *prev_time_window_iter.get();
				prev_time_window.sample_time_span.push_back(sample);
				++prev_time_window.end_time_slot;
			}
			else
			{
				// Append a new time window.
				d_time_windows.push_back(TimeWindow(sample, time_slot));
			}
		}


		template <typename T>
		boost::optional<const T &>
		TimeWindowSpan<T>::get_sample_in_time_slot(
				unsigned int time_slot) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					time_slot < d_time_range.get_num_time_slots(),
					GPLATES_ASSERTION_SOURCE);

			// Iterate through the time windows.
			// Note that the time windows are ordered moving forward in time.
			// In other words pretty much everything is going forward in time from earliest (or least recent)
			// to latest (or most recent).
			typename time_window_seq_type::const_iterator time_windows_iter = d_time_windows.begin();
			typename time_window_seq_type::const_iterator time_windows_end = d_time_windows.end();
			for ( ; time_windows_iter != time_windows_end; ++time_windows_iter)
			{
				const TimeWindow &time_window = *time_windows_iter;

				// If we've found the right time window....
				if (time_slot <= time_window.end_time_slot)
				{
					if (time_slot >= time_window.begin_time_slot)
					{
						return time_window.sample_time_span[time_slot - time_window.begin_time_slot];
					}

					break;
				}
			}

			return boost::none;
		}


		template <typename T>
		boost::optional<T &>
		TimeWindowSpan<T>::get_sample_in_time_slot(
				unsigned int time_slot)
		{
			// Re-use the 'const' version of this method.
			boost::optional<const T &> sample =
					static_cast<const TimeWindowSpan *>(this)->get_sample_in_time_slot(time_slot);
			if (!sample)
			{
				return boost::none;
			}

			return const_cast<T &>(sample.get());
		}


		template <typename T>
		T
		TimeWindowSpan<T>::get_or_create_sample(
				const double &time) const
		{
			boost::optional<unsigned int> time_slot = d_time_range.get_nearest_time_slot(time);
			if (!time_slot)
			{
				// Since the time does not satisfy:
				//
				//   begin_time >= time >= end_time
				//
				// ...then it must satisfy either:
				//
				//   time > begin_time
				//
				// ...or...
				//
				//   time < end_time
				//
				if (time >= d_time_range.get_begin_time())
				{
					// Create a sample using the begin sample of the first time window (if any),
					// otherwise using the present-day sample.
					if (!d_time_windows.empty())
					{
						const TimeWindow &first_time_window = d_time_windows.front();
						return d_sample_creator_function(
								time,
								d_time_range.get_time(first_time_window.begin_time_slot),
								first_time_window.sample_time_span.front());
					}
					else // No time windows...
					{
						return d_sample_creator_function(time, 0, d_present_day_sample);
					}
				}
				else // time < d_time_range.get_end_time() ...
				{
					// Create a sample using the present-day sample.
					return d_sample_creator_function(time, 0, d_present_day_sample);
				}
			}

			// Iterate through the time windows.
			// Note that the time windows are ordered moving forward in time.
			// In other words pretty much everything is going forward in time from earliest (or least recent)
			// to latest (or most recent).
			typename time_window_seq_type::const_iterator time_windows_iter = d_time_windows.begin();
			typename time_window_seq_type::const_iterator time_windows_end = d_time_windows.end();
			for ( ; time_windows_iter != time_windows_end; ++time_windows_iter)
			{
				const TimeWindow &time_window = *time_windows_iter;

				// If we've found the right time window....
				if (time_slot.get() <= time_window.end_time_slot)
				{
					if (time_slot.get() >= time_window.begin_time_slot)
					{
						return time_window.sample_time_span[time_slot.get() - time_window.begin_time_slot];
					}

					// Create a sample using the begin sample of the current time window.
					return d_sample_creator_function(
							time,
							d_time_range.get_time(time_window.begin_time_slot),
							time_window.sample_time_span.front());
				}
			}

			// There are no initialised time slots after the requested time slot.
			// So create a sample using the present-day sample.
			return d_sample_creator_function(time, 0, d_present_day_sample);
		}
	}
}

#endif // GPLATES_APP_LOGIC_TIMESPANUTILS_H
