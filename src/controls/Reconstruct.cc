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
 * Primary Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 *
 * Corrections, Additions:
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include <list>
#include <map>
#include <utility>  /* std::pair */
#include <algorithm>  /* std::find */
#include <wx/timer.h>

#include "Reconstruct.h"
#include "Dialogs.h"
#include "AnimationTimer.h"
#include "global/types.h"
#include "state/Data.h"
#include "state/Layout.h"
#include "controls/GuiCalls.h"
#include "maths/FiniteRotation.h"


using namespace GPlatesGlobal;
using namespace GPlatesState;
using namespace GPlatesMaths;


typedef std::map< rid_t, FiniteRotation > RotationsByPlate;


enum status { SUCCESSFUL, CANNOT_BE_ROTATED };

/**
 * FIXME: this should be placed somewhere "more official".
 */
static const rid_t RIDOfGlobe = rid_t(0);


/**
 * A convenient alias for the operation of searching through a list
 * for a particular element.
 */
template< typename T >
inline bool
ListContainsElem(const std::list< T > &l, const T &e) {

	return (std::find(l.begin(), l.end(), e) != l.end());
}


/**
 * Check whether the plate described by @a plate_id can be rotated to time @a t.
 *
 * If a plate can be rotated, the function will return 'SUCCESSFUL'.
 * Otherwise, it will return 'CANNOT_BE_ROTATED'.
 *
 * Additionally, when it is verified that the plate can be rotated to time @a t,
 * the finite rotation to perform this rotation is calculated and stored in
 * the map @a rot_cache.
 * 
 * This function operates recursively.  This is due to the fact that a plate
 * depends on all the plates in the rotation hierarchy between itself and the
 * globe.  Thus, in checking a plate X, all plates between X and the globe
 * will also be checked.
 *
 * The operation of this function is sped up by caching negative results in
 * the list @a cannot_be_rotated.  The map @a rot_cache serves as a cache of
 * positive results.  This caching ensures that it is not necessary to
 * traverse the hierarchy all the way to the globe every single time.
 */
static enum status
CheckRotation(rid_t plate_id, real_t t,
	RotationsByPlate &rot_cache, std::list< rid_t > &cannot_be_rotated,
	const Data::RotationMap_type &histories) {

	/*
	 * Firstly, check whether this plate can be rotated to time 't'.
	 *
	 * See if it can be found in the collection of rotation histories
	 * for each plate (a map: plate id -> rotation history for that plate).
	 * If there is no match for the given plate id, this plate has no
	 * rotation history, and hence there is no way it can be rotated.
	 *
	 * If there IS a match, query the rotation history as to whether
	 * it is defined at time 't'.  If not, there is no rotation sequence
	 * defined at 't', and hence there is no way the plate can be
	 * rotated to a position at 't'.
	 */
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
	 * at time 't', which means there must exist a finite rotation for 't'.
	 *
	 * Now we need to check upwards in the rotation hierarchy, to ensure
	 * that those plates are also rotatable to time 't'.
	 */
	RotationHistory::const_iterator rot_seq = i->second.atTime(t);
	rid_t fixed_plate_id = (*rot_seq).fixedPlate();

	// Base case of recursion: if this plate is moving relative to globe.
	if (fixed_plate_id == RIDOfGlobe) {

		/*
		 * The fixed plate is the globe, which is always defined,
		 * so the rotation of our plate is defined.
		 */
		FiniteRotation rot = (*rot_seq).finiteRotationAtTime(t);
		rot_cache.insert(std::make_pair(plate_id, rot));
		return SUCCESSFUL;
	}

	/*
	 * Now, check in caches to see if the fixed plate has already been
	 * dealt with.
	 */
	RotationsByPlate::const_iterator j = rot_cache.find(fixed_plate_id);
	if (j != rot_cache.end()) {

		/*
		 * The fixed plate has already been dealt with, and is known
		 * to be rotatable, so the rotation of our plate is defined.
		 */
		FiniteRotation rot = (*rot_seq).finiteRotationAtTime(t);
		rot_cache.insert(std::make_pair(plate_id, j->second * rot));
		return SUCCESSFUL;
	}
	if (ListContainsElem(cannot_be_rotated, fixed_plate_id)) {

		/*
		 * The fixed plate (or some plate in the hierarchy above it)
		 * is known not to be rotatable.
		 */
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

		/*
		 * The fixed plate (or some plate in the hierarchy above it)
		 * has been determined to be not rotatable.
		 */
		cannot_be_rotated.push_back(plate_id);
		return CANNOT_BE_ROTATED;
	}
	// else, the fixed plate has been determined to be rotatable

	/*
	 * Since the fixed plate is rotatable, it must be in the rotation cache,
	 * so this search should not fail.
	 */
	RotationsByPlate::const_iterator k = rot_cache.find(fixed_plate_id);
	// FIXME: check that (k != rot_cache.end()), and if so, throw an
	// exception for an internal error.

	FiniteRotation rot = (*rot_seq).finiteRotationAtTime(t);
	rot_cache.insert(std::make_pair(plate_id, k->second * rot));
	return SUCCESSFUL;
}


