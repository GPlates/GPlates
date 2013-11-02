/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <cmath>

#include "FastMaths.h"


namespace GPlatesMaths
{
	namespace
	{
		const double ASIN4_PARAMS1[5] =
		{
			6.32559537178112e-05,
			9.97002719101181e-01,
			3.23729856176963e-02,
			3.89287300071597e-02,
			1.93549238398372e-01
		};

		const double ASIN4_PARAMS2[7] =
		{
			2.09625797161885e+01,
			-1.74835553411477e+02,
			6.13575281494908e+02,
			-1.14033116228467e+03,
			1.19159992307311e+03,
			-6.63957441058529e+02,
			1.54421991537526e+02
		};

		const double ASIN4_PARAMS3[4] =
		{
			1.57080010233116e+00,
			-1.41437401362252e+00,
			1.84777752400778e-03,
			-1.24625163381900e-01
		};

		const double ASIN4_SPLIT1 = 0.6;

		const double ASIN4_SPLIT2 = 0.925;
	}
}


double
GPlatesMaths::FastMaths::asin(
		double sine)
{
	//
	// This algorithm was obtained from a post by BabyUniverse at
	// http://devmaster.net/forums/topic/4648-fast-and-accurate-sinecosine/page__st__60
	//

	// Convert range -1 <= sine <= 1 to range 0 <= sine <= 1
	// and then apply the sign before returning since:
	//   asin(-x) = -asin(x)
	double sgn;
	if (sine < 0)
	{
		sgn = -1;
		sine = -sine;
	}
	else
	{
		sgn = 1;
	}

	if (sine < ASIN4_SPLIT1)
	{
		return sgn * (
				ASIN4_PARAMS1[0] + sine * (
					ASIN4_PARAMS1[1] + sine * (
						ASIN4_PARAMS1[2] + sine * (
							ASIN4_PARAMS1[3] + sine * (
								ASIN4_PARAMS1[4])))));
	}

	if (sine < ASIN4_SPLIT2)
	{
		return sgn * (
				ASIN4_PARAMS2[0] + sine * (
					ASIN4_PARAMS2[1] + sine * (
						ASIN4_PARAMS2[2] + sine * (
							ASIN4_PARAMS2[3] + sine * (
								ASIN4_PARAMS2[4] + sine * (
									ASIN4_PARAMS2[5] + sine * (
										ASIN4_PARAMS2[6])))))));
	}

	// Protect 'sqrt' function from negative argument.
	if (sine >= 1)
	{
		return sgn * HALF_PI;
	}

	const double sqrt_one_minus_sine = std::sqrt(1 - sine);

	return sgn * (
			ASIN4_PARAMS3[0] + sqrt_one_minus_sine * (
				ASIN4_PARAMS3[1] + sqrt_one_minus_sine * (
					ASIN4_PARAMS3[2] + sqrt_one_minus_sine * (
						ASIN4_PARAMS3[3]))));
}
