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
#include "RotationHistory.h"
#include "InvalidOperationException.h"


bool
GPlatesMaths::RotationHistory::isDefinedAtTime(real_t t) const {

	ensureSeqSorted();
	for (seq_type::const_iterator it = _seq.begin();
	     it != _seq.end();
	     ++it) {

		if ((*it).isDefinedAtTime(t)) return true;
	}
	return false;
}


GPlatesMaths::RotationHistory::const_iterator
GPlatesMaths::RotationHistory::atTime(real_t t) const {

	ensureSeqSorted();
	for (seq_type::const_iterator it = _seq.begin();
	     it != _seq.end();
	     ++it) {

		if ((*it).isDefinedAtTime(t)) return it;
	}

	std::ostringstream oss;
	oss << "Attempted to access a rotation sequence for the time "
	 << t << ",\nat which time this rotation history is not defined.";

	throw InvalidOperationException(oss.str().c_str());
}
