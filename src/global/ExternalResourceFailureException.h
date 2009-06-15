/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_EXTERNALRESOURCEFAILUREEXCEPTION_H
#define GPLATES_GLOBAL_EXTERNALRESOURCEFAILUREEXCEPTION_H

#include "GPlatesException.h"


namespace GPlatesGlobal
{
	/**
	 * This is the base class of all exceptions in GPlates which are due to the failure of some
	 * external resource.
	 */
	class ExternalResourceFailureException:
			public Exception
	{
		public:
			/**
			 * An alternative constructor that adds the location at which exception is thrown
			 * to the call stack trace.
			 */
			ExternalResourceFailureException(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				Exception(exception_source)
			{  }
	};
}

#endif  // GPLATES_GLOBAL_EXTERNALRESOURCEFAILUREEXCEPTION_H
