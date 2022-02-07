/* $Id$ */

/**
 * \file
 * Defines the Parse template class.
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

#ifndef GPLATES_UTILS_PARSE_H
#define GPLATES_UTILS_PARSE_H

#include <functional>
#include <QLocale>
#include <QString>


namespace GPlatesUtils
{
	class ParseError {  };

	/**
	 * The Parse template class can be used when a string representation of a
	 * non-string value needs to be turned into that non-string value.
	 *
	 * This header file provides specialisations of Parse for common types.
	 * However, the idea is for specialisations of Parse to accompany the
	 * definitions of other types that can be converted from a string.
	 *
	 * Specialisations must provide a const public function operator that takes a
	 * const QString & parameter and returns, by value, the parsed value.
	 * ParseError is to be thrown when the provided string cannot be parsed.
	 */
	template<typename T>
	struct Parse;
		// Intentionally not defined.


	namespace ParseInternals
	{
		template<typename T, typename FunctionType>
		T
		parse_using_qlocale(
				const QLocale &loc,
				const QString &s,
				FunctionType fn)
		{
			bool ok;

			// Attempt to parse using provided locale.
			T result = (loc.*fn)(s, &ok);
			if (ok)
			{
				return result;
			}

			// Attempt to parse using "C" locale.
			static const QLocale C_LOCALE = QLocale::c();
			if (C_LOCALE != loc)
			{
				result = (C_LOCALE.*fn)(s, &ok);
				if (ok)
				{
					return result;
				}
			}

			throw ParseError();
		}


		template<typename T, typename FunctionType>
		T
		parse_using_qstring(
				const QString &s,
				FunctionType fn,
				int base)
		{
			bool ok;

			// Parse using the QString.
			// Locale-based "base" encoding does not make sense, and was removed in Qt5.
			T result = (s.*fn)(&ok, base);
			if (ok)
			{
				return result;
			}

			throw ParseError();
		}


		class ParseWithLocale
		{
		protected:

			QLocale d_locale;
		};

		
		class ParseWithBase :
				public ParseWithLocale
		{
		protected:

			ParseWithBase(
					int base) :
				d_base(base)
			{  }

			int d_base;
		};
	}


	/**
	 * Template specialisation of Parse for int.
	 */
	template<>
	struct Parse<int> :
			public ParseInternals::ParseWithBase
	{
		Parse(
				int base = 10) :
			ParseWithBase(base)
		{  }

		int
		operator()(
				const QString &s) const
		{
			// Locale-based "base" encoding does not make sense, and was removed in Qt5.
			// Use QLocale for base 10, otherwise use QString.
			if (d_base == 10)
			{
				int (QLocale::*mfp)(const QString &, bool *) const = &QLocale::toInt;  // Select correct overload
				return ParseInternals::parse_using_qlocale<int>(d_locale, s, mfp);
			}
			else
			{
				return ParseInternals::parse_using_qstring<int>(s, &QString::toInt, d_base);
			}
		}
	};


	/**
	 * Template specialisation of Parse for unsigned int.
	 */
	template<>
	struct Parse<unsigned int> :
			public ParseInternals::ParseWithBase
	{
		Parse(
				int base = 10) :
			ParseWithBase(base)
		{  }

		unsigned int
		operator()(
				const QString &s) const
		{
			// Locale-based "base" encoding does not make sense, and was removed in Qt5.
			// Use QLocale for base 10, otherwise use QString.
			if (d_base == 10)
			{
				uint (QLocale::*mfp)(const QString &, bool *) const = &QLocale::toUInt;  // Select correct overload
				return ParseInternals::parse_using_qlocale<unsigned int>(d_locale, s, mfp);
			}
			else
			{
				return ParseInternals::parse_using_qstring<unsigned int>(s, &QString::toUInt, d_base);
			}
		}
	};


	/**
	 * A wrapper around int to allow integers expressed in a base other than 10 to
	 * be parsed correctly.
	 *
	 * For all intents and purposes, an object of type Int<Base> can be used just
	 * like an int (because of the conversions to and from int), but when used with
	 * Parse, the Base template parameter is used to indicate the base in which the
	 * string representation of the integer is expressed.
	 *
	 * An alternative method of parsing integers expressed in a base that is not 10
	 * is to pass in the base into the Parse<int> constructor.
	 */
	template<int Base, typename IntType = int>
	class Int
	{
	public:

		Int(
				IntType value) :
			d_value(value)
		{
		}

		inline
		operator IntType() const
		{
			return d_value;
		}

	private:

		IntType d_value;
	};


	/**
	 * Template specialisation of Parse for Int<Base, IntType>.
	 */
	template<int Base, typename IntType>
	struct Parse<Int<Base, IntType> >
	{
	public:

		Parse() :
			d_parse(Base)
		{  }

		Int<Base, IntType>
		operator()(
				const QString &s) const
		{
			return d_parse(s);
		}

	private:

		Parse<IntType> d_parse;
	};


	/**
	 * Template specialisation of Parse for float.
	 */
	template<>
	struct Parse<float> :
			public ParseInternals::ParseWithLocale
	{
		float
		operator()(
				const QString &s) const
		{
			float (QLocale::*mfp)(const QString &, bool *) const = &QLocale::toFloat;  // Select correct overload
			return ParseInternals::parse_using_qlocale<float>(d_locale, s, mfp);
		}
	};


	/**
	 * Template specialisation of Parse for double.
	 */
	template<>
	struct Parse<double> :
			public ParseInternals::ParseWithLocale
	{
		double
		operator()(
				const QString &s) const
		{
			double (QLocale::*mfp)(const QString &, bool *) const = &QLocale::toDouble;  // Select correct overload
			return ParseInternals::parse_using_qlocale<double>(d_locale, s, mfp);
		}
	};


	/**
	 * Template specialisation of Parse for bool.
	 */
	template<>
	struct Parse<bool>
	{
		bool
		operator()(
				const QString &s) const
		{
			static const QString TRUE_STRING = "true";
			static const QString FALSE_STRING = "false";

			QString lower = s.toLower();
			if (lower == TRUE_STRING)
			{
				return true;
			}
			else if (lower == FALSE_STRING)
			{
				return false;
			}
			else
			{
				throw ParseError();
			}
		}
	};


	/**
	 * Template specialisation of Parse for const QString &.
	 */
	template<>
	struct Parse<const QString &>
	{
		const QString &
		operator()(
				const QString &s) const
		{
			return s;
		}
	};


	/**
	 * Template specialisation of Parse for QString.
	 */
	template<>
	struct Parse<QString>
	{
		QString
		operator()(
				const QString &s) const
		{
			return s;
		}
	};
}

#endif  // GPLATES_UTILS_SINGLETON_H
