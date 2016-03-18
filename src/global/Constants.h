/* $Id$ */

/**
 * \file 
 * This file contains typedefs that are used in various places in
 * the code; they are not tied to any particular class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_CONSTANTS_H
#define GPLATES_GLOBAL_CONSTANTS_H

namespace GPlatesGlobal 
{
	/**
	 * The string "GPlates <MAJOR>.<MINOR>.<PATCH>".
	 *
	 * For example "GPlates 1.4.0".
	 */
	extern const char VersionString[];

	/**
	 * The MAJOR.MINOR.PATCH version number of GPlates.
	 *
	 * For example "1.4.0".
	 */
	extern const char GPlatesVersion[];

	/**
	 * The MAJOR version number of GPlates.
	 *
	 * For example "1".
	 */
	extern const char GPlatesVersionMajor[];

	/**
	 * The MINOR version number of GPlates.
	 *
	 * For example "4".
	 */
	extern const char GPlatesVersionMinor[];

	/**
	 * The PATCH version number of GPlates.
	 *
	 * For example "0".
	 */
	extern const char GPlatesVersionPatch[];

	extern const char CopyrightString[];

	extern const char HtmlCopyrightString[];

	/**
	 * The Subversion revision number of GPlates.
	 */
	extern const char SourceCodeControlVersionString[];
}

#endif // GPLATES_GLOBAL_CONSTANTS_H
