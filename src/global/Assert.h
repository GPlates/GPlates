/* $Id$ */

/**
 * \file 
 * File specific comments.
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
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#ifndef _GPLATES_GLOBAL_ASSERT_H_
#define _GPLATES_GLOBAL_ASSERT_H_

#include <iostream>

namespace GPlatesGlobal
{
	/**
	 * This is our new favourite Assert() statement.
	 * You use it thusly:
	 *
	 *   Assert(assertion, SomeException(args...));
	 *
	 * If `assertion' is not true, `SomeException' is thrown
	 * with arguments `args...'.
	 */
	template< typename E >
	inline void
	Assert(bool assertion, E ex_to_throw) {

		if (! assertion) throw ex_to_throw;
	}
}

#endif  // _GPLATES_GLOBAL_ASSERT_H_
