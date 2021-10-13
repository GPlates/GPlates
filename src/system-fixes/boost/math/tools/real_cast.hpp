// The contents of the file after the #else was copied from Boost 1.35.
// If the user's version of Boost has real_cast.hpp, it is used; otherwise this
// version from Boost 1.35 is used.
// This file is in system-fixes because boost/math/special_functions/fpclassify.hpp
// which is also in system-fixes includes this file, which is not in Boost 1.34.
#include "global/config.h"

#ifdef BOOST_MATH_TOOLS_REAL_CAST_HPP_PATH
#	include BOOST_MATH_TOOLS_REAL_CAST_HPP_PATH
#else // BOOST_MATH_TOOLS_REAL_CAST_HPP_PATH

//  Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_REAL_CAST_HPP
#define BOOST_MATH_TOOLS_REAL_CAST_HPP

namespace boost{ namespace math
{
  namespace tools
  {
    template <class To, class T>
    inline To real_cast(T t)
    {
       return static_cast<To>(t);
    }
  } // namespace tools
} // namespace math
} // namespace boost

#endif // BOOST_MATH_TOOLS_REAL_CAST_HPP

#endif // BOOST_MATH_TOOLS_REAL_CAST_HPP_PATH

