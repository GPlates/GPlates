/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#include <cstdlib>
#include <iostream>
#include <limits>
#include <QDebug>

#include "MathsUtils.h"


namespace
{
	template<typename T>
	bool
	type_has_infinity_and_nan()
	{
		return std::numeric_limits<T>::has_infinity &&
			std::numeric_limits<T>::has_quiet_NaN &&
			std::numeric_limits<T>::has_signaling_NaN;
	}
}


bool
GPlatesMaths::has_infinity_and_nan()
{
	return type_has_infinity_and_nan<float>() &&
		type_has_infinity_and_nan<double>();
}



void
GPlatesMaths::assert_has_infinity_and_nan()
{
	if (!has_infinity_and_nan())
	{
		qWarning() << "Float and double types must have infinity, quiet NaN and signaling NaN.";
		exit(1);
	}
}

