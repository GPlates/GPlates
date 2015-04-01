/* $Id$ */

/**
 * @file 
 * Contains the definition of the Select template class.
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

#ifndef GPLATES_UTILS_SAFEBOOL_H
#define GPLATES_UTILS_SAFEBOOL_H

#include <stdlib.h>
#include <boost/static_assert.hpp>


namespace GPlatesUtils
{
	/**
	 * SafeBool is a reuseable solution to the safe bool idiom.
	 *
	 * See http://www.artima.com/cppsource/safebool.html for a description of the
	 * safe bool idiom.
	 *
	 * SafeBool provides a safe version of operator bool() the result of which
	 * cannot be accidentally used in arithmetic operations. To use SafeBool,
	 * publicly inherit from SafeBool and add a public, non-virtual function:
	 *
	 *		bool
	 *		boolean_test() const
	 *		{
	 *			// operator bool() logic goes here.
	 *		}
	 */
	template<class T>
	class SafeBool
	{
	protected:

		typedef void (SafeBool::*bool_type)() const;

		void this_type_does_not_support_comparisons() const
		{  }

	public:

		operator bool_type() const
		{
			return (static_cast<const T*>(this))->boolean_test() ?
				&SafeBool::this_type_does_not_support_comparisons : NULL;
		}

	protected:

		SafeBool()
		{  }

		SafeBool(const SafeBool &)
		{  }

		SafeBool&
		operator=(
				const SafeBool &)
		{
			return *this;
		}

		~SafeBool()
		{  }
	};


	// Disallow operator== on SafeBools.
	template<class T, class U>
	bool
	operator==(
			const SafeBool<T> &lhs,
			const SafeBool<U> &rhs)
	{
		// Make sure compiler does not instantiate this function.
		//
		// Evaluates to false - but is dependent on template parameter - compiler workaround...
		BOOST_STATIC_ASSERT(sizeof(T) == 0);

		return false;
	}

	// Disallow operator!= on SafeBools.
	template<class T, class U>
	bool
	operator!=(
			const SafeBool<T> &lhs,
			const SafeBool<U> &rhs)
	{
		// Make sure compiler does not instantiate this function.
		//
		// Evaluates to false - but is dependent on template parameter - compiler workaround...
		BOOST_STATIC_ASSERT(sizeof(T) == 0);

		return false;
	}
}

#endif  // GPLATES_UTILS_SAFEBOOL_H
