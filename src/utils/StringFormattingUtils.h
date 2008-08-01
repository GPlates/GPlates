/* $Id$ */

/**
 * \file 
 * A collection of functions to aid in the formatting of strings.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_STRINGFORMATTINGUTILS_H
#define GPLATES_UTILS_STRINGFORMATTINGUTILS_H

#include <string>
#include "maths/Real.h"
#include "global/GPlatesException.h"


namespace GPlatesUtils
{
	class InvalidFormattingParametersException:
			public GPlatesGlobal::Exception
	{
		public:
			InvalidFormattingParametersException(
					const std::string &message):
				d_message(message)
			{ }

		protected:

			virtual
			const char *
			ExceptionName() const
			{ 
				return "InvalidFormattingParametersException";
			}

			virtual 
			std::string
			Message() const 
			{
				return d_message; 
			}

		private:

			std::string d_message;
	};


	/*
	 * Print a real number in a space of 'width' characters, right-justified,
	 * with exactly 'prec' digits to the right of the decimal place.  The
	 * following numbers (between the exclamation marks) give a few examples
	 * when (width == 9) && (prec == 4) (as is the case for printing
	 * latitudes and longitudes in the PLATES4 format):
	 *
	 *   ! -31.4159!
	 *   !  27.1828!
	 *   !   1.6180!
	 */
	const std::string
	formatted_double_to_string(
			const double &val, 
			unsigned width,
			unsigned prec,
			bool elide_trailing_zeroes = false);


	const std::string
	formatted_int_to_string(
			int val,
			unsigned width,
			char fill_char = ' ');
}

#endif // GPLATES_UTILS_STRINGFORMATTINGUTILS_H