/**
 * Given @a plates_to_draw (the collection of all plates to attempt to draw),
 * populate @a rot_cache (the collection of all plates which can be drawn)
 * with the finite rotations which will rotate the plates to their positions
 * at time @a t.
 *
 * This function is a non-recursive "wrapper" around the recursive function
 * @a CheckRotation.
 */
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

		// Check whether this plate can be rotated to time 't'
		CheckRotation(plate_id, t, rot_cache, cannot_be_rotated,
		 *histories);
	}
}


/**
 * Warp the geological data to its position at time @a t.
 *
 * This function assumes it has been invoked by a function such as
 * 'GPlatesControls::Reconstruct::Time', which will verify the 
 * validity of the data returned by 'Data::GetDataGroup'.
 */
static void
WarpToTime(const fpdata_t &t) {

	/*
	 * Assume that if Data::GetDataGroup() is not NULL, then
	 * Data::GetDrawableData() is also not NULL.
	 */
	Data::DrawableMap_type *drawable_data = Data::GetDrawableData();
	Layout::Clear();

	/*
	 * From the collection of drawable data, generate the collection of
	 * rotatable data.
	 */
	RotationsByPlate rotatable_data;
	PopulateRotatableData(drawable_data, rotatable_data, real_t(t));

	// for each drawable plate...
	for (Data::DrawableMap_type::const_iterator i = drawable_data->begin();
	     i != drawable_data->end();
	     i++) {

		// the rotation id of this plate
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
	GPlatesControls::GuiCalls::SetCurrentTime(t);
	GPlatesControls::GuiCalls::RepaintCanvas();
}


// A hack to deal with a GCC bug and a bad XFree86 header file...
namespace GPlatesControls {

/**
 * Perform a reconstruction animation for the time @a time.
 *
 * This function corresponds directly to the GUI menu item of
 * 'Reconstruct -> Jump to Time'.
 */
void
Reconstruct::Time(const fpdata_t &time)
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
		std::exit(1);
	}
}

}


/**
 * Warp the geological data to its position in the present-day
 * (ie, at time 0.0 Ma).
 *
 * This function assumes it has been invoked by a function such as
 * 'GPlatesControls::Reconstruct::Present', which will verify the 
 * validity of the data returned by 'Data::GetDataGroup'.
 */
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
	GPlatesControls::GuiCalls::SetCurrentTime(0.0);
	GPlatesControls::GuiCalls::RepaintCanvas();
}


/**
 * Perform a reconstruction for the present.
 *
 * This function corresponds directly to the GUI menu item of
 * 'Reconstruct -> Return to Present'.
 */
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


/**
 * Perform a reconstruction animation between the two times @a start_time
 * and @a end_time.
 *
 * This function corresponds directly to the GUI menu item of
 * 'Reconstruct -> Animation'.
 */
void
GPlatesControls::Reconstruct::Animation(const fpdata_t &start_time,
 const fpdata_t &end_time, const fpdata_t& time_delta, bool finish_on_end) {

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

	// FIXME: Don't forget to check return value!
	AnimationTimer::StartNew(WarpToTime, start_time, end_time, time_delta,
	 finish_on_end, 500);  // 500 ms => 2 updates per second
}
