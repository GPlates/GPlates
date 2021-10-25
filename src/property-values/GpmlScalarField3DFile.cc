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


const GPlatesPropertyValues::GpmlScalarField3DFile::non_null_ptr_type
GPlatesPropertyValues::GpmlScalarField3DFile::create(
		const file_name_type &filename_)
{
	return non_null_ptr_type(new GpmlScalarField3DFile(filename_));
}


std::ostream &
GPlatesPropertyValues::GpmlScalarField3DFile::print_to(
		std::ostream &os) const
{
	return os << "GpmlScalarField3DFile";
}
