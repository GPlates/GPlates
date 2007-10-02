/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_UNIQUEID_H
#define GPLATES_UTILS_UNIQUEID_H

#include <string>
#include <unicode/unistr.h>

namespace GPlatesUtils
{
	/**
	 * Generate a unique string identifier.
	 *
	 * To enable the result of this function to serve as XML IDs (which might one day
	 * be useful) without becoming too complicated for us programmers, the resultant
	 * string will conform to the regexp "[A-Za-z_][-A-Za-z_0-9.]*", which is a subset
	 * of the NCName production which defines the set of string values which are valid
	 * for the XML ID type:
	 *  - http://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#ID
	 *  - http://www.w3.org/TR/1999/REC-xml-names-19990114/#NT-NCName
	 *
	 * @return A unique string identifier.
	 *
	 * @pre True.
	 *
	 * @post Return-value is a unique string identifier which conforms to the regexp
	 * "[A-Za-z_][-A-Za-z_0-9.]*", or an exception has been thrown.
	 */
	const UnicodeString
	generate_unique_id();
}

#endif  // GPLATES_UTILS_UNIQUEID_H
