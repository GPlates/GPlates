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

#ifndef _GPLATES_MATHS_INDETERMINATERESULTEXCEPTION_H_
#define _GPLATES_MATHS_INDETERMINATERESULTEXCEPTION_H_

#include "MathematicalException.h"

namespace GPlatesMaths
{
	/**
	 * The Exception thrown when a mathematical calculation is attempted
	 * which would return an indeterminate result.
	 */
	class IndeterminateResultException
		: public MathematicalException
	{
		public:
			/**
			 * @param msg is a description of the conditions
			 * which would result in an indeterminate result.
			 */
			IndeterminateResultException(const char *msg)
				: _msg(msg) {  }

			virtual
			~IndeterminateResultException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "IndeterminateResultException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_MATHS_INDETERMINATERESULTEXCEPTION_H_
