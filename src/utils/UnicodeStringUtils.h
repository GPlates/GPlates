/* $Id$ */

/**
 * \file
 * Contains functions to convert between UnicodeString instances and QString instances.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007, 2008, 2010 The University of Sydney, Australia
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

#include <functional>
#include <string>
#include <iosfwd>
#include <QString>

#include "global/unicode.h"


namespace GPlatesUtils
{
	/**
	 * Make a QString from an ICU UnicodeString.
	 */
	inline
	const QString
	make_qstring_from_icu_string(
			const GPlatesUtils::UnicodeString &icu_string)
	{
		return icu_string.qstring();
	}

	/**
	 * Make a std::string from an ICU UnicodeString.
	 */
	inline
	const std::string
	make_std_string_from_icu_string(
			const GPlatesUtils::UnicodeString &icu_string)
	{
		return icu_string.qstring().toStdString();
	}

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
	inline
	const GPlatesUtils::UnicodeString
	make_icu_string_from_qstring(
			const QString &qstring)
	{
		return GPlatesUtils::UnicodeString(qstring);
	}

}

#endif  // GPLATES_UTILS_UNICODESTRINGUTILS_H
