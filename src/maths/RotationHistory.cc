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

#include <sstream>
#include "RotationHistory.h"
#include "InvalidOperationException.h"


using namespace GPlatesMaths;


bool
RotationHistory::isDefinedAtTime(real_t t) const {

	// check outer bounds
	if (t < _most_recent_time) return false;
	if (t > _most_distant_time) return false;

	ensureSeqSorted();
	for (seq_type::const_iterator it = _seq.begin();
	     it != _seq.end();
	     it++) {

		if ((*it).isDefinedAtTime(t)) return true;
	}
	return false;
}


void
RotationHistory::insert(const RotationSequence &rseq) {

	_seq.push_back(rseq);
	_is_modified = true;
	if (rseq.mostRecentTime() < _most_recent_time) {

		_most_recent_time = rseq.mostRecentTime();
	}
	if (rseq.mostDistantTime() < _most_distant_time) {

		_most_distant_time = rseq.mostDistantTime();
	}
}


RotationHistory::const_iterator
RotationHistory::atTime(real_t t) const {

	// it is assumed that the list can never be empty

	ensureSeqSorted();
	for (seq_type::const_iterator it = _seq.begin();
	     it != _seq.end();
	     it++) {

		if ((*it).isDefinedAtTime(t)) return it;
	}

	std::ostringstream oss("Attempted to access a rotation sequence "
	 "for the time ");
	oss << t << ", at which time this rotation history is not defined.";

	throw InvalidOperationException(oss.str().c_str());
}
