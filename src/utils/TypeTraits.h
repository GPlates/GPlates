/* $Id$ */

/**
 * @file 
 * Contains the definition of the TypeTraits template class.
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

#ifndef GPLATES_UTILS_TYPETRAITS_H
#define GPLATES_UTILS_TYPETRAITS_H

#include <boost/mpl/set.hpp>

#include "Select.h"

namespace GPlatesUtils
{
	namespace TypeTraitsInternals
	{
		typedef boost::mpl::set<
			unsigned char,
			unsigned short int,
			unsigned int,
			unsigned long int,
			signed char,
			short int,
			int,
			long int,
			bool,
			char,
			wchar_t,
			float,
			double,
			long double
		> built_in_types;

		template<typename T>
		class IsBuiltIn
		{
			typedef typename boost::mpl::has_key<built_in_types, T>::type result_type;

		public:

			enum
			{
				result = result_type::value
			};
		};

		template<typename T>
		class IsBuiltIn<T *>
		{
		public:

			enum
			{
				result = true
			};
		};

		typedef boost::mpl::set<
			unsigned char,
			unsigned short int,
			unsigned int,
			unsigned long int,
			signed char,
			short int,
			int,
			long int,
			bool,
			char,
			wchar_t
		> integral_types;

		template<typename T>
		class IsIntegral
		{
			typedef typename boost::mpl::has_key<built_in_types, T>::type result_type;

		public:

			enum
			{
				result = result_type::value
			};
		};

		typedef boost::mpl::set<
			float,
			double,
			long double
		> floating_point_types;

		template<typename T>
		class IsFloatingPoint
		{
			typedef typename boost::mpl::has_key<floating_point_types, T>::type result_type;

		public:

			enum
			{
				result = result_type::value
			};
		};
	}

	/**
	 * Provides compile-time type information.
	 *
	 * The implementation below is not intended as a comprehensive type traits
	 * class. It contains what is sufficient for our current needs. In particular,
	 * incorrect results may be returned for implementation-specific types.
	 */
	template<typename T>
	struct TypeTraits
	{
		enum
		{
			/**
			 * True if the type is a built-in type.
			 */
			is_built_in = TypeTraitsInternals::IsBuiltIn<T>::result
		};

		enum
		{
			/**
			 * True if the type is a built-in integral type.
			 */
			is_integral = TypeTraitsInternals::IsIntegral<T>::result
		};

		enum
		{
			/**
			 * True if the type is a built-in floating-point type.
			 */
			is_floating_point = TypeTraitsInternals::IsFloatingPoint<T>::result
		};

		/**
		 * Use argument_type to pick a good type for arguments to functions.
		 * If T is a built-in type, argument_type is T.
		 * If T is not a built-in type, argument_type is const T &.
		 */
		typedef typename Select<is_built_in, T, const T &>::result argument_type;
	};
}

#endif  /* GPLATES_UTILS_TYPETRAITS_H */
