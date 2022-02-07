/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include <algorithm>
#include <boost/static_assert.hpp>
#include <boost/scoped_array.hpp>

#include "StringUtils.h"


QString
GPlatesUtils::make_qstring_from_wstring(
		const std::wstring &str)
{
#if defined(_MSC_VER)
	// Setting the /Zc:wchar_t- compiler flag won't work in our case because
	// Boost is compiled with wchar_t as a native type so we're just moving the
	// problem elsewhere.
	// The workaround is to assume wchar_t is 16-bit on Windows and that the
	// encoding in the given wstring is UTF-16. We cast the wchar_t's into
	// unsigned shorts and then call QString::fromUtf16.
	BOOST_STATIC_ASSERT(sizeof(wchar_t) == sizeof(unsigned short));

	boost::scoped_array<unsigned short> utf16(new unsigned short[str.size()]);
	std::copy(str.begin(), str.end(), utf16.get());

	return QString::fromUtf16(utf16.get(), str.size());
#else
	return QString::fromStdWString(str);
#endif
}


std::wstring
GPlatesUtils::make_wstring_from_qstring(
		const QString &str)
{
#if defined(_MSC_VER)
	// See comments above in @a make_qstring_from_wstring.
	BOOST_STATIC_ASSERT(sizeof(wchar_t) == sizeof(unsigned short));

	const unsigned short *utf16 = str.utf16(); // null-terminated
	std::wstring result;
	while (*utf16)
	{
		result.push_back(static_cast<wchar_t>(*utf16));
		++utf16;
	}

	return result;
#else
	return str.toStdWString();
#endif
}

