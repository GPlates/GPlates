/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include "FPData.h"

/*
 * FIXME: the value below was just a guess. Discover what this value should be.
 *
 * According to:
 *  http://www.cs.berkeley.edu/~demmel/cs267/lecture21/lecture21.html
 * and
 *  http://www.ma.utexas.edu/documentation/lapack/node73.html
 * the machine epsilon for an IEEE 754-compliant machine is about 1.2e-16.
 *
 * According to these documents, the machine epsilon (aka "macheps") is
 * half the distance between 1 and the next largest fp value.
 *
 * I wish to allow for rounding errors due to the limits of floating-point
 * precision, but _unlike_ the "Real" type (found in ../maths/Real.{h,cc}),
 * I don't want to allow for accumulation of rounding errors.  The "FPData"
 * type is only supposed to be used for arithmetic comparisons, not other
 * arithmetic operations (such as addition, subtraction, multiplcation, etc.).
 */
#define FPDATA_EPSILON (1.2e-16)

using namespace GPlatesGlobal;

const double FPData::Epsilon = FPDATA_EPSILON;
const double FPData::NegativeEpsilon = -FPDATA_EPSILON;
