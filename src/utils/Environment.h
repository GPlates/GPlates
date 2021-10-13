/* $Id$ */

/**
 * \file A wrapper around std::getenv() that returns QStrings.
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

#ifndef GPLATES_UTILS_ENVIRONMENT_H
#define GPLATES_UTILS_ENVIRONMENT_H

#include <QString>


namespace GPlatesUtils
{

	/**
	 * Wrap a call to std::getenv() so that we return a QString,
	 * which is much more friendly to deal with locale-aware upper/lower case transforms
	 * without having to use the stupid STL algorithm. Having a QString means you can also
	 * immediately use operator==, and even regexes if that's your thing.
	 *
	 * You can check @a .isNull() on the string to test if the variable was not defined,
	 * but if you don't care, that's fine, operator== will work just fine even on a null QString.
	 */
	QString
	getenv(
			const char *variable_name);
	
	/**
	 * Test for an environment variable's "truthiness", to allow users to easily export
	 * variables as "1" or "true" or "yes" etc.
	 *
	 * If the environment variable is not defined, returns the @a default_value.
	 */
	bool
	getenv_as_bool(
			const char *variable_name,
			bool default_value);

}



#endif // GPLATES_UTILS_ENVIRONMENT_H
