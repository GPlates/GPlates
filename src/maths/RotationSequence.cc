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

#include <sstream>
#include <memory>  /* std::auto_ptr */
#include "RotationSequence.h"
#include "StageRotation.h"  /* interpolate */
#include "InvalidOperationException.h"
#include "global/ControlFlowException.h"


GPlatesMaths::RotationSequence::RotationSequence(const rid_t &fixed_plate,
	const rid_t &moving_plate, const FiniteRotation &frot)

	: _fixed_plate(fixed_plate), _moving_plate(moving_plate) {

	/*
	 * Avoid memory leaks which would occur if an exception were thrown
	 * in this ctor.
	 */
	std::auto_ptr< SharedSequence > ss_ptr(new SharedSequence());
	ss_ptr->insert(frot);

	// ok, it should be safe now
	_shared_seq = ss_ptr.release();
	_most_recent_time = frot.time();
	_most_distant_time = frot.time();
}


GPlatesMaths::RotationSequence &
GPlatesMaths::RotationSequence::operator=(const RotationSequence &other) { 

	_fixed_plate = other._fixed_plate;
	_most_recent_time = other._most_recent_time;
	_most_distant_time = other._most_distant_time;

	SharedSequence *ss = _shared_seq;
	shareOthersSharedSeq(other);
	decrementSharedSeqRefCount(ss);

	return *this;
}


void
GPlatesMaths::RotationSequence::insert(const FiniteRotation &frot) {

	_shared_seq->insert(frot);
	if (frot.time() < _most_recent_time) {

		_most_recent_time = frot.time();
	}
	if (frot.time() > _most_distant_time) {

		_most_distant_time = frot.time();
	}
}


GPlatesMaths::FiniteRotation
GPlatesMaths::RotationSequence::finiteRotationAtTime(real_t t) const {

	// It is assumed that a rotation sequence can never be empty.

	// First, deal with times in the future.
	if (t < 0.0) {

		if ( ! isDefinedInFuture()) {

			std::ostringstream oss;
			oss << "Attempted to obtain a finite rotation "
			 << "for the time " << t << ",\n"
			 << "but this rotation sequence ["
			 << _most_recent_time << "Ma, "
			 << _most_distant_time<< "Ma] cannot be extrapolated\n"
			 << "into the future.";

			throw InvalidOperationException(oss.str().c_str());
		}

		seq_type::const_iterator most_recent = _shared_seq->begin();
		seq_type::const_iterator next = most_recent;
		next++;

		return interpolate(*most_recent, *next, t);
	}

	seq_type::const_iterator curr_rot = _shared_seq->begin();
	if (t == (*curr_rot).time()) {

		// direct hit! first time! You got the touch!
		return (*curr_rot);
	}
	if (t < (*curr_rot).time()) {

		std::ostringstream oss;
		oss << "Attempted to obtain a finite rotation for the time "
		 << t << ",\n"
		 << "which is outside the time-span of this rotation sequence: "
		 << "[" << _most_recent_time << "Ma, "
		 << _most_distant_time << "Ma].";

		throw InvalidOperationException(oss.str().c_str());
	}

	seq_type::const_iterator prev_rot = curr_rot;
	seq_type::const_iterator seq_end = _shared_seq->end();
	for (curr_rot++; curr_rot != seq_end; prev_rot = curr_rot++) {

		if (t == (*curr_rot).time()) {

			// direct hit!
			return (*curr_rot);
		}
		if (t < (*curr_rot).time()) {

			/*
			 * The time for which we're searching ('t')
			 * lies in-between the time of the previous finite
			 * rotation and the time of the current finite
			 * rotation.  Hence we need to interpolate with
			 * a stage rotation.
			 *
			 * As we iterate through the sequence, we're moving
			 * back in time, away from the present.  So 'curr_rot'
			 * will be more distant in time than 'prev_rot'.
			 */
			return interpolate(*prev_rot, *curr_rot, t);
		}
	}

	if (t > (*prev_rot).time()) {

		std::ostringstream oss;
		oss << "Attempted to obtain a finite rotation for the time "
		 << t << ",\n"
		 << "which is outside the time-span of this rotation sequence: "
		 << "[" << _most_recent_time << "Ma, "
		 << _most_distant_time << "Ma].";

		throw InvalidOperationException(oss.str().c_str());
	}

	/*
	 * It should not be possible to reach the end of this function:
	 * Logically, 't' must be one of:
	 *  - more recent than this rotation sequence -> exception
	 *  - more distant than this rotation sequence -> exception
	 *  - within the time-span of this rotation sequence -> return
	 *     a finite rotation
	 *
	 * So, if control flow gets to the end of this function, the
	 * programmer has made a mistake.
	 */
	throw ControlFlowException("Reached the end of the function "
	 "RotationSequence::finiteRotationAtTime");
}
