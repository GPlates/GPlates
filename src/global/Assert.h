/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#ifndef _GPLATES_GLOBAL_ASSERT_H_
#define _GPLATES_GLOBAL_ASSERT_H_

namespace GPlatesGlobal
{
	/**
	 * This is our new favourite Assert() statement.
	 * You use it thusly:
	 *
	 *   Assert(assertion, SomeException(args...));
	 *
	 * If @a assertion is not true, @a SomeException is thrown
	 * with arguments @a args... .
	 *
	 * @param assertion is the expression to test as the assertion
	 *  condition.
	 * @param ex_to_throw is the constructor-call of the exception to throw
	 *  if the assertion condition is not true.
	 */
	template< typename E >
	inline void
	Assert(bool assertion, E ex_to_throw) {

		if (! assertion) throw ex_to_throw;
	}
}

#endif  // _GPLATES_GLOBAL_ASSERT_H_
