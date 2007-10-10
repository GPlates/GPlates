/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006 The University of Sydney, Australia
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

#include <unicode/schriter.h>  // ICU's StringCharacterIterator

#include "UnicodeStringUtils.h"


const QString
GPlatesUtils::make_qstring_from_icu_string(
		const UnicodeString &icu_string)
{
	/*
	 * Rationale for the implementation of this function:
	 *
	 *  1. We're using ICU's UnicodeString:
	 *      http://www.icu-project.org/apiref/icu4c/classUnicodeString.html
	 *
	 *  2. The Unicode standard defines a default encoding based on 16-bit code units.  This is
	 *     supported in ICU by the definition of the UChar to be an unsigned 16-bit integer
	 *     type.  This is the base type for character arrays for strings in ICU.
	 *      -- http://icu-project.org/userguide/strings.html#strings
	 *
	 *  3. In ICU, a Unicode string consists of 16-bit Unicode code units.  A Unicode character
	 *     may be stored with either one code unit (the most common case) or with a matched
	 *     pair of special code units ("surrogates").  The data type for code units is UChar.
	 *     For single-character handling, a Unicode character code point is a value in the
	 *     range 0..0x10ffff.  ICU uses the UChar32 type for code points.  Indexes and offsets
	 *     into and lengths of strings always count code units, not code points.
	 *      -- http://www.icu-project.org/apiref/icu4c/classUnicodeString.html#_details
	 *
	 *  4. The length of a string and all indexes and offsets related to the string are always
	 *     counted in terms of UChar code units, not in terms of UChar32 code points.  [...]
	 *     ICU uses signed 32-bit integers (int32_t) for lengths and offsets.  Because of
	 *     internal computations, strings (and arrays in general) are limited to 1G base units
	 *     or 2G bytes, whichever is smaller.
	 *      -- http://icu-project.org/userguide/strings.html#indexes
	 *
	 *  5. We want to convert to Qt's QString:
	 *      http://doc.trolltech.com/4.2/qstring.html
	 *
	 *  6. QString stores a string of 16-bit QChars, where each QChar corresponds one Unicode
	 *     4.0 character.  (Unicode characters with code values above 65535 are stored using
	 *     surrogate pairs, i.e., two consecutive QChars.)
	 *      -- http://doc.trolltech.com/4.2/qstring.html#details
	 *
	 *  7. The QChar class provides a 16-bit Unicode character.
	 *      -- http://doc.trolltech.com/4.2/qchar.html#details
	 *
	 *  8. Thus, it seems that what the Qt docs refer to as a "Unicode character" is actually a
	 *     16-bit Unicode code unit, rather than a Unicode code point (which is what would more
	 *     correctly be referred-to as a "Unicode character").
	 *
	 *  9. There's no way to instantiate a QString from an array of unsigned 16-bit integers.
	 *      -- http://doc.trolltech.com/4.2/qstring.html#QString
	 *
	 *  10. Thus, we'd need to append code units (in the form of QChar instances) one-by-one.
	 *       -- http://doc.trolltech.com/4.2/qstring.html#append-5
	 *
	 *  11. So, we would create an empty QString, reserve space for the appropriate number of
	 *      code units, and iterate through the code units in the UnicodeString, instantiating
	 *      a QChar instance for each and appending it to the QString.
	 *       http://doc.trolltech.com/4.2/qstring.html#QString
	 *       http://doc.trolltech.com/4.2/qstring.html#reserve
	 *       http://doc.trolltech.com/4.2/qchar.html#QChar-6
	 *       http://doc.trolltech.com/4.2/qstring.html#append-5
	 *
	 *  12. The function 'UnicodeString::length' returns the number of 16-bit code units.
	 *       http://www.icu-project.org/apiref/icu4c/classReplaceable.html#6b41489ab29778f61016edb5e89f0b14
	 *
	 *  13. UnicodeString provides functions for random access and use [...] of both code units
	 *      and code points.  [...]  Code point and code unit iteration is provided by the
	 *      CharacterIterator abstract class and its subclasses.  There are concrete iterator
	 *      implementations for UnicodeString objects [...].
	 *       -- http://icu-project.org/userguide/strings.html#ustrings_cpp
	 *
	 *  14. Using CharacterIterator ICU iterates over text that is independent of its storage
	 *      method.  The text can be stored locally or remotely in a string, file, database, or
	 *      other method.  [...]  ICU provides the following concrete subclasses of the
	 *      CharacterIterator class:  [...]  StringCharacterIterator subclass extends from
	 *      UCharCharacterIterator and iterates over the contents of a UnicodeString.
	 *       -- http://icu-project.org/userguide/characterIterator.html
	 *
	 *  15. So, let's use StringCharacterIterator.
	 *       http://icu-project.org/apiref/icu4c/classStringCharacterIterator.html
	 *       http://icu-project.org/apiref/icu4c/classCharacterIterator.html
	 */

	QString qstring;
	qstring.reserve(icu_string.length());

	StringCharacterIterator iter(icu_string);

	// This code is based upon the examples on the page
	//  http://icu-project.org/apiref/icu4c/classCharacterIterator.html#_details
	UChar code_unit;
	for (iter.setToStart(); iter.hasNext(); ) {
		UCharCharacterIterator &iterator = iter;
		code_unit = iterator.nextPostInc();
		qstring.append(QChar(code_unit));
	}

	return qstring;
}
