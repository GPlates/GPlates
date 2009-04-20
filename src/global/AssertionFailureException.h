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

#ifndef GPLATES_GLOBAL_ASSERTIONFAILUREEXCEPTION_H
#define GPLATES_GLOBAL_ASSERTIONFAILUREEXCEPTION_H

#include "GPlatesException.h"

namespace GPlatesGlobal
{
	/**
	 * Base GPlatesGlobal::Exception class which should be used for assertion
	 * failures; these exceptions indicate something is seriously wrong with
	 * the internal state of the program.
	 */
	class AssertionFailureException :
			public Exception
	{
	public:
		/**
		 * @param exception_source should be supplied using the @c GPLATES_EXCEPTION_SOURCE macro,
		 */
		explicit
		AssertionFailureException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			Exception(exception_source),
			d_filename(exception_source.get_filename()),
			d_line_num(exception_source.get_line_num())
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "AssertionFailureException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:
		const char *d_filename;
		int d_line_num;
	};
}

#endif  // GPLATES_GLOBAL_ASSERTIONFAILUREEXCEPTION_H
