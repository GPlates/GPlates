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
 * FIXME: this value was just a guess. Discover what this value should be.
 * [The guess was based upon several pages found on the net; first and
 * foremost http://www.ma.utexas.edu/documentation/lapack/node73.html ]
 */
#define FPDATA_EPSILON (1.2e-16)

using namespace GPlatesGlobal;

const double FPData::Epsilon = FPDATA_EPSILON;
const double FPData::NegativeEpsilon = -FPDATA_EPSILON;
