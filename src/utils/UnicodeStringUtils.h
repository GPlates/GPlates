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
#include <unicode/ustream.h> // operator<<
#include <functional>
#include <string>
#include <iosfwd>
#include <QString>


#ifndef GPLATES_ICU_BOOL
#define GPLATES_ICU_BOOL(b) ((b) != 0)
#endif

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
	 * Make a std::string from an ICU UnicodeString.
	 */
	const std::string
	make_std_string_from_icu_string(
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


namespace std
{
	/**
	 * Template specialisation std::less<UnicodeString> to support std::map, etc.
	 * This is because on Visual Studio we get the following error:
	 *   "warning C4800: 'UBool' : forcing value to bool 'true' or 'false' (performance warning)"
	 * and since warnings are treated as errors we get failed compilation.
	 * This is because "operator<" for UnicodeString returns UBool.
	 */
	template<>
	struct less<UnicodeString>
		: public binary_function<UnicodeString, UnicodeString, bool>
	{
		bool
		operator()(
				const UnicodeString &lhs,
				const UnicodeString &rhs) const
		{
			// Solution is to explicitly compare returned UBool with zero.
			return GPLATES_ICU_BOOL(lhs < rhs);
		}
	};
}

#endif  // GPLATES_UTILS_UNICODESTRINGUTILS_H
