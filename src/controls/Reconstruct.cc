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
#include "state/Layout.h"
#include "controls/View.h"


using namespace GPlatesControls;


void
Reconstruct::Time(const GPlatesMaths::real_t &time)
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


static void
WarpToPresent() {

	using namespace GPlatesState;

	/*
	 * Assume that if Data::GetDataGroup() is not NULL, then
	 * Data::GetDrawableData() is also not NULL.
	 */
	Data::DrawableMap_type *drawable_data = Data::GetDrawableData();
	Layout::Clear();

	// for each plate...
	for (Data::DrawableMap_type::const_iterator i = drawable_data->begin();
	     i != drawable_data->end();
	     i++) {

		// get the drawable data items which move with that plate
		const Data::DrawableDataSet &items = i->second;

		// then for each item...
		for (Data::DrawableDataSet::const_iterator j = items.begin();
		     j != items.end();
		     j++) {

			// if that item exists in the present...
			if ((*j)->ExistsAtTime(0.0)) {

				// draw it
				(*j)->Draw();
			}
		}
	}
	GPlatesControls::View::Redisplay();
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

	WarpToPresent();
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
