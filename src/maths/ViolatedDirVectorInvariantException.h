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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_VIOLATEDDIRVECTORINVARIANTEXCEPTION_H_
#define _GPLATES_MATHS_VIOLATEDDIRVECTORINVARIANTEXCEPTION_H_

#include "MathematicalException.h"

namespace GPlatesMaths
{
	/**
	 * The Exception thrown when direction vector invariants are violated.
	 */
	class ViolatedDirVectorInvariantException
		: public MathematicalException
	{
		public:
			/**
			 * @param msg is a description of the conditions
			 * which cause the invariant to be violated.
			 */
			ViolatedDirVectorInvariantException(const char *msg)
				: _msg(msg) {  }

			virtual
			~ViolatedDirVectorInvariantException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "ViolatedDirVectorInvariantException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_MATHS_VIOLATEDDIRVECTORINVARIANTEXCEPTION_H_
