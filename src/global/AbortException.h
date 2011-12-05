/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_GLOBAL_ABORTEXCEPTION_H
#define GPLATES_GLOBAL_ABORTEXCEPTION_H

#include "GPlatesException.h"

namespace GPlatesGlobal
{
	/**
	 * Base GPlatesGlobal::Exception class which should be used for aborts;
	 * these exceptions indicate something is seriously wrong with
	 * the internal state of the program.
	 */
	class AbortException :
			public Exception
	{
	public:
		/**
		 * @param exception_source should be supplied using the @c GPLATES_EXCEPTION_SOURCE macro,
		 */
		explicit
		AbortException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			Exception(exception_source)
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "AbortException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;
	};
}

#endif // GPLATES_GLOBAL_ABORTEXCEPTION_H
