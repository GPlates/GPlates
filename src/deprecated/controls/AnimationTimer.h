/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_CONTROLS_ANIMATIONTIMER_H_
#define _GPLATES_CONTROLS_ANIMATIONTIMER_H_

#include <wx/timer.h>
#include "global/types.h"  /* fpdata_t */
#include "maths/types.h"  /* real_t */


namespace GPlatesControls
{
	/**
	 * An animation-timer controls the rate of execution of an animation.
	 *
	 * This class is a singleton, since there can be at most <em>one</em>
	 * animation in progress at any point in time.  This single animation
	 * may be running or stopped.  An animation-timer needs to exist
	 * beyond the end of the function which creates it.
	 *
	 * NOTE that since this class contains static member data, there will
	 * be problems if threading is introduced within our code, or if
	 * multiple main-windows are allowed to exist.  In the latter case,
	 * an instance of this class should probably be a data member of the
	 * main-window class rather than a global singleton.
	 */
	class AnimationTimer : private wxTimer
	{
		public:
			/**
			 * This type represents a pointer to the function which
			 * the animation-timer will invoke to update the screen
			 * during the course of the animation.
			 *
			 * The function takes a single floating-point argument:
			 * the (geological) time to which to "warp".
			 *
			 * It is passed-in and stored as a data member to
			 * provide better separation of components.
			 */
			typedef void (*WarpFn)(const GPlatesGlobal::fpdata_t &);


			/**
			 * Create a new singleton animation-timer instance
			 * and start it running.
			 *
			 * The return-value describes whether the timer could
			 * be started: 'false' if it could not be started (in
			 * MS Windows, timers are a limited resource), 'true'
			 * otherwise.
			 *
			 * @param warp_to_time A pointer to the function which
			 *   will be invoked to update the screen during the
			 *   course of the animation.
			 * @param start_time The (geological) starting-time
			 *   of the animation.
			 * @param end_time The (geological) ending-time of the
			 *   animation.
			 * @param time_delta The (geological) time interval
			 *  between successive frames.  NOTE that this value
			 *  must be greater than zero.
			 * @param finish_on_end Whether or not the animation
			 *  should finish exactly on the ending-time.
			 * @param milli_secs The (real) time interval between
			 *   updates.
			 *
			 * NOTE that this function should not be called while
			 * an animation is running.  The animation-in-progress
			 * should be stopped (by 'stopping' the timer);
			 * alternately, wait for the animation to finish
			 * of its own accord; alternately, 'restart' the
			 * timer.
			 */
			static bool StartNew(WarpFn warp_to_time,
			                     GPlatesGlobal::fpdata_t start_time,
			                     GPlatesGlobal::fpdata_t end_time,
			                     GPlatesGlobal::fpdata_t time_delta,
			                     bool finish_on_end,
			                     int milli_secs);


			/**
			 * Return whether a singleton instance exists.
			 */
			static bool exists();


			/**
			 * Return whether an animation is currently in
			 * progress.  The timer of a running animation
			 * may be either stopped or restarted.
			 */
			static bool isRunning();


			/**
			 * Restart the animation-timer.  This operation
			 * may be performed regardless of whether the
			 * animation is currently running or not.
			 */
			static bool RestartTimer(int milli_secs);


			/**
			 * Stop the animation timer.
			 */
			static void StopTimer();


			/**
			 * The virtual function which is invoked by
			 * wxWindows to perform each update.
			 */
			virtual void Notify();

		private:
			/**
			 * The singleton instance.
			 */
			static AnimationTimer *_instance;


			/**
			 * A private constructor to ensure the singleton
			 * invariant.
			 */
			AnimationTimer(WarpFn warp_to_time,
			               GPlatesGlobal::fpdata_t start_time,
			               GPlatesGlobal::fpdata_t end_time,
			               GPlatesGlobal::fpdata_t time_delta,
			               bool finish_on_end);


			/**
			 * A pointer to the function which will be invoked
			 * to update the screen during the course of the
			 * animation.
			 */
			const WarpFn _warp_to_time;

			GPlatesMaths::real_t _curr_t;
			GPlatesMaths::real_t _end_t;
			GPlatesMaths::real_t _time_delta;

			bool _finish_on_end;

			GPlatesMaths::real_t _sense;
	};

}

#endif  // _GPLATES_CONTROLS_ANIMATIONTIMER_H_
