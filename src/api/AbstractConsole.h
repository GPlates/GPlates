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

#ifndef GPLATES_API_ABSTRACTCONSOLE_H
#define GPLATES_API_ABSTRACTCONSOLE_H
#include <QString>

#if !defined(GPLATES_NO_PYTHON)

#include "global/python.h"

namespace GPlatesApi
{
	/**
	 * The abstract base class for consoles that can display output from Python,
	 * and read lines as input.
	 */
	class AbstractConsole
	{
	public:

		virtual
		~AbstractConsole()
		{  }

		/**
		 * Appends the given @a text to the console. The @a error flag indicates
		 * whether it should be decorated as an error message or not.
		 *
		 * Any implementation of this function must be thread-safe.
		 */
		virtual
		void
		append_text(
				const QString &text,
				bool error = false) = 0;


		/**
		 * Appends the stringified version of @a obj to the console. The @a error
		 * flag indicates whether it should be decorated as an error message or not.
		 *
		 * Any implementation of this function must be thread-safe.
		 */
		virtual
		void
		append_text(
				const boost::python::object &obj,
				bool error = false) = 0;


		/**
		 * Prompts the user for a line of input.
		 *
		 * Any implementation of this function must be thread-safe.
		 */
		virtual
		QString
		read_line() = 0;
	};
}
#endif   //GPLATES_NO_PYTHON
#endif  // GPLATES_API_ABSTRACTCONSOLE_H
