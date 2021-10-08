/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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
GPlatesMaths::RotationHistory::findAtTime(real_t t) const {

	ensureSeqSorted();
	for (seq_type::const_iterator it = _seq.begin();
	     it != _seq.end();
	     ++it) {

		if ((*it).isDefinedAtTime(t)) return const_iterator(it);
	}
	return const_iterator(_seq.end());
}
