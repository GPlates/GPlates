/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#include <iostream>
#include <algorithm>  // std::find
#include <boost/none.hpp>

#include "TotalReconstructionSequenceTimePeriodFinder.h"

#include "model/TopLevelPropertyInline.h"
#include "property-values/GpmlIrregularSampling.h"


GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder::TotalReconstructionSequenceTimePeriodFinder(
		bool skip_over_disabled_samples):
	d_skip_over_disabled_samples(skip_over_disabled_samples)
{
	d_property_names_to_allow.push_back(
			GPlatesModel::PropertyName::create_gpml("totalReconstructionPole"));
}


namespace
{
	template<typename C, typename E>
	bool
	contains_elem(
			const C &container,
			const E &elem)
	{
		return (std::find(container.begin(), container.end(), elem) != container.end());
	}
}


bool
GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder::initialise_pre_property_values(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	const GPlatesModel::PropertyName &curr_prop_name = top_level_property_inline.get_property_name();
	if ( ! d_property_names_to_allow.empty()) {
		// We're not allowing all property names.
		if ( ! contains_elem(d_property_names_to_allow, curr_prop_name)) {
			// The current property name is not allowed.
			return false;
		}
	}
	return true;
}


void
GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	using namespace GPlatesPropertyValues;

	GPlatesModel::RevisionedVector<GpmlTimeSample>::const_iterator iter =
			gpml_irregular_sampling.time_samples().begin();
	GPlatesModel::RevisionedVector<GpmlTimeSample>::const_iterator end =
			gpml_irregular_sampling.time_samples().end();
	for ( ; iter != end; ++iter) {
		// First, skip over any disabled time samples (if the client code wants us to).
		if (d_skip_over_disabled_samples) {
			if (iter->get()->is_disabled()) {
				continue;
			}
		}
		const GeoTimeInstant &gti = iter->get()->valid_time()->get_time_position();
		if ( ! gti.is_real()) {
			// This geo-time-instant seems to be in either the distant past or the
			// distant future.  This should not be the case in an irregular sampling.
			// FIXME: Should we complain?
			std::cerr << "Current time sample (at "
					<< gti
					<< " Ma) should have a real value in an irregular sampling."
					<< std::endl;

			// For now, let's just skip this time sample.
			continue;
		}

		// We expect that the time samples in an irregular sampling should be ordered so
		// that the first is the most recent (ie, least far in the past), and subsequent
		// samples should be strictly earlier (ie, further in the past).

		if ( ! d_begin_time) {
			// We haven't yet set the begin-time.  So, let's set it to this current
			// geo-time-instant, and we can compare subsequent geo-time-instants with
			// this one.
			d_begin_time = gti;
		} else if (gti.is_strictly_earlier_than(*d_begin_time)) {
			// The begin-time has been set in a previous iteration, and the current
			// geo-time-instant is earlier than it.  This is as expected, since each
			// successive time sample should be further in the past (ie, earlier).
			d_begin_time = gti;
		} else if (iter->get()->is_disabled()) {
			// This time sample is disabled, so we won't bother comparing its time
			// position with that of the preceding time sample.  (This if-condition
			// basically prevents us from proceeding into the following "else" block.)
			// We want to allow this because in rotation files, adjacent disabled time
			// samples often have the same time position.
		} else {
			// The begin-time has been set in a previous iteration, and the current
			// geo-time-instant is not earlier than it.  This is strange, because the
			// first time sample in an irregular sampling should be the most recent;
			// subsequent samples should be strictly earlier.
			// FIXME: What should we do about this?
			std::cerr << "Current time sample (at "
					<< gti
					<< " Ma) is not earlier than the time sample (at "
					<< *d_begin_time
					<< " Ma) which preceded it in the sampling sequence."
					<< std::endl;
		}

		if ( ! d_end_time) {
			// We haven't yet set the end-time.  So, let's set it to this current
			// geo-time-instant, and we can compare subsequent geo-time-instants with
			// this one.
			d_end_time = gti;
		}
	}
}


void
GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder::reset()
{
	d_begin_time = boost::none;
	d_end_time = boost::none;
}
