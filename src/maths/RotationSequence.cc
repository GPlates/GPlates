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

	if (&other != this) {

		_fixed_plate = other._fixed_plate;
		_most_recent_time = other._most_recent_time;
		_most_distant_time = other._most_distant_time;

		SharedSequence *ss = _shared_seq;
		shareOthersSharedSeq(other);
		relinquish(ss);
	}
	return *this;
}


bool
GPlatesMaths::RotationSequence::edgeProperties(real_t t, EdgeType mode) const {

	// First, deal with times in the future.
	if (t < 0.0) {

		/*
		 * Either:
		 *  (i) this sequence IS defined in the future, which means
		 *   it's defined at ALL times in the future, which means there
		 *   cannot be an edge at the current point in time;
		 * or:
		 *  (ii) this sequence IS NOT defined in the future, which
		 *   means there cannot be an edge at the current point in
		 *   time.
		 *
		 * Either way, the answer is, uh, 'false'.
		 */
		return false;
	}

	if ((mode & EARLIER_EDGE) && (t == _most_recent_time)) return true;
	if ((mode & LATER_EDGE) && (t == _most_distant_time)) return true;

	return false;
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

		// Interpolate between most recent (which is assumed to be
		// at time 0) and the next most recent.
		seq_type::const_iterator most_recent = _shared_seq->begin();

		seq_type::const_iterator next_most_recent = most_recent;
		next_most_recent++;

		return interpolate(*most_recent, *next_most_recent, t);
	}

	/*
	 * Otherwise, t >= 0.  See if 't' matches the most recent finite
	 * rotation (there will always be at least one finite rotation in
	 * a rotation sequence).
	 */
	seq_type::const_iterator curr_rot = _shared_seq->begin();
	if (t < (*curr_rot).time()) {

		std::ostringstream oss;
		oss << "Attempted to obtain a finite rotation for the time "
		 << t << ",\n"
		 << "which is outside the time-span of this rotation sequence: "
		 << "[" << _most_recent_time << "Ma, "
		 << _most_distant_time << "Ma].";

		throw InvalidOperationException(oss.str().c_str());
	}
	if (t == (*curr_rot).time()) {

		// An exact match.
		return (*curr_rot);
	}

	/*
	 * Imagine this whole finite rotation thing like a series of
	 * fence-posts with horizontal rails between them: |--|--|--|
	 * (great asky-art, huh?)
	 *
	 * Each fence-post is a finite rotation, and each rail is the
	 * interpolation between adjacent posts.  We want to check whether
	 * the point representing 't' lies on this fence, or outside it.
	 * We've already checked the first fence-post, and we know that 't'
	 * lies after this first fence-post.  Now we will compare 't' with
	 * all the remaining rails and posts.
	 */

	// This is the most recent post which has already been considered.
	seq_type::const_iterator prev_rot = curr_rot;

	// This is the very last post.
	seq_type::const_iterator seq_end = _shared_seq->end();
	for (curr_rot++; curr_rot != seq_end; prev_rot = curr_rot++) {

		if (t < (*curr_rot).time()) {

			/*
			 * The time for which we're searching ('t') lies
			 * in-between the time of the previous finite
			 * rotation (the most recent post) and the time of
			 * the current finite rotation (the next post) --
			 * on a rail, if you like.  Hence we need to
			 * interpolate with a stage rotation.
			 *
			 * As we iterate through the sequence, we're moving
			 * back in time, away from the present.  So 'curr_rot'
			 * will be more distant in time than 'prev_rot'.
			 */
			return interpolate(*prev_rot, *curr_rot, t);
		}
		if (t == (*curr_rot).time()) {

			// An exact match.
			return (*curr_rot);
		}
	}
	/*
	 * Else, we've passed the last fence-post, and none of them were
	 * greater-than or equal-to 't', which means that 't' was greater
	 * than them all.  [Tweedledee: "That's logic."]
	 */
	std::ostringstream oss;
	oss << "Attempted to obtain a finite rotation for the time "
	 << t << ",\n"
	 << "which is outside the time-span of this rotation sequence: "
	 << "[" << _most_recent_time << "Ma, "
	 << _most_distant_time << "Ma].";

	throw InvalidOperationException(oss.str().c_str());
}


void
GPlatesMaths::RotationSequence::insert(const FiniteRotation &frot) {

	_shared_seq->insert(frot);

	// Update '_most_recent_time' and '_most_distant_time' as appropriate.
	if (frot.time() < _most_recent_time) _most_recent_time = frot.time();
	if (frot.time() > _most_distant_time) _most_distant_time = frot.time();
}
