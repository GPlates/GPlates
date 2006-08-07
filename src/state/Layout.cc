/* $Id$ */

/**
 * @file 
 * This is not a file!!! WTG?
 *
 * Most recent change:
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
 */

#include "Layout.h"


namespace {

	using namespace GPlatesState;


	/**
	 * Used by @a GPlatesState::Layout::find_close_data.
	 */
	void
	find_close_point_data(
	 std::priority_queue< Layout::CloseDatum > &sorted_results,
	 const GPlatesMaths::PointOnSphere &test_point,
	 const GPlatesMaths::real_t &closeness_inclusion_threshold) {

		Layout::PointDataLayout::iterator
		 iter = Layout::PointDataLayoutBegin(),
		 end = Layout::PointDataLayoutEnd();

		for ( ; iter != end; ++iter) {

			GPlatesMaths::PointOnSphere &the_point =
			 iter->second;
			GPlatesGeo::DrawableData *the_datum =
			 iter->first;

			// Don't bother initialising this.
			GPlatesMaths::real_t closeness;

			if (the_point.is_close_to(test_point,
			     closeness_inclusion_threshold, closeness)) {

				sorted_results.push(
				 Layout::CloseDatum(the_datum,
				 Layout::POINT_DATUM, closeness));
			}
		}
	}


	/**
	 * Used by @a GPlatesState::Layout::find_close_data.
	 */
	void
	find_close_line_data(
	 std::priority_queue< Layout::CloseDatum > &sorted_results,
	 const GPlatesMaths::PointOnSphere &test_point,
	 const GPlatesMaths::real_t &closeness_inclusion_threshold) {

		const GPlatesMaths::real_t cit_sqrd =
		 closeness_inclusion_threshold * closeness_inclusion_threshold;
		const GPlatesMaths::real_t latitude_exclusion_threshold =
		 sqrt(1.0 - cit_sqrd);

		Layout::LineDataLayout::iterator
		 iter = Layout::LineDataLayoutBegin(),
		 end = Layout::LineDataLayoutEnd();

		for ( ; iter != end; ++iter) {

			GPlatesMaths::PolyLineOnSphere &the_polyline =
			 iter->second;
			GPlatesGeo::DrawableData *the_datum =
			 iter->first;

			// Don't bother initialising this.
			GPlatesMaths::real_t closeness;

			if (the_polyline.is_close_to(test_point,
			     closeness_inclusion_threshold,
			     latitude_exclusion_threshold, closeness)) {

				sorted_results.push(
				 Layout::CloseDatum(the_datum,
				 Layout::LINE_DATUM, closeness));
			}
		}
	}

}


void
GPlatesState::Layout::find_close_data(
 std::priority_queue< CloseDatum > &sorted_results,
 const GPlatesMaths::PointOnSphere &test_point,
 const GPlatesMaths::real_t &closeness_inclusion_threshold) {

	find_close_point_data(sorted_results, test_point,
	 closeness_inclusion_threshold);
	find_close_line_data(sorted_results, test_point,
	 closeness_inclusion_threshold);
}


GPlatesState::Layout::PointDataLayout *
GPlatesState::Layout::_point_data_layout = NULL;

GPlatesState::Layout::LineDataLayout *
GPlatesState::Layout::_line_data_layout = NULL;
