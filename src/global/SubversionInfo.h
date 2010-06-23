/* $Id$ */

/**
 * \file 
 * Contains information about the Subversion working copy.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GLOBAL_SUBVERSIONINFO_H
#define GPLATES_GLOBAL_SUBVERSIONINFO_H


namespace GPlatesGlobal 
{
	// The functions in this namespace are declared in this header file, but their
	// definition exists in SubversionInfo.cc, which is generated at compile time.
	namespace SubversionInfo
	{
		/**
		 * Returns the compact version number returned by `svnversion`.
		 *
		 * This will be the empty string if an error occurred while running `svnversion`.
		 */
		const char *
		get_working_copy_version_number();

		/**
		 * Returns the name of the branch from which this was compiled.
		 *
		 * The branch name was obtained by inspecting the URL returned by `svn info`.
		 * If the working copy is trunk, this is the empty string.
		 *
		 * This will be the empty string if an error occurred while running `svnversion`.
		 */
		const char *
		get_working_copy_branch_name();
	}
}

#endif // GPLATES_GLOBAL_SUBVERSIONINFO_H
