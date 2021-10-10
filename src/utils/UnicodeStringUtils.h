/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_UNICODESTRINGUTILS_H
#define GPLATES_UTILS_UNICODESTRINGUTILS_H

#include <unicode/unistr.h>  // ICU's UnicodeString
#include <QString>


namespace GPlatesUtils
{
	/**
	 * Make a QString from an ICU UnicodeString.
	 *
	 * This won't be super-fast:  It will require memory allocation for the internals of the
	 * QString and O(n) iteration through the characters in the UnicodeString.
	 */
	const QString
	make_qstring_from_icu_string(
			const UnicodeString &icu_string);


	/**
	 * Make a QString from a Unicode string container in the Model.
	 *
	 * This is a convenience function intended for use with Unicode string containers such as
	 * FeatureType, FeatureId, PropertyName and TextContent -- classes which store Unicode
	 * string content in a contained ICU UnicodeString instance which is accessed using the
	 * member function 'T::get()'.
	 */
	template<typename T>
	inline
	const QString
	make_qstring(
			const T &source)
	{
		return make_qstring_from_icu_string(source.get());
	}


	/**
	 * Make a ICU UnicodeString from a QString.
	 */
	const UnicodeString
	make_icu_string_from_qstring(
			const QString &qstring);
}

#endif  // GPLATES_UTILS_UNICODESTRINGUTILS_H
