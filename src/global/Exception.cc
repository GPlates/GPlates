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

#include "Exception.h"

using namespace GPlatesGlobal;

void
Exception::Write(std::ostream &os) const {

	os << ExceptionName();

	// output a message (if it exists)
	std::string msg = Message();
	if ( ! msg.empty()) {

		os << "(\"" << msg << "\")";
	}
}

