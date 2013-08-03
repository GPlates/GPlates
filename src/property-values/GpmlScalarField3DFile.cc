/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "GpmlScalarField3DFile.h"


void
GPlatesPropertyValues::GpmlScalarField3DFile::set_file_name(
		const file_name_type &filename_)
{
	MutableRevisionHandler revision_handler(this);
	// To keep our revision state immutable we clone the filename property value so that the client
	// can no longer modify it indirectly...
	revision_handler.get_mutable_revision<Revision>().filename = filename_->clone();
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GpmlScalarField3DFile::print_to(
		std::ostream &os) const
{
	return os << "GpmlScalarField3DFile";
}
