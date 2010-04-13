/* $Id$ */

/**
 * \file
 * File specific comments.
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

#include <vector>
#include <unicode/schriter.h>  // ICU's StringCharacterIterator
#include <iostream>

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
		// The (apparently redundant) line of code which follows is actually a small hack
		// to enable the resolution of the 'nextPostInc' symbol on MSVC7.1.  Without this
		// hack, linking will fail.  This is apparently due to a bug in MSVC7.1.
		UCharCharacterIterator &iterator = iter;
		code_unit = iterator.nextPostInc();

		qstring.append(QChar(code_unit));
	}

	return qstring;
}


const std::string
GPlatesUtils::make_std_string_from_icu_string(
		const UnicodeString &icu_string)
{
	return make_qstring_from_icu_string(icu_string).toStdString();
}


const UnicodeString
GPlatesUtils::make_icu_string_from_qstring(
		const QString &qstring)
{
	/**
	 * Rationale for the implementation of this function:
	 *
	 *  1. We're using Qt's QString:
	 *      http://doc.trolltech.com/4.2/qstring.html
	 *
	 *  2. QString stores a string of 16-bit QChars, where each QChar corresponds one Unicode
	 *     4.0 character.  (Unicode characters with code values above 65535 are stored using
	 *     surrogate pairs, i.e., two consecutive QChars.)
	 *      http://doc.trolltech.com/4.2/qstring.html#details
	 *
	 *  3. The QChar class provides a 16-bit Unicode character.
	 *      -- http://doc.trolltech.com/4.2/qchar.html#details
	 *
	 *  4. Thus, it seems that what the Qt docs refer to as a "Unicode character" is actually a
	 *     16-bit Unicode code unit, rather than a Unicode code point (which is what would more
	 *     correctly be referred-to as a "Unicode character").
	 *
	 *  5. We want to convert to ICU's UnicodeString:
	 *      http://www.icu-project.org/apiref/icu4c/classUnicodeString.html
	 *
	 *  6. The Unicode standard defines a default encoding based on 16-bit code units.  This is
	 *     supported in ICU by the definition of the UChar to be an unsigned 16-bit integer
	 *     type.  This is the base type for character arrays for strings in ICU.
	 *      -- http://icu-project.org/userguide/strings.html#strings
	 *
	 *  7. In ICU, a Unicode string consists of 16-bit Unicode code units.  A Unicode character
	 *     may be stored with either one code unit (the most common case) or with a matched
	 *     pair of special code units ("surrogates").  The data type for code units is UChar.
	 *     For single-character handling, a Unicode character code point is a value in the
	 *     range 0..0x10ffff.  ICU uses the UChar32 type for code points.  Indexes and offsets
	 *     into and lengths of strings always count code units, not code points.
	 *      -- http://www.icu-project.org/apiref/icu4c/classUnicodeString.html#_details
	 *
	 *  8. The length of a string and all indexes and offsets related to the string are always
	 *     counted in terms of UChar code units, not in terms of UChar32 code points.  [...]
	 *     ICU uses signed 32-bit integers (int32_t) for lengths and offsets.  Because of
	 *     internal computations, strings (and arrays in general) are limited to 1G base units
	 *     or 2G bytes, whichever is smaller.
	 *      -- http://icu-project.org/userguide/strings.html#indexes
	 *
	 *  9. UnicodeString provides a constructor which accepts the paremeters
	 *      - const UChar *text
	 *      - int32_t textLength
	 *      http://www.icu-project.org/apiref/icu4c/classUnicodeString.html#37d5560f5f05e53143da7765ecd9aff0
	 *
	 *  10. Thus, we would determine the number of code units ("characters" in the QString
	 *      documentation) in the QString instance using the 'size' member function:
	 *       http://doc.trolltech.com/4.2/qstring.html#size
	 *
	 *  11. If the size of the QString instance is zero, return an empty UnicodeString instance
	 *      using the default constructor.  (This is not just a lame optimisation; it actually
	 *      has significance in the use of 'std::vector' below.)
	 *
	 *  12. Then, we would instantiate a 'std::vector' of UChar on the stack, reserve the
	 *      capacity of the 'std::vector' instance using the 'reserve' member function, then
	 *      obtain the QChar contents of the QString instance using the 'data' member function:
	 *       http://doc.trolltech.com/4.2/qstring.html#data-2
	 *
	 *  13. Then iterate through the array of QChar, and, for each QChar instance, invoke the
	 *      QChar member function 'unicode' to obtain the 'unsigned short' numeric value, which
	 *      is then appended to the 'std::vector' instance.
	 *       http://doc.trolltech.com/4.2/qchar.html#unicode
	 *
	 *  14. Once all the QChars in the QString instance have been copied as 'unsigned short's
	 *      into the 'std::vector' instance, access the internal buffer of the 'std::vector'
	 *      instance by taking the address of the element at index 0 of the 'std::vector'
	 *      instance.  (This is why we returned from the function early in step 11 if the size
	 *      of the QString instance was zero:  It is not valid to index a 'std::vector'
	 *      instance whose length is zero.)  This internal buffer will be an array of UChar.
	 *      (According to Meyers01, "Effective STL", Item 16 "Know how to pass vector and
	 *      string data to legacy APIs", the elements in a vector are guaranteed by the C++
	 *      Standard to be stored in contiguous memory, just like the elements in an array.)
	 *
	 *  15. Pass this array to the UnicodeString constructor which accepts (and copies) an
	 *      array of const UChar:
	 *       http://www.icu-project.org/apiref/icu4c/classUnicodeString.html#37d5560f5f05e53143da7765ecd9aff0
	 */

	int qstring_size = qstring.size();
	if (qstring_size == 0) {
		// Read point 14 (in the comment above) to understand why we're returning early if
		// the size is zero.
		return UnicodeString();
	}
	std::vector<UChar> uchars;
	uchars.reserve(qstring_size);
	const QChar *qstring_contents = qstring.data();
	for (int i = 0; i < qstring_size; ++i) {
		uchars.push_back(qstring_contents[i].unicode());
	}
	// Note that, since the size of the QString instance is greater than zero, 'uchars[0]' is
	// valid.
	return UnicodeString(&(uchars[0]), qstring_size);
}
