/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#include <iostream>
#include "GPlatesException.h"


void
GPlatesGlobal::Exception::write(
		std::ostream &os) const
{
	os << ExceptionName();

	// output a message (if it exists)
	// FIXME:  Surely a message should always exist?  (I can't think of any useful exceptions
	// which shouldn't contain some specific information.)
	// FIXME:  Rather than creating a string for the message, there should be a 'write_message'
	// function which accepts this same ostream.
	std::string msg = Message();
	if ( ! msg.empty()) {

		os << "(\"" << msg << "\")";
	} else {
		os << ": ";
		write_message(os);
	}
}

