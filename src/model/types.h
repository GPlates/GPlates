/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_TYPES_H
#define GPLATES_MODEL_TYPES_H

namespace GPlatesModel
{
	/**
	 * This is the type which is used to represent integer plate IDs.
	 */
	typedef unsigned long integer_plate_id_type;

	/**
	 * This is the type which is used to describe the sizes of containers of properties,
	 * features, and feature collections, and also for the indices into these containers.  We
	 * define this (rather than using the appropriate container<T>::size_type in each different
	 * context) to avoid circular header includes and to simplify code in general.
	 */
	typedef size_t container_size_type;

	/**
	 * This is the value used to indicate an invalid index.  We define this as a constant here
	 * to avoid circular header includes and to simplify code in general.
	 */
	static const container_size_type INVALID_INDEX = -1;
}

#endif  // GPLATES_MODEL_TYPES_H
