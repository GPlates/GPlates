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

#include <boost/optional.hpp>
#include <QString>

namespace GPlatesGlobal
{
	namespace Version
	{
		/**
		 * The MAJOR.MINOR.PATCH[-PRERELEASE] human-readable version of GPlates.
		 *
		 * Where '-PRERELEASE' is optional and not used for official public releases (GPLATES_PUBLIC_RELEASE is true).
		 *
		 * For example "2.3.0-dev.1" for first development pre-release leading up to official "2.3.0".
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
		 * The optional PRERELEASE human-readable version suffix of GPlates.
		 *
		 * Returns none if GPLATES_PUBLIC_RELEASE is true (official public release).
		 */
		boost::optional<QString>
		get_GPlates_version_prerelease();
		
		
		/**
		* Returns the source code control revision number.
		*
		* Returns an empty string if either GPLATES_PUBLIC_RELEASE is true (official public release), or
		* there was an error retrieving the revision number.
		*/
		QString
		get_working_copy_version_number();

		/**
		* Returns the name of the branch from which this was compiled.
		*
		* Returns an empty string if either GPLATES_PUBLIC_RELEASE is true (official public release), or
		* there was an error retrieving the branch name.
		*/
		QString
		get_working_copy_branch_name();
	}
}

#endif // GPLATES_GLOBAL_VERSION_H
