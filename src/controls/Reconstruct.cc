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

#include <list>
#include <map>
#include <utility>  /* std::pair */
#include <algorithm>  /* std::find */

#include "Reconstruct.h"
#include "Dialogs.h"
#include "global/types.h"
#include "state/Data.h"
#include "state/Layout.h"
#include "controls/View.h"
#include "maths/FiniteRotation.h"


using namespace GPlatesGlobal;
using namespace GPlatesState;
//using namespace GPlatesControls;
using namespace GPlatesMaths;


typedef std::map< rid_t, FiniteRotation > RotationsByPlate;


enum status { SUCCESSFUL, CANNOT_BE_ROTATED };

static const rid_t RIDOfGlobe = 0;


template< typename T >
inline bool
ListContainsElem(const std::list< T > &l, const T &e) {

	return (std::find(l.begin(), l.end(), e) != l.end());
}


enum status
CheckRotation(rid_t plate_id, real_t t,
	RotationsByPlate &rot_cache, std::list< rid_t > &cannot_be_rotated,
	const Data::RotationMap_type &histories) {

	// Firstly, check whether this plate can be rotated
	Data::RotationMap_type::const_iterator i = histories.find(plate_id);
	if (i == histories.end()) {

		// this plate has no rotation history, so cannot rotate it
		cannot_be_rotated.push_back(plate_id);
		return CANNOT_BE_ROTATED;
	}
	if ( ! i->second.isDefinedAtTime(t)) {

		// this plate's rotation history is not defined at this time
		cannot_be_rotated.push_back(plate_id);
		return CANNOT_BE_ROTATED;
	}

	/*
	 * Otherwise, we know that this plate has a rotation history defined
	 * at time 't', which means there must exist a finite rotation for
	 * time 't'.
	 *
	 * Now we need to check upwards in the rotation hierarchy, to ensure
	 * that those plates are also rotatable to time 't'.
	 */
	RotationHistory::const_iterator info = i->second.atTime(t);
	rid_t fixed_plate_id = (*info).fixedPlate();

	if (fixed_plate_id == RIDOfGlobe) {

		/* the fixed plate is the globe, which is always defined,
		 * so the rotation of our plate is defined */
		FiniteRotation rot = (*info).finiteRotationAtTime(t);
		rot_cache.insert(std::make_pair(plate_id, rot));
		return SUCCESSFUL;
	}

	/*
	 * Now, check in caches to see if the fixed plate has already been
	 * dealt with.
	 */
	RotationsByPlate::const_iterator j = rot_cache.find(fixed_plate_id);
	if (j != rot_cache.end()) {

		/* the fixed plate has already been dealt with, and is known
		 * to be rotatable, so the rotation of our plate is defined */
		FiniteRotation rot = (*info).finiteRotationAtTime(t);
		rot_cache.insert(std::make_pair(plate_id, j->second * rot));
		return SUCCESSFUL;
	}
	if (ListContainsElem(cannot_be_rotated, fixed_plate_id)) {

		/* the fixed plate (or some plate in the hierarchy above it)
		 * is known not to be rotatable */
		cannot_be_rotated.push_back(plate_id);
		return CANNOT_BE_ROTATED;
	}

	/*
	 * There is no cached result, so we must query this the hard way
	 * (recursively).
	 */
	enum status query_status = CheckRotation(fixed_plate_id, t, rot_cache,
	 cannot_be_rotated, histories);
	if (query_status == CANNOT_BE_ROTATED) {

		/* the fixed plate (or some plate in the hierarchy above it)
		 * has been determined to be not rotatable */
		cannot_be_rotated.push_back(plate_id);
		return CANNOT_BE_ROTATED;
	}
	// else, the fixed plate has been determined to be rotatable

	/*
	 * Since the fixed plate is rotatable, it must be in the rotation cache,
	 * so this search should not fail.
	 */
	RotationsByPlate::const_iterator k = rot_cache.find(fixed_plate_id);

	FiniteRotation rot = (*info).finiteRotationAtTime(t);
	rot_cache.insert(std::make_pair(plate_id, k->second * rot));
	return SUCCESSFUL;
}


