/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_STRINGUTILS_H
#define GPLATES_UTILS_STRINGUTILS_H

#include <string>
#include <QString>


namespace GPlatesUtils
{
	/**
	 * Converts a std::wstring instance into a QString instance.
	 *
	 * On Windows, Qt is compiled without wchar_t as a native type:
	 * http://www.qtforum.org/article/17883/problem-using-qstring-fromstdwstring.html
	 * But GPlates is, so you'll get a linker error if you call
	 * QString::fromStdWString(). This function contains a workaround.
	 *
	 * On other platforms, this just calls QString::fromStdWString().
	 */
	QString
	make_qstring_from_wstring(
			const std::wstring &str);


	/**
	 * Converts a QString instance into a std::wstring instance.
	 *
	 * On Windows, this contains a workaround; see comments above for
	 * @a make_qstring_from_wstring.
	 *
	 * On other platforms, this just calls QString::toStdWString().
	 */
	std::wstring
	make_wstring_from_qstring(
			const QString &str);
}

#endif  // GPLATES_UTILS_STRINGUTILS_H
