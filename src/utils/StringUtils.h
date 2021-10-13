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

#include <cctype>
#include <string>
#include <boost/lexical_cast.hpp>
#include <QString>


namespace GPlatesUtils
{
	/**
	 * Slice the string @a source between the index @a start and the index @a end, convert the
	 * slice to template argument Type and return the result in @a dest.
	 *
	 * The indices @a start and @a end are used to specify a substring as in the Python "slice"
	 * notation:  The @a start index is the index of the first character to be included in the
	 * slice; the @a end index is the first character, after the end of the slice, @em not to
	 * be included.
	 * 
	 * The std::string::npos value can be used for the third parameter ('end') to indicate 
	 * the slice commences at 'start' and ends at the last character in the string.
	 *
	 * Leading and trailing whitespace in the slice are by default stripped; this behaviour can
	 * be altered by passing 'false' to the parameters @a should_strip_leading_whitespace and
	 * @a should_strip_trailing_whitespace, respectively.
	 */
	// Might want to provide an overload for UnicodeString which uses unum.h functions instead.
	template<typename Type, typename Error>
	Type
 	slice_string(
			const std::string &source,
			std::string::size_type start,
			std::string::size_type end,
			const Error &error,
			bool should_strip_leading_whitespace = true,
			bool should_strip_trailing_whitespace = true)
	{
		std::string result;

		// Ensure that (end - 1) is a valid index into 'source' (ie, that 'end' is not past
		// the end of 'source').
		// 
		// (Note that we don't need to perform the same test for 'start', since it will
		// only be used as an index into 'source' if it is less than 'end':  Since this
		// if-statement will guarantee that 'end' is a valid index, if 'start' is less than
		// 'end', 'start' will also be a valid index.)
		if (end > source.length()) {
			end = source.length();
		}
		if (should_strip_leading_whitespace) {
			while (start < end && isspace(source[start])) {
				start++;
			}
		}
		if (should_strip_trailing_whitespace) {
			while (end > start && isspace(source[end - 1])) {
				end--;
			}
		}

		// Ensure that 'end' is greater than 'start'.
		//
		// This serves two purposes:
		// 1. We don't bother calculating (and assigning) empty sub-strings (when 'end' is
		//    equal to 'start').
		// 2. We don't risk any strange behaviour (such as 'out_of_range' or 'length_error'
		//    being thrown because 'start' is past the end of the string or (end - start)
		//    is a very large unsigned number because 'end' is less than 'start').
		if (end > start) {
			result = source.substr(start, end - start);
		}

		try {
			return boost::lexical_cast<Type>(result);
		} catch (boost::bad_lexical_cast cast_error) {
			throw error; 
		}
	}


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
