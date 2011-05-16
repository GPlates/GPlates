/* $Id$ */

/**
 * \file 
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_UTILS_UNICODESTRING_H
#define GPLATES_UTILS_UNICODESTRING_H

#include <boost/cstdint.hpp>
#include <iosfwd>
#include <QString>

// This is a hold-over from using ICU's UnicodeString.
#ifndef GPLATES_ICU_BOOL
// The ICU UnicodeString binary comparison operators returned a UBool rather than a bool,
// which caused problems.  Hence, we needed to wrap all binary comparisons in this macro.
//#define GPLATES_ICU_BOOL(b) ((b) != 0)
// The macro still pervades the code; for the time being, let's make it idempotent.
#define GPLATES_ICU_BOOL(b) (b)
#endif


namespace GPlatesUtils
{
	/**
	 * A wrapper class around QString which mirrors the interface of ICU's UnicodeString as needed.
	 *
	 * http://icu-project.org/apiref/icu4c/classUnicodeString.html
	 */
	class UnicodeString
	{
	public:
		UnicodeString()
		{  }

		explicit
		UnicodeString(
				const QString &qs):
			d_qstring(qs)
		{  }

		/**
		 * Construct a UnicodeString instance from a null-terminated array of chars.
		 *
		 * http://icu-project.org/apiref/icu4c/classUnicodeString.html#2e81e482db97eb362b6d0d62ff331ca3
		 *
		 * It seems that this constructor is not explicit in ICU UnicodeString.
		 */
		UnicodeString(
				const char *s)://We assume s contains Ascii data. Don't try to pass in local code page data.
			d_qstring(s)
		{  }

		/**
		 * Access the underlying internal QString by ref-to-const.
		 *
		 * This is the only member function that breaks the illusion of ICU UnicodeString.
		 */
		const QString &
		qstring() const
		{
			return d_qstring;
		}

		/**
		 * Determine if this string is empty.
		 *
		 * http://icu-project.org/apiref/icu4c/classUnicodeString.html#4004ef18a48eafbefc4bbc67cb12dcdf
		 */
		bool
		isEmpty() const
		{
			return d_qstring.isEmpty();
		}

		/**
		 * Return the length of the UnicodeString object.
		 *
		 * The length is the number of UChar code units are in the UnicodeString. If you
		 * want the number of code points, please use countChar32().
		 *
		 * http://icu-project.org/apiref/icu4c/classUnicodeString.html#a772ced3c5e5c737d07a05adb3818f37
		 */
		boost::int32_t
		length() const;

		/**
		 * Locate in this the first occurrence of the characters in @a text, using bitwise
		 * comparison.
		 *
		 * http://icu-project.org/apiref/icu4c/classUnicodeString.html#8f3956140af1d4d9d255e5da837b297c
		 */
		boost::int32_t
		indexOf(
				const UnicodeString &text) const;

		/**
		 * Locate in this the first occurrence of the characters in @a text starting at offset
		 * @a start, using bitwise comparison.
		 *
		 * http://icu-project.org/apiref/icu4c/classUnicodeString.html#81248ae2f8f2700f808c3fdf14a2ee67
		 */
		boost::int32_t
		indexOf(
				const UnicodeString &text,
				boost::int32_t start) const;

		/**
		 * Copy the characters in the range [start, limit) into the UnicodeString @a target.
		 *
		 * http://icu-project.org/apiref/icu4c/classUnicodeString.html#d8946e6ca397f9b37a60a6a3c1a2ab93
		 */
		void
		extractBetween(
				boost::int32_t start,
				boost::int32_t limit,
				UnicodeString &target);

		/**
		 * Remove the characters in the range [start, limit) from the UnicodeString object.
		 *
		 * http://icu-project.org/apiref/icu4c/classUnicodeString.html#46ca3daa10b0bcbcc4d75da6b7496f4e
		 */
		UnicodeString &
		removeBetween(
				boost::int32_t start,
				boost::int32_t limit);

		UnicodeString &
		operator+=(
				const UnicodeString &other)
		{
			d_qstring += other.qstring();
			return *this;
		}

	private:
		QString d_qstring;
	};


	inline
	bool
	operator==(
			const UnicodeString &us1,
			const UnicodeString &us2)
	{
		return us1.qstring() == us2.qstring();
	}


	inline
	bool
	operator<(
			const UnicodeString &us1,
			const UnicodeString &us2)
	{
		return us1.qstring() < us2.qstring();
	}


	inline
	const UnicodeString
	operator+(
			const UnicodeString &us1,
			const UnicodeString &us2)
	{
		return UnicodeString(us1.qstring() + us2.qstring());
	}
}


namespace std
{
	ostream &
	operator<<(
			ostream &os,
			const GPlatesUtils::UnicodeString &us);
}

#endif  // GPLATES_UTILS_UNICODESTRING_H
