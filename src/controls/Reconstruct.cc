/* $Id$ */

/**
 * @file 
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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#include "Reconstruct.h"
#include "Dialogs.h"

using namespace GPlatesControls;

void
Reconstruct::Time(const GPlatesMaths::real_t&)
{
	Dialogs::ErrorMessage(
		"Construction not implemented.",
		
		"The functionality you requested (construction) is "
		"not yet implemented.",

		"No construction could be made.");
}

void
Reconstruct::Animation(const GPlatesMaths::real_t&, 
					   const GPlatesMaths::real_t&,
					   const GPlatesGlobal::integer_t&)
{
	Dialogs::ErrorMessage(
		"Animation not implemented.",
		
		"The functionality you requested (animation) is "
		"not yet implemented.",

		"No animation could be constructed.");
		
}
