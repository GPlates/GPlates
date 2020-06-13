/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_VERSION_H
#define GPLATES_GLOBAL_VERSION_H

#include <QString>

namespace GPlatesGlobal
{
	namespace Version
	{
		/**
		 * The MAJOR.MINOR.PATCH version number of GPlates.
		 *
		 * For example "2.2.0".
		 */
		QString
		get_GPlates_version();

		/**
		 * The MAJOR version number of GPlates.
		 */
		unsigned int
		get_GPlates_version_major();

		/**
		 * The MINOR version number of GPlates.
		 */
		unsigned int
		get_GPlates_version_minor();

		/**
		 * The PATCH version number of GPlates.
		 */
		unsigned int
		get_GPlates_version_patch();

		/**
		 * The TWEAK version number of GPlates.
		 */
		unsigned int
		get_GPlates_version_tweak();


		/**
		 * The revision number of pyGPlates.
		 */
		unsigned int
		GPlatesGlobal::Version::get_pyGPlates_revision()


		/**
		* Returns the source code control revision number.
		*
		* Returns an empty string if either GPLATES_PUBLIC_RELEASE is true, or
		* there was an error retrieving the revision number.
		*/
		QString
		get_working_copy_version_number();

		/**
		* Returns the name of the branch from which this was compiled.
		*
		* Returns an empty string if either GPLATES_PUBLIC_RELEASE is true, or
		* there was an error retrieving the branch name.
		*/
		QString
		get_working_copy_branch_name();
	}
}

#endif // GPLATES_GLOBAL_VERSION_H
