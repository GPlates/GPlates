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

#ifndef GPLATES_API_CONSOLEREADER_H
#define GPLATES_API_CONSOLEREADER_H

#include "global/python.h"

#include "AbstractConsole.h"

namespace GPlatesApi
{
	/**
	 * On construction, replaces sys.stdin with this, and on destruction,
	 * restores the original sys.stdin.
	 *
	 * This replacement is necessary to stop users from shooting themselves in
	 * the foot. If they attempt to use sys.stdin as provided, GPlates will just
	 * hang until the user types something into stdin, and this is a real
	 * problem if the user can't see the system console (e.g. on Windows)!
	 *
	 * Note that only the readline() method is supported. This method opens a
	 * modal dialog that prompts the user to enter in one line of text.
	 */
	class ConsoleReader
	{
	public:

		explicit
		ConsoleReader(
				AbstractConsole *console = NULL);

		~ConsoleReader();


		boost::python::object
		readline();

	private:
		AbstractConsole *d_console;
		boost::python::object d_old_object;
	};
}

#endif  // GPLATES_API_CONSOLEREADER_H
