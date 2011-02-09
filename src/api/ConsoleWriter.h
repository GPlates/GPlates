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

#ifndef GPLATES_API_CONSOLEWRITER_H
#define GPLATES_API_CONSOLEWRITER_H

#include "global/python.h"

#include "AbstractConsole.h"


namespace GPlatesApi
{
	/**
	 * On construction, redirects either of sys.stdout or sys.stderr (depending
	 * on the @a stream argument) to the specified console by replacing
	 * it with 'this'. On destruction, the original sys.stdout or sys.stderr
	 * is restored.
	 *
	 * Note that only the write() method is supported, and the other methods
	 * that Python's native sys.stdout or sys.stderr support are not present.
	 * However, this enough to capture output using "print" and to capture errors
	 * printed via @a PyErr_Print().
	 */
	class ConsoleWriter
	{
	public:

		explicit
		ConsoleWriter(
				bool error = false,
				AbstractConsole *console = NULL);

		~ConsoleWriter();

#if !defined(GPLATES_NO_PYTHON)
		void
		write(
				const boost::python::object &text);
#endif

	private:

		bool d_error;
		AbstractConsole *d_console;
#if !defined(GPLATES_NO_PYTHON)
		boost::python::object d_old_object;
#endif
	};
}

#endif  // GPLATES_API_CONSOLEWRITER_H
