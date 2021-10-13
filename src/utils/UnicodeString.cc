/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <ostream>
#include "UnicodeString.h"


boost::int32_t
GPlatesUtils::UnicodeString::length() const
{
	// Implementing
	// http://icu-project.org/apiref/icu4c/classUnicodeString.html#a772ced3c5e5c737d07a05adb3818f37
	// 
	// QString::length (and equivalently, QString::size) return "the number of characters in
	// this string", which, since Qt uses its 16-bit QChar type as a single "character", would
	// seem to be what we want.
	//
	// (The Unicode code-point space requires 32 bits rather than 16 bits, so the 16-bit QChars
	// can only represent 16-bit code units rather than 32-bit code points, so we must assume
	// that QString is talking about code units rather than code points.)
	//
	// http://doc.qt.nokia.com/4.4/qstring.html#length
	// http://doc.qt.nokia.com/4.4/qstring.html#size
	// http://doc.qt.nokia.com/4.4/qchar.html
	return d_qstring.length();
}


void
GPlatesUtils::UnicodeString::extractBetween(
		boost::int32_t start,
		boost::int32_t limit,
		UnicodeString &target)
{
	// Implementing
	// http://icu-project.org/apiref/icu4c/classUnicodeString.html#d8946e6ca397f9b37a60a6a3c1a2ab93
	// in terms of
	// http://doc.qt.nokia.com/4.4/qstring.html#mid
	int n = limit - start;
	target = UnicodeString(qstring().mid(start, n));
}


GPlatesUtils::UnicodeString &
GPlatesUtils::UnicodeString::removeBetween(
		boost::int32_t start,
		boost::int32_t limit)
{
	// Implementing
	// http://icu-project.org/apiref/icu4c/classUnicodeString.html#46ca3daa10b0bcbcc4d75da6b7496f4e
	// in terms of
	// http://doc.qt.nokia.com/4.4/qstring.html#remove
	int n = limit - start;
	d_qstring.remove(start, n);
	return *this;
}


boost::int32_t
GPlatesUtils::UnicodeString::indexOf(
		const UnicodeString &text) const
{
	// Implementing
	// http://icu-project.org/apiref/icu4c/classUnicodeString.html#8f3956140af1d4d9d255e5da837b297c
	// in terms of
	// http://doc.qt.nokia.com/4.4/qstring.html#indexOf
	return qstring().indexOf(text.qstring());
}


boost::int32_t
GPlatesUtils::UnicodeString::indexOf(
		const UnicodeString &text,
		boost::int32_t start) const
{
	// Implementing
	// http://icu-project.org/apiref/icu4c/classUnicodeString.html#81248ae2f8f2700f808c3fdf14a2ee67
	// in terms of
	// http://doc.qt.nokia.com/4.4/qstring.html#indexOf
	return qstring().indexOf(text.qstring(), start);
}


std::ostream &
std::operator<<(
		ostream &os,
		const GPlatesUtils::UnicodeString &us)
{
	const QString &qs = us.qstring();

	// The only way we can insert the QString into a single-byte std::ostream is to convert it
	// to UTF-8: http://doc.qt.nokia.com/4.4/qstring.html#toUtf8
	QByteArray bytes = qs.toUtf8();

	// Unfortunately, the Internet cannot seem to agree whether or not 0x00 bytes can occur in
	// the sequence of bytes containing a UTF-8 string (other than a single 0x00 byte used
	// specifically to represent the ASCII NUL character '\0', of course).  Hence, I can't
	// determine whether I need to worry about a 0x00 byte appearing in this sequence of bytes,
	// and being misinterpreted by std::ostream as the null-terminator.
	//
	// 'QByteArray::constData' returns "the bytes that compose the array", which is guaranteed
	// to be "'\0'-terminated".  However, the documentation warns:
	//
	//   Note: A QByteArray can store any byte values including '\0's, but most functions that
	//   take char * arguments assume that the data ends at the first '\0' they encounter.
	// http://doc.qt.nokia.com/4.4/qbytearray.html#constData
	//
	// Hence, I'll err on the side of safety, and use the function 'ostream::write'.
	//
	// Ultimately, this whole problem would be avoided if we replaced all usage of std::ostream
	// with std::wostream in the GPlates source code, and then used 'QString::utf16' instead of
	// 'QString::toUtf8': http://doc.qt.nokia.com/4.4/qstring.html#utf16
	os.write(bytes.constData(), bytes.count() + 1);

	return os;
}
