/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_NULLNONNULLINTRUSIVEPOINTEREXCEPTION_H
#define GPLATES_UTILS_NULLNONNULLINTRUSIVEPOINTEREXCEPTION_H

#include "global/PreconditionViolationError.h"


namespace GPlatesUtils
{
	/**
	 * This is the exception thrown by NullIntrusivePointerHandler when an attempt is made to
	 * instantiate a non-null intrusive-pointer with a NULL pointer.
	 */
	class NullNonNullIntrusivePointerException:
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		NullNonNullIntrusivePointerException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::PreconditionViolationError(exception_source)
		{  }

		~NullNonNullIntrusivePointerException() throw(){}

	protected:
		virtual
		const char *
		exception_name() const
		{
			// FIXME:  This function should really be defined in a .cc file.
			return "NullNonNullIntrusivePointerException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			// FIXME:  This function should really be defined in a .cc file.
		}
	};
}

#endif  // GPLATES_UTILS_NULLNONNULLINTRUSIVEPOINTEREXCEPTION_H