static void
PopulateRotatableData(const Data::DrawableMap_type *plates_to_draw,
	RotationsByPlate &rot_cache, const real_t &t) {

	std::list< rid_t > cannot_be_rotated;

	// assume this is non-NULL
	const Data::RotationMap_type *histories = Data::GetRotationHistories();

	// for each plate to draw...
	for (Data::DrawableMap_type::const_iterator i = plates_to_draw->begin();
	     i != plates_to_draw->end();
	     i++) {

		// get the plate id
		rid_t plate_id = i->first;

		CheckRotation(plate_id, t, rot_cache, cannot_be_rotated,
		 *histories);
	}
}


static void
WarpToTime(const fpdata_t &t) {

	/*
	 * Assume that if Data::GetDataGroup() is not NULL, then
	 * Data::GetDrawableData() is also not NULL.
	 */
	Data::DrawableMap_type *drawable_data = Data::GetDrawableData();
	Layout::Clear();

	RotationsByPlate rotatable_data;
	PopulateRotatableData(drawable_data, rotatable_data, real_t(t));

	// for each drawable plate...
	for (Data::DrawableMap_type::const_iterator i = drawable_data->begin();
	     i != drawable_data->end();
	     i++) {

		rid_t plate_id = i->first;

		RotationsByPlate::const_iterator k =
		 rotatable_data.find(plate_id);
		if (k == rotatable_data.end()) {

			// the current plate is not rotatable to time 't'
			continue;
		}
		FiniteRotation rot = k->second;  // the rotation for this plate

		// get the drawable data items which move with this plate
		const Data::DrawableDataSet &items = i->second;

		// then for each item...
		for (Data::DrawableDataSet::const_iterator j = items.begin();
		     j != items.end();
		     j++) {

			// if that item exists at time 't'
			if ((*j)->ExistsAtTime(t)) {

				// rotate it and draw it
				(*j)->RotateAndDraw(rot);
			}
		}
	}
	GPlatesControls::View::Redisplay();
}

namespace GPlatesControls {
void
GPlatesControls::Reconstruct::Time(const fpdata_t &time)
{
	if (Data::GetDataGroup() == NULL) {

		Dialogs::ErrorMessage("No data to reconstruct",
		 "Cannot perform a reconstruction, since there is no"
		 " data loaded.",
		 "Cannot perform reconstruction.");

		return;
	}
	if (Data::GetRotationHistories() == NULL) {

		Dialogs::ErrorMessage("No rotation data",
		 "Cannot perform a reconstruction, since there is no"
		 " rotation data loaded.",
		 "Cannot perform reconstruction.");

		return;
	}

	try {
		WarpToTime(time);

	} catch (const GPlatesGlobal::Exception &e) {

		std::cerr << "Internal exception: " << e << std::endl;
		exit(1);
	}
}
}

static void
WarpToPresent() {

	/*
	 * Assume that if Data::GetDataGroup() is not NULL, then
	 * Data::GetDrawableData() is also not NULL.
	 */
	Data::DrawableMap_type *drawable_data = Data::GetDrawableData();
	Layout::Clear();

	// for each drawable plate...
	for (Data::DrawableMap_type::const_iterator i = drawable_data->begin();
	     i != drawable_data->end();
	     i++) {

		// get the drawable data items which move with this plate
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
GPlatesControls::Reconstruct::Present() {

	if (Data::GetDataGroup() == NULL) {

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
GPlatesControls::Reconstruct::Animation(const fpdata_t &start_time,
	const fpdata_t &end_time,
	const integer_t &nsteps)
{
	if (Data::GetDataGroup() == NULL) {

		Dialogs::ErrorMessage("No data to reconstruct",
		 "Cannot perform a reconstruction, since there is no"
		 " data loaded.",
		 "Cannot perform reconstruction.");

		return;
	}
	if (Data::GetRotationHistories() == NULL) {

		Dialogs::ErrorMessage("No rotation data",
		 "Cannot perform a reconstruction, since there is no"
		 " rotation data loaded.",
		 "Cannot perform reconstruction.");

		return;
	}

	// FIXME: we really should COMPLAIN if 'nsteps' is < 2
	if (nsteps < 2) return;
	try {

		real_t t(start_time);  // first time of the animation
		real_t time_incr = (real_t(end_time) - t) / (nsteps - 1);

		unsigned num_frames = static_cast< unsigned >(nsteps);

		for (unsigned n = 0;
		     n < num_frames;
		     n++, t += time_incr) {

			// display the frame for time 't'
			WarpToTime(t.dval());
		}

	} catch (const GPlatesGlobal::Exception &e) {

		std::cerr << "Internal exception: " << e << std::endl;
		exit(1);
	}
}
