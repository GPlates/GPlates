/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <iostream>
#include "AnimationTimer.h"
#include "GuiCalls.h"
#include "Lifetime.h"
#include "global/Exception.h"


bool
GPlatesControls::AnimationTimer::StartNew(WarpFn warp_to_time,
 GPlatesGlobal::fpdata_t start_time, GPlatesGlobal::fpdata_t end_time,
 GPlatesGlobal::fpdata_t time_delta, bool finish_on_end, int milli_secs) {

	if (AnimationTimer::exists()) {

		// an instance already exists -- better ensure it's not running
		if (AnimationTimer::isRunning()) {

			// this should not be allowed to occur
			// FIXME: complain, or something...

		} else {

			// current instance is not running, so we can kill it
			delete _instance;
		}
	}
	_instance = new AnimationTimer(warp_to_time, start_time, end_time,
	 time_delta, finish_on_end);

	bool success = _instance->Start(milli_secs, wxTIMER_CONTINUOUS);
	if (success) {

		// the animation was successfully started
		GuiCalls::SetOpModeToAnimation();
	}
	return success;
}


bool
GPlatesControls::AnimationTimer::exists() {

	return (_instance != NULL);
}


bool
GPlatesControls::AnimationTimer::isRunning() {

	if ( ! AnimationTimer::exists()) {

		// an instance can't be running if it doesn't exist...
		return false;

	} else {

		return (_instance->IsRunning());
	}
}


bool
GPlatesControls::AnimationTimer::RestartTimer(int milli_secs) {

	if ( ! AnimationTimer::exists()) {

		// an instance can't start if it doesn't exist...
		return false;

	} else {

		return (_instance->Start(milli_secs, wxTIMER_CONTINUOUS));
	}
}


void
GPlatesControls::AnimationTimer::StopTimer() {

	if (AnimationTimer::isRunning()) {

		_instance->Stop();
		GuiCalls::StopAnimation(true);
		GuiCalls::ReturnOpModeToNormal();
	}
}


void
GPlatesControls::AnimationTimer::Notify() {

	try {

		if ((_sense * _curr_t) <= (_sense * _end_t)) {

			// display the frame for time '_curr_t'
			(*_warp_to_time)(_curr_t.dval());

			_curr_t += (_sense * _time_delta);

		} else {

			// finish animation

			if ((_sense * _curr_t - _time_delta) < (_sense * _end_t)
			    && _finish_on_end) {

				/*
				 * Add one last frame to ensure the animation
				 * finishes *exactly* on the end time.
				 */
				(*_warp_to_time)(_end_t.dval());
			}
			_instance->Stop();
			GuiCalls::StopAnimation(false);
			GuiCalls::ReturnOpModeToNormal();
		}

	} catch (const GPlatesGlobal::Exception &e) {

		/*
		 * Need to deal with GPlates Exceptions here in a "tough" way
		 * (ie. terminating the program) since this member function is
		 * called directly by wxWindows.
		 */
		std::cerr << "Caught Exception: " << e << std::endl;
		GPlatesControls::Lifetime::instance()->terminate(
		 "Unable to recover from exception caught in GPlatesControls::AnimationTimer::Notify.");
	}
}


GPlatesControls::AnimationTimer *
GPlatesControls::AnimationTimer::_instance = NULL;


GPlatesControls::AnimationTimer::AnimationTimer(WarpFn warp_to_time,
 GPlatesGlobal::fpdata_t start_time, GPlatesGlobal::fpdata_t end_time,
 GPlatesGlobal::fpdata_t time_delta, bool finish_on_end) :
 wxTimer(), _warp_to_time(warp_to_time), _curr_t(start_time), _end_t(end_time),
 _time_delta(time_delta), _finish_on_end(finish_on_end) {

	/*
	 * Calculate the "sense" (ie, direction through time)
	 * from 'start_time' to 'end_time'.
	 */
	if (end_time >= start_time) {

		_sense = 1.0;

	} else {

		_sense = -1.0;
	}
}
