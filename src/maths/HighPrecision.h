/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _GPLATES_MATHS_HIGHPRECISION_H_
#define _GPLATES_MATHS_HIGHPRECISION_H_

#include <ostream>
#include "Real.h"


namespace GPlatesMaths
{
	/**
	 * This class is used to enable high-precision output of Real numbers.
	 *
	 * Since it is a template class, it can be used to cause a temporary
	 * "burst" of high-precision output of any variable.  Simply place
	 * 'HighPrecision( )' around the variable which is being inserted
	 * into a stream.  For example:
	 * - std::cout << val << std::endl;
	 * would become:
	 * - std::cout << HighPrecision(val) << std::endl;
	 */
	template< typename T >
	class HighPrecision
	{
		public:
			typedef T output_type;

			explicit
			HighPrecision(const output_type &val) : _val(val) {  }

			void writeTo(std::ostream &os) const;

		private:
			output_type _val;
	};


	template< typename T >
	void
	HighPrecision< T >::writeTo(std::ostream &os) const {

		std::streamsize high_prec = Real::High_Precision;
		std::streamsize orig_prec = os.precision(high_prec);
		os << _val;
		os.precision(orig_prec);
	}


	template< typename T >
	inline std::ostream &
	operator<<(std::ostream &os, const HighPrecision< T > &hp) {

		hp.writeTo(os);
		return os;
	}
}

#endif  // _GPLATES_MATHS_HIGHPRECISION_H_
