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
		/////////////
		// GPlates //
		/////////////

		/**
		 * The MAJOR.MINOR.PATCH[-PRERELEASE] "human-readable" version of GPlates
		 * (very similar to Semantic Versioning https://semver.org/spec/v2.0.0.html).
		 *
		 * Where '-PRERELEASE' is optional and only used for pre-releases.
		 *
		 * For example "2.3.0-dev1" for first development pre-release leading up to official "2.3.0", or
		 * "2.3.0-rc.1" for first release candidate.
		 *
		 * Note: Currently the only difference compared to Semantic Versioning is the "dev" in development releases
		 *       (which is not part of Semantic Versioning).
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
		 * The optional PRERELEASE "human-readable" version suffix of GPlates
		 * (very similar to Semantic Versioning https://semver.org/spec/v2.0.0.html).
		 *
		 * For example "dev1" for first development pre-release, or "rc.1" for first release candidate.
		 *
		 * Returns none if GPlates is not a pre-release.
		 *
		 * Note: Currently the only difference compared to Semantic Versioning is the "dev" in development releases
		 *       (which is not part of Semantic Versioning).
		 */
		boost::optional<QString>
		get_GPlates_version_prerelease_suffix();


		///////////////
		// PyGPlates //
		///////////////

		/**
		 * The MAJOR.MINOR.PATCH[PRERELEASE] version of pyGPlates formatted in the PEP440 versioning scheme
		 * (https://www.python.org/dev/peps/pep-0440/).
		 *
		 * Where 'PRERELEASE' is optional and only used for pre-releases.
		 *
		 * For example "1.0.0.dev1" for first development pre-release leading up to official "1.0.0", or
		 * "1.0.0rc1" for first release candidate.
		 */
		QString
		get_pyGPlates_version();

		/**
		 * The MAJOR version number of pyGPlates.
		 */
		unsigned int
		get_pyGPlates_version_major();

		/**
		 * The MINOR version number of pyGPlates.
		 */
		unsigned int
		get_pyGPlates_version_minor();

		/**
		 * The PATCH version number of pyGPlates.
		 */
		unsigned int
		get_pyGPlates_version_patch();

		/**
		 * The optional PRERELEASE version suffix of pyGPlates formatted in the PEP440 versioning scheme
		 * (https://www.python.org/dev/peps/pep-0440/).
		 *
		 * For example ".dev1" for first development pre-release, or "rc1" for first release candidate.
		 *
		 * Returns none if pyGPlates is not a pre-release.
		 */
		boost::optional<QString>
		get_pyGPlates_version_prerelease_suffix();
	}
}

#endif // GPLATES_GLOBAL_VERSION_H
