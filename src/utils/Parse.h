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
	 * Specialisations must provide a public function operator that takes a
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
		parse_using_qstring(
				const QString &s,
				FunctionType fn)
		{
			bool ok;
			T result = (s.*fn)(&ok);
			if (ok)
			{
				return result;
			}
			else
			{
				throw ParseError();
			}
		}


		template<typename T, typename FunctionType>
		T
		parse_using_qstring(
				const QString &s,
				FunctionType fn,
				int base)
		{
			bool ok;
			T result = (s.*fn)(&ok, base);
			if (ok)
			{
				return result;
			}
			else
			{
				throw ParseError();
			}
		}

		
		class ParseWithBase
		{
		public:

			ParseWithBase(
					int base) :
				d_base(base)
			{
			}

		protected:

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
		{
		}

		int
		operator()(
				const QString &s)
		{
			return ParseInternals::parse_using_qstring<int>(s, &QString::toInt, d_base);
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
		{
		}

		unsigned int
		operator()(
				const QString &s)
		{
			return ParseInternals::parse_using_qstring<unsigned int>(s, &QString::toUInt, d_base);
		}
	};


	/**
	 * Template specialisation of Parse for long.
	 */
	template<>
	struct Parse<long> :
			public ParseInternals::ParseWithBase
	{
		Parse(
				int base = 10) :
			ParseWithBase(base)
		{
		}

		long
		operator()(
				const QString &s)
		{
			return ParseInternals::parse_using_qstring<long>(s, &QString::toLong, d_base);
		}
	};


	/**
	 * Template specialisation of Parse for unsigned long.
	 */
	template<>
	struct Parse<unsigned long> :
			public ParseInternals::ParseWithBase
	{
		Parse(
				int base = 10) :
			ParseWithBase(base)
		{
		}

		unsigned long
		operator()(
				const QString &s)
		{
			return ParseInternals::parse_using_qstring<long>(s, &QString::toULong, d_base);
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
	class Parse<Int<Base, IntType> >
	{
	public:

		Int<Base, IntType>
		operator()(
				const QString &s)
		{
			Parse<IntType> parse(Base);
			return parse(s);
		}
	};


	/**
	 * Template specialisation of Parse for float.
	 */
	template<>
	struct Parse<float>
	{
		float
		operator()(
				const QString &s)
		{
			return ParseInternals::parse_using_qstring<float>(s, &QString::toFloat);
		}
	};


	/**
	 * Template specialisation of Parse for double.
	 */
	template<>
	struct Parse<double>
	{
		double
		operator()(
				const QString &s)
		{
			return ParseInternals::parse_using_qstring<double>(s, &QString::toDouble);
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
				const QString &s)
		{
			QString lower = s.toLower();
			if (lower == "true")
			{
				return true;
			}
			else if (lower == "false")
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
				const QString &s)
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
				const QString &s)
		{
			return s;
		}
	};
}

#endif  // GPLATES_UTILS_SINGLETON_H
