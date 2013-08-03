/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "GpmlTopologicalPoint.h"


void
GPlatesPropertyValues::GpmlTopologicalPoint::set_source_geometry(
		GpmlPropertyDelegate::non_null_ptr_to_const_type source_geometry)
{
	MutableRevisionHandler revision_handler(this);
	// To keep our revision state immutable we clone the property value so that the client
	// can no longer modify it indirectly...
	revision_handler.get_mutable_revision<Revision>().source_geometry = source_geometry->clone();
	revision_handler.handle_revision_modification();
}
