/* $Id$ */

/**
 * \file 
 * This file contains typedefs that are used in various places in
 * the code; they are not tied to any particular class.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#ifndef _GPLATES_GLOBAL_TYPES_H_
#define _GPLATES_GLOBAL_TYPES_H_

#include <cstdlib>

namespace GPlatesGlobal
{
	/** 
	 * From the field of reals.
	 */
	typedef double real_t;

	/** 
	 * The integral type.
	 */
	typedef int integer_t;

	/** 
	 * The index for the subscript operator. 
	 * index_t has integral semantics.  
	 */
	typedef int index_t;
}

#endif  // _GPLATES_GLOBAL_TYPES_H_
