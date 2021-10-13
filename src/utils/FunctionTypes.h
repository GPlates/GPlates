/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_UTILS_FUNCTIONTYPES_H
#define GPLATES_UTILS_FUNCTIONTYPES_H

#include <boost/mpl/vector.hpp>


namespace GPlatesUtils
{
	/**
	 * This namespace mimics the functionality provided by Boost.FunctionTypes,
	 * which is currently unavailable because it was introduced in Boost 1.35 (as
	 * of writing, the minimum Boost version is 1.34). The key word is *mimics* -
	 * only what is necessary has been implemented!
	 *
	 * In theory, once we can use Boost 1.35, Boost.FunctionTypes can be used as a
	 * replacement with the only modifications to client code being the change of
	 * namespace in which the structs reside.
	 *
	 * Note that we follow the convention in Boost.FunctionTypes that the hidden
	 * 'this' pointer of member function pointers counts towards the arity.
	 */
	namespace FunctionTypes
	{
		/**
		 * This mimics the struct @a component_types in Boost.FunctionTypes. Quoting
		 * from the Boost documentation:
		 *
		 *     Extracts all properties of a callable builtin type, that is the result
		 *     type, followed by the parameter types (including the type of this for
		 *     member function pointers).
		 *
		 * Note that volatile functions and functions with variable arguments are not
		 * supported by this implementation.
		 */
		template<typename FunctionType>
		struct component_types;

		// Arity = 0.
		
		template<
			typename Result
		>
		struct component_types< Result (*)() >
		{
			typedef boost::mpl::vector<Result> types;
		};

		// Arity = 1.

		template<
			typename Result,
			typename A0
		>
		struct component_types< Result (*)(A0) >
		{
			typedef boost::mpl::vector<Result, A0> types;
		};

		template<
			typename Result,
			typename Class
		>
		struct component_types< Result (Class::*)() >
		{
			typedef boost::mpl::vector<Result, Class> types;
		};

		template<
			typename Result,
			typename Class
		>
		struct component_types< Result (Class::*)() const >
		{
			typedef boost::mpl::vector<Result, Class> types;
		};

		// Arity = 2.

		template<
			typename Result,
			typename A0,
			typename A1
		>
		struct component_types< Result (*)(A0, A1) >
		{
			typedef boost::mpl::vector<Result, A0, A1> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1
		>
		struct component_types< Result (Class::*)(A1) >
		{
			typedef boost::mpl::vector<Result, Class, A1> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1
		>
		struct component_types< Result (Class::*)(A1) const >
		{
			typedef boost::mpl::vector<Result, Class, A1> types;
		};

		// Arity = 3.

		template<
			typename Result,
			typename A0,
			typename A1,
			typename A2
		>
		struct component_types< Result (*)(A0, A1, A2) >
		{
			typedef boost::mpl::vector<Result, A0, A1, A2> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2
		>
		struct component_types< Result (Class::*)(A1, A2) >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2
		>
		struct component_types< Result (Class::*)(A1, A2) const >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2> types;
		};

		// Arity = 4.

		template<
			typename Result,
			typename A0,
			typename A1,
			typename A2,
			typename A3
		>
		struct component_types< Result (*)(A0, A1, A2, A3) >
		{
			typedef boost::mpl::vector<Result, A0, A1, A2, A3> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3
		>
		struct component_types< Result (Class::*)(A1, A2, A3) >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3
		>
		struct component_types< Result (Class::*)(A1, A2, A3) const >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3> types;
		};

		// Arity = 5.

		template<
			typename Result,
			typename A0,
			typename A1,
			typename A2,
			typename A3,
			typename A4
		>
		struct component_types< Result (*)(A0, A1, A2, A3, A4) >
		{
			typedef boost::mpl::vector<Result, A0, A1, A2, A3, A4> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4) >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4) const >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4> types;
		};

		// Arity = 6.

		template<
			typename Result,
			typename A0,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5
		>
		struct component_types< Result (*)(A0, A1, A2, A3, A4, A5) >
		{
			typedef boost::mpl::vector<Result, A0, A1, A2, A3, A4, A5> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5) >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5) const >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5> types;
		};

		// Arity = 7.

		template<
			typename Result,
			typename A0,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6
		>
		struct component_types< Result (*)(A0, A1, A2, A3, A4, A5, A6) >
		{
			typedef boost::mpl::vector<Result, A0, A1, A2, A3, A4, A5, A6> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5, A6) >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5, A6> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5, A6) const >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5, A6> types;
		};

		// Arity = 8.

		template<
			typename Result,
			typename A0,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6,
			typename A7
		>
		struct component_types< Result (*)(A0, A1, A2, A3, A4, A5, A6, A7) >
		{
			typedef boost::mpl::vector<Result, A0, A1, A2, A3, A4, A5, A6, A7> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6,
			typename A7
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5, A6, A7) >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5, A6, A7> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6,
			typename A7
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5, A6, A7) const >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5, A6, A7> types;
		};

		// Arity = 9.

		template<
			typename Result,
			typename A0,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6,
			typename A7,
			typename A8
		>
		struct component_types< Result (*)(A0, A1, A2, A3, A4, A5, A6, A7, A8) >
		{
			typedef boost::mpl::vector<Result, A0, A1, A2, A3, A4, A5, A6, A7, A8> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6,
			typename A7,
			typename A8
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5, A6, A7, A8) >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5, A6, A7, A8> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6,
			typename A7,
			typename A8
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5, A6, A7, A8) const >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5, A6, A7, A8> types;
		};

		// Arity = 10.

		template<
			typename Result,
			typename A0,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6,
			typename A7,
			typename A8,
			typename A9
		>
		struct component_types< Result (*)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9) >
		{
			typedef boost::mpl::vector<Result, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6,
			typename A7,
			typename A8,
			typename A9
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5, A6, A7, A8, A9) >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5, A6, A7, A8, A9> types;
		};

		template<
			typename Result,
			typename Class,
			typename A1,
			typename A2,
			typename A3,
			typename A4,
			typename A5,
			typename A6,
			typename A7,
			typename A8,
			typename A9
		>
		struct component_types< Result (Class::*)(A1, A2, A3, A4, A5, A6, A7, A8, A9) const >
		{
			typedef boost::mpl::vector<Result, Class, A1, A2, A3, A4, A5, A6, A7, A8, A9> types;
		};


		/**
		 * This mimics the struct @a function_arity in Boost.FunctionTypes. Quoting
		 * from the Boost documentation:
		 *
		 *     Extracts the function arity, that is the number of parameters.
		 */
		template<typename FunctionType>
		struct function_arity
		{
			static const std::size_t value = boost::mpl::size< typename component_types<FunctionType>::types >::value - 1;
		};
	};
}

#endif // GPLATES_UTILS_FUNCTIONTYPES_H
