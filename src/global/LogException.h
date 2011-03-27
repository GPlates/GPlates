/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_LOGEXCEPTION_H
#define GPLATES_GLOBAL_LOGEXCEPTION_H

#include <string>
#include <QString>

#include "GPlatesException.h"

namespace GPlatesGlobal
{
	/**
	 * Base GPlatesGlobal::Exception class which should be used for assertion
	 * failures; these exceptions indicate something is seriously wrong with
	 * the internal state of the program.
	 */
	class LogException :
			public Exception
	{
	public:
		/**
		 * @param exception_source should be supplied using the @c GPLATES_EXCEPTION_SOURCE macro,
		 * @param message the error message, as a 'const char *', to get logged.
		 */
		explicit
		LogException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const char *message) :
			Exception(exception_source),
			d_message(message)
		{  }

		/**
		 * @param exception_source should be supplied using the @c GPLATES_EXCEPTION_SOURCE macro,
		 * @param message the error message, as a std::string, to get logged.
		 */
		explicit
		LogException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const std::string &message) :
			Exception(exception_source),
			d_message(message)
		{  }

		/**
		 * @param exception_source should be supplied using the @c GPLATES_EXCEPTION_SOURCE macro,
		 * @param message the error message, as a QString, to get logged.
		 *
		 * Note that the QString input message gets converted to a std::string so only
		 * standard ascii should be used.
		 */
		explicit
		LogException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const QString &message) :
			Exception(exception_source),
			d_message(message.toStdString())
		{  }

		~LogException() throw() { }

	protected:
		virtual
		const char *
		exception_name() const
		{
			return "LogException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			write_string_message(os, d_message);
		}

	private:
		std::string d_message;
	};
}

#endif // GPLATES_GLOBAL_LOGEXCEPTION_H
