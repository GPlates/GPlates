/* $Id$ */

/**
 * \file 
 * This file contains typedefs that are used in various places in
 * the code; they are not tied to any particular class.
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

#ifndef _GPLATES_GLOBAL_TYPES_H_
#define _GPLATES_GLOBAL_TYPES_H_

#include <cstdlib>
#include <string>
#include "InternalRID.h"


namespace GPlatesGlobal
{
	/**
	 * The type internally for rotation ids.
	 */
	typedef InternalRID rid_t;

	/** 
	 * The integral type.
	 */
	typedef int integer_t;

	/** 
	 * The index for the subscript operator.
	 * index_t has integral semantics, but is always non-negative.
	 */
	typedef unsigned int index_t;
}

#endif  // _GPLATES_GLOBAL_TYPES_H_
