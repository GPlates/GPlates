/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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

#ifndef GPLATES_FILEIO_EXTERNALPROGRAM_H
#define GPLATES_FILEIO_EXTERNALPROGRAM_H

#include <QString>

namespace GPlatesFileIO
{
	/**
	 * Encapsulates an external program that GPlates is interested in
	 * invoking; as this program is not necessarily installed on all
	 * systems, it also includes a command to test for the existence
	 * of the program (A non-destructive command such as calling the
	 * program with a --help or --version argument)
	 */
	class ExternalProgram
	{
	public:
		/**
		 * Creates an ExternalProgram record, given the program name and
		 * arguments intended to run it with, and a command-line used to
		 * test the existence of the program.
		 */
		explicit
		ExternalProgram(
				const QString &command_,
				const QString &command_test_):
			d_command(command_),
			d_command_test(command_test_)
		{  }

		/**
		 * Return the command line to invoke for this program.
		 */
		const QString &
		command() const
		{
			return d_command;
		}

		/**
		 * Return the command line used to test for this program's existence.
		 */
		const QString &
		command_test() const
		{
			return d_command_test;
		}
		
		/**
		 * Verify the program is available, by executing the test command.
		 */
		bool
		test() const;

	private:
		/**
		 * The command line to invoke.
		 */
		QString d_command;
	
		/**
		 * The command line to test with.
		 */
		QString d_command_test;
	};
}
#endif  // GPLATES_FILEIO_EXTERNALPROGRAM_H
