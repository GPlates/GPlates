/* $Id$ */

/**
 * \file
 * File specific comments.
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

#ifndef GPLATES_UTIL_STRINGUTILS_H
#define GPLATES_UTIL_STRINGUTILS_H

#include <cctype>
#include <boost/lexical_cast.hpp>

namespace GPlatesUtil
{
	// FIXME:  Make this a proper GPlates exception.
	struct BadConversionException { };

	/**
	 * Slice the string @a source between the index @a start and the index @a end, convert the
	 * slice to the type of @a dest and store the result in @a dest.
	 *
	 * The indices @a start and @a end are used to specify a substring as in the Python "slice"
	 * notation:  The @a start index is the index of the first character to be included in the
	 * slice; the @a end index is the first character, after the end of the slice, @em not to
	 * be included.
	 *
	 * Leading and trailing whitespace in the slice are by default stripped; this behaviour can
	 * be altered by passing 'false' to the parameters @a should_strip_leading_whitespace and
	 * @a should_strip_trailing_whitespace, respectively.
	 */
	// Might want to provide an overload for UnicodeString which uses unum.h functions instead.
	template<class T>
	void
 	slice_string(
		T &dest,
		const std::string &source,
		std::string::size_type start,
		std::string::size_type end = std::string::npos,
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
			dest = boost::lexical_cast<T>(result);
		} catch (boost::bad_lexical_cast error) {
			throw BadConversionException(); 
		}
	}
}

#endif
