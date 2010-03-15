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
 
#ifndef GPLATES_CLI_CLIINVALIDOPTIONVALUE_H
#define GPLATES_CLI_CLIINVALIDOPTIONVALUE_H

#include <string>

#include "global/GPlatesException.h"


namespace GPlatesCli
{
	/**
	 * This exception is thrown when the value of an option is invalid.
	 */
	class InvalidOptionValue :
			public GPlatesGlobal::Exception
	{
	public:
		InvalidOptionValue(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const char *option_,
				const std::string &message_):
			Exception(exception_source),
			d_option(option_),
			d_message(message_)
		{  }

		/**
		 * Return the option that was required but not present.
		 */
		const std::string &
		option() const
		{
			return d_option;
		}

		/**
		 * Return the error message.
		 */
		const std::string &
		message() const
		{
			return d_message;
		}

	protected:
		virtual
		const char *
		exception_name() const
		{
			return "InvalidOptionValue";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			write_string_message(
					os,
					std::string("Option '") + d_option + "' has an invalid value - " + d_message);
		}

	private:
		/**
		 * The option that was required but not present.
		 */
		std::string d_option;

		/**
		 * Error message describing reason option value is invalid.
		 */
		std::string d_message;
	};
}

#endif // GPLATES_CLI_CLIINVALIDOPTIONVALUE_H
