/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include <iostream>
#include "AnimationTimer.h"
#include "global/Exception.h"


bool
GPlatesControls::AnimationTimer::StartNew(WarpFn warp_to_time, int num_steps,
 GPlatesGlobal::fpdata_t start_time, GPlatesGlobal::fpdata_t end_time,
 int milli_secs) {

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
	_instance =
	 new AnimationTimer(warp_to_time, num_steps, start_time, end_time);
	return (_instance->Start(milli_secs, wxTIMER_CONTINUOUS));
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

	if (AnimationTimer::isRunning()) _instance->Stop();
}


void
GPlatesControls::AnimationTimer::Notify() {

	try {

		if (_curr_frame < _num_frames) {

			// display the frame for time '_curr_t'
			(*_warp_to_time)(_curr_t.dval());

			_curr_frame++;
			_curr_t += _time_incr;

		} else {

			// final frame
			(*_warp_to_time)(_end_t.dval());
			_instance->Stop();
		}

	} catch (const GPlatesGlobal::Exception &e) {

		std::cerr
		 << "Internal exception inside 'AnimationTimer::Notify':"
		 << e << std::endl;
		exit(1);
	}
}


GPlatesControls::AnimationTimer *
GPlatesControls::AnimationTimer::_instance = NULL;


GPlatesControls::AnimationTimer::AnimationTimer(WarpFn warp_to_time,
 int num_steps, GPlatesGlobal::fpdata_t start_time,
 GPlatesGlobal::fpdata_t end_time) :
 wxTimer(), _warp_to_time(warp_to_time), _curr_frame(1), _curr_t(start_time),
 _end_t(end_time) {

	_num_frames = static_cast< unsigned >(num_steps);
	_time_incr = (_end_t - _curr_t) / (num_steps - 1);
}
