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
 *
 */

#ifndef _GPLATES_GLOBAL_TYPES_H_
#define _GPLATES_GLOBAL_TYPES_H_

#include <cstdlib>
#include <string>
#include "FPData.h"
#include "InternalRID.h"


namespace GPlatesGlobal
{
	/**
	 * A floating-point type used to store static (ie. unchanging) data.
	 */
	typedef FPData fpdata_t;

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
