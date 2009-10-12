/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_CLI_CLIREQUIREDOPTIONNOTPRESENT_H
#define GPLATES_CLI_CLIREQUIREDOPTIONNOTPRESENT_H

#include <string>

#include "global/GPlatesException.h"


namespace GPlatesCli
{
	/**
	 * This exception is thrown when an option is required but was not present (not found on
	 * command-line or in a config file).
	 */
	class RequiredOptionNotPresent :
			public GPlatesGlobal::Exception
	{
	public:
		/**
		 * Instantiate an exception for a file named @a filename.
		 */
		explicit
		RequiredOptionNotPresent(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const char *option_):
			Exception(exception_source),
			d_option(option_)
		{  }

		/**
		 * Return the option that was required but not present.
		 */
		const std::string &
		option() const
		{
			return d_option;
		}

	protected:
		virtual
		const char *
		exception_name() const
		{
			return "RequiredOptionNotPresent";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			write_string_message(
					os,
					std::string("Option '") + d_option + "' is required and was not found.");
		}

	private:
		/**
		 * The option that was required but not present.
		 */
		std::string d_option;
	};
}

#endif // GPLATES_CLI_CLIREQUIREDOPTIONNOTPRESENT_H
