/* $Id$ */

/**
 * \file 
 * This file contains typedefs that are used in various places in
 * the code; they are not tied to any particular class.
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
 */

#ifndef GPLATES_MATHS_TYPES_H
#define GPLATES_MATHS_TYPES_H

#include "Real.h"

namespace GPlatesMaths
{
	/**
	 * A floating-point approximation to the field of reals.
	 */
	typedef Real real_t;

	/**
	 * The type used to identify plate rotations.
	 * FIXME: This type should replace GPlatesGlobal::rid_t.
	 * Also, instead of unsigned long, use std::size_t.
	 */
	typedef unsigned long rot_id_t;
}

#endif  // GPLATES_MATHS_TYPES_H
