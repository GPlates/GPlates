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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include "Reconstruct.h"
#include "Dialogs.h"
#include "state/Data.h"


using namespace GPlatesControls;


void
Reconstruct::Time(const GPlatesMaths::real_t&)
{
	/*
	 * NOTE: remember that this can be optimised if the time is 0.0:
	 * still need to check which data is defined at 0.0, but no need
	 * to rotate (since GPML and PLATES data are defined by their
	 * position at time 0.0).  If the time is 0.0, then a rotation
	 * file is also not strictly necessary.
	 */

	if (GPlatesState::Data::GetDataGroup() == NULL) {

		Dialogs::ErrorMessage("No data to reconstruct",
		 "Cannot perform a reconstruction, since there is no"
		 " data loaded.",
		 "Cannot perform reconstruction.");

		return;
	}
	if (GPlatesState::Data::GetRotationHistories() == NULL) {

		Dialogs::ErrorMessage("No rotation data",
		 "Cannot perform a reconstruction, since there is no"
		 " rotation data loaded.",
		 "Cannot perform reconstruction.");

		return;
	}
}


void
Reconstruct::Present() {

	if (GPlatesState::Data::GetDataGroup() == NULL) {

		Dialogs::ErrorMessage("No data to reconstruct",
		 "Cannot perform a reconstruction, since there is no"
		 " data loaded.",
		 "Cannot perform reconstruction.");

		return;
	}
	// Do not need rotation data

	// Reconstruct to a time of 0.0
}


void
Reconstruct::Animation(const GPlatesMaths::real_t &start_time,
	const GPlatesMaths::real_t &end_time,
	const GPlatesGlobal::integer_t &nsteps)
{
	if (GPlatesState::Data::GetDataGroup() == NULL) {

		Dialogs::ErrorMessage("No data to reconstruct",
		 "Cannot perform a reconstruction, since there is no"
		 " data loaded.",
		 "Cannot perform reconstruction.");

		return;
	}
	if (GPlatesState::Data::GetRotationHistories() == NULL) {

		Dialogs::ErrorMessage("No rotation data",
		 "Cannot perform a reconstruction, since there is no"
		 " rotation data loaded.",
		 "Cannot perform reconstruction.");

		return;
	}
}
