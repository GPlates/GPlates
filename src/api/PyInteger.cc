/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "PythonConverterUtils.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


void
export_integer()
{
	//
	// Note: We don't need to register to-from python converters for integers because boost-python
	// takes care of that for us.
	// However we do need to register converters for boost::optional<int>, etc, so that python's
	// "None" (ie, Py_None) can be used as a function argument for example.
	//

	GPlatesApi::PythonConverterUtils::register_optional_conversion<char>();
	GPlatesApi::PythonConverterUtils::register_optional_conversion<unsigned char>();

	GPlatesApi::PythonConverterUtils::register_optional_conversion<short>();
	GPlatesApi::PythonConverterUtils::register_optional_conversion<unsigned short>();

	GPlatesApi::PythonConverterUtils::register_optional_conversion<int>();
	GPlatesApi::PythonConverterUtils::register_optional_conversion<unsigned int>();

	GPlatesApi::PythonConverterUtils::register_optional_conversion<long>();
	GPlatesApi::PythonConverterUtils::register_optional_conversion<unsigned long>();

	// TODO: Perhaps might also need 64-bit integers (when supported) such as boost::uint64_t.
}

#endif // GPLATES_NO_PYTHON
