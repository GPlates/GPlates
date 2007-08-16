/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include "ReconstructionTreePopulator.h"
#include "ReconstructionTree.h"
#include "FeatureHandle.h"
#include "GpmlFiniteRotation.h"
#include "GpmlFiniteRotationSlerp.h"
#include "GpmlIrregularSampling.h"
#include "GpmlTimeSample.h"
#include "InlinePropertyContainer.h"


namespace
{
	// This useful little function is used to obtain the iterator before the supplied iterator.
	// To understand why it is necessary, read Josuttis99, section 7.2.6 "The Increment and
	// Decrement Problem of Vector Iterators".
	template<typename T>
	inline
	const T
	prev_before(
			T t)
	{
		return --t;
	}
}


GPlatesModel::ReconstructionTreePopulator::ReconstructionTreePopulator(
		const double &recon_time,
		ReconstructionTree &recon_tree):
	d_recon_time(GeoTimeInstant(recon_time)),
	d_recon_tree_ptr(&recon_tree)
{  }


void
GPlatesModel::ReconstructionTreePopulator::visit_feature_handle(
		FeatureHandle &feature_handle)
{
	static FeatureType total_recon_seq_feature_type =
			FeatureType(UnicodeString("gpml:TotalReconstructionSequence"));

	// If a feature handle isn't for a feature of type "gpml:TotalReconstructionSequence",
	// we're not interested.
	if (feature_handle.feature_type() == total_recon_seq_feature_type) {
		d_accumulator.reset(new ReconstructionSequenceAccumulator());

		// Now visit each of the properties in turn.
		GPlatesModel::FeatureHandle::properties_iterator iter =
				feature_handle.properties_begin();
		GPlatesModel::FeatureHandle::properties_iterator end =
				feature_handle.properties_end();
		for ( ; iter != end; ++iter) {
			// Elements of this properties vector can be NULL pointers.  (See the
			// comment in "model/FeatureRevision.h" for more details.)
			if (*iter != NULL) {
				d_accumulator->d_most_recent_propname_read.reset(
						new PropertyName((*iter)->property_name()));
				(*iter)->accept_visitor(*this);
			}
		}

		// So now we've visited the contents of this Total Recon Seq feature.  Let's find
		// out if we were able to obtain all the information we need.
		if (d_accumulator->d_fixed_ref_frame == NULL) {
			// We couldn't obtain the fixed ref-frame.
			d_accumulator.reset(NULL);
			return;
		}
		if (d_accumulator->d_moving_ref_frame == NULL) {
			// We couldn't obtain the moving ref-frame.
			d_accumulator.reset(NULL);
			return;
		}
		if (d_accumulator->d_finite_rotation.get() == NULL) {
			// We couldn't obtain the finite rotation.
			d_accumulator.reset(NULL);
			return;
		}

		// If we got to here, we have all the information we need.
		d_recon_tree_ptr->insert_total_reconstruction_pole(
				GpmlPlateId::non_null_ptr_type(*d_accumulator->d_fixed_ref_frame),
				GpmlPlateId::non_null_ptr_type(*d_accumulator->d_moving_ref_frame),
				*(d_accumulator->d_finite_rotation));

		d_accumulator.reset(NULL);
	}
}


void
GPlatesModel::ReconstructionTreePopulator::visit_gpml_finite_rotation(
		GpmlFiniteRotation &gpml_finite_rotation) {
	if (d_accumulator->d_is_expecting_a_finite_rotation) {
		// The visitor was expecting a FiniteRotation, which means the structure of the
		// Total Recon Seq is (more or less) correct.
		d_accumulator->d_finite_rotation.reset(
				new GPlatesMaths::FiniteRotation(gpml_finite_rotation.finite_rotation()));
		d_accumulator->d_is_expecting_a_finite_rotation = false;
	} else {
		// FIXME:  Should we complain?
	}
}


void
GPlatesModel::ReconstructionTreePopulator::visit_gpml_finite_rotation_slerp(
		GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
{
	// FIXME:  We should use this for something... (Currently, FiniteRotation SLERP is the only
	// option, so the code below is hard-coded to perform a FiniteRotation SLERP.  But still,
	// we should do this properly.)
}


void
GPlatesModel::ReconstructionTreePopulator::visit_gpml_irregular_sampling(
		GpmlIrregularSampling &gpml_irregular_sampling)
{
	// It is assumed that an IrregularSampling instance which has been reached by the visit
	// function of a ReconstructionTreePopulator instance will only ever contain
	// FiniteRotation instances.
	// FIXME:  Should we check this? (by looking at the 'value_type' value of the IrrSampling).

	// It is assumed that an IrregularSampling must contain at least one time sample, however,
	// that time sample might be disabled.

	// First, deal with times in the future.
	GeoTimeInstant present_day(0.0);
	if (d_recon_time.is_later_than(present_day)) {
		// FIXME:  Not yet implemented.
		return;
	}

	// Otherwise, the reconstruction time is either the present-day, or in the past.
	// First, let's see whether the reconstruction time matches the time of the most-recent
	// (non-disabled) time sample.

	// So, let's get to the most-recent non-disabled time sample.
	std::vector<GpmlTimeSample>::iterator iter = gpml_irregular_sampling.time_samples().begin();
	std::vector<GpmlTimeSample>::iterator end = gpml_irregular_sampling.time_samples().end();
	while (iter != end && iter->is_disabled()) {
		// This time-sample is disabled.  Let's move to the next one.
		++iter;
	}
	if (iter == end) {
		// There were no non-disabled time samples.  We can't do anything with this
		// irregular sampling.
		// FIXME:  Should we complain about this?
		return;
	}
	// else:  'iter' points to the most-recent non-disabled time sample.

	if (d_recon_time.is_later_than(iter->valid_time()->time_position())) {
		// The requested reconstruction time is later than the time of the most-recent
		// non-disabled time sample.  Hence, it is not valid to reconstruct to the
		// requested reconstruction time.
		// FIXME:  Should we complain about this?
		return;
	}
	// FIXME:  Use approximate floating-point equality for the time position.  Extract the code
	// 'geo_time_instants_are_approx_equal' from "fileio/PlatesRotationFormatReader.cc".
	if ( ! d_recon_time.is_earlier_than(iter->valid_time()->time_position())) {
		// An exact match!  Hence, we can use the FiniteRotation of this time sample
		// directly, without need for interpolation.

		// Let's visit the time sample, to collect (what we expect to be) the
		// FiniteRotation inside it.
		d_accumulator->d_is_expecting_a_finite_rotation = true;
		iter->accept_visitor(*this);

		// Did the visitor successfully collect the FiniteRotation?
		if (d_accumulator->d_finite_rotation.get() != NULL) {
			// Success!
		} else {
			// FIXME:  Should we complain?
		}
		return;
	}

	// Imagine this Total Recon Seq as a sequence of fence-posts with horizontal rails between
	// them: |--|--|--|
	// (Great asky-art, huh?)
	//
	// Each fence-post is a FiniteRotation; each rail is the interpolation between adjacent
	// FiniteRotations in the sequence.  The first (left-most) post corresponds to the
	// most-recent FiniteRotation; the last (right-most) post corresponds to the most-distant
	// FiniteRotation (furthest in the past).
	//
	// We want to determine whether the point corresponding to the requested reconstruction
	// time sits on this fence or not.  We've already looked at the first fence-post:  We now
	// know that the reconstruction time is somewhere to the right of (further in the past
	// than) this first fence-post.  Now we will compare the reconstruction time with the
	// remaining rails and posts.

	// 'prev' is the previous non-disabled time sample.
	std::vector<GpmlTimeSample>::iterator prev = iter;
	for (++iter; iter != end; ++iter) {
		if (iter->is_disabled()) {
			// This time-sample is disabled.  Let's move to the next one.
			continue;
		}
		// else:  'iter' points to the most-recent non-disabled time sample.

		if (d_recon_time.is_later_than(iter->valid_time()->time_position())) {
			// The requested reconstruction time is later than (ie, less far in the
			// past than) the time of the current time sample, which must mean that it
			// lies "on the rail" between the current time sample and the time sample
			// before it in the sequence.
			//
			// The current time sample will be more temporally-distant than the
			// previous time sample.

			std::auto_ptr<GPlatesMaths::FiniteRotation> current_finite_rotation;
			std::auto_ptr<GPlatesMaths::FiniteRotation> previous_finite_rotation;

			// Let's visit the time sample, to collect (what we expect to be) the
			// FiniteRotation inside it.
			d_accumulator->d_is_expecting_a_finite_rotation = true;
			iter->accept_visitor(*this);

			// Did the visitor successfully collect the FiniteRotation?
			if (d_accumulator->d_finite_rotation.get() != NULL) {
				// Success!
				current_finite_rotation = d_accumulator->d_finite_rotation;
			} else {
				// FIXME:  Should we complain?
				return;
			}

			// Now let's visit the _previous_ non-disabled time sample, to collect
			// (what we expect to be) the FiniteRotation inside it.
			d_accumulator->d_is_expecting_a_finite_rotation = true;
			prev->accept_visitor(*this);

			// Did the visitor successfully collect the FiniteRotation?
			if (d_accumulator->d_finite_rotation.get() != NULL) {
				// Success!
				previous_finite_rotation = d_accumulator->d_finite_rotation;
			} else {
				// FIXME:  Should we complain?
				return;
			}

			GPlatesMaths::real_t current_time =
					iter->valid_time()->time_position().value();
			GPlatesMaths::real_t previous_time =
					prev->valid_time()->time_position().value();
			GPlatesMaths::real_t target_time =
					d_recon_time.value();

			d_accumulator->d_finite_rotation.reset(
					new GPlatesMaths::FiniteRotation(
						GPlatesMaths::interpolate(
							*previous_finite_rotation, *current_finite_rotation,
							previous_time, current_time, target_time)));

			return;
		}
		// FIXME:  Use approximate floating-point equality for the time position.  Extract
		// the code 'geo_time_instants_are_approx_equal' from
		// "fileio/PlatesRotationFormatReader.cc".
		if ( ! d_recon_time.is_earlier_than(iter->valid_time()->time_position())) {
			// An exact match!  Hence, we can use the FiniteRotation of this time
			// sample directly, without need for interpolation.

			// Let's visit the time sample, to collect (what we expect to be) the
			// FiniteRotation inside it.
			d_accumulator->d_is_expecting_a_finite_rotation = true;
			iter->accept_visitor(*this);

			// Did the visitor successfully collect the FiniteRotation?
			if (d_accumulator->d_finite_rotation.get() != NULL) {
				// Success!
			} else {
				// FIXME:  Should we complain?
			}
			return;
		}

		// Note that this assignment is not made in ALL circumstances (which is why it
		// isn't happening inside the increment-expression of the for-loop):  It is only
		// made when the time sample pointed-to by 'iter' was non-disabled.
		prev = iter;
	}
	// FIXME:  We've passed the last fence-post, and not yet reached the requested recon time. 
	// Should we complain?
}


void
GPlatesModel::ReconstructionTreePopulator::visit_gpml_plate_id(
		GpmlPlateId &gpml_plate_id)
{
	static PropertyName property_name_fixed_ref_frame =
			PropertyName(UnicodeString("gpml:fixedReferenceFrame"));
	static PropertyName property_name_moving_ref_frame =
			PropertyName(UnicodeString("gpml:movingReferenceFrame"));

	// FIXME:  Should we ensure that 'd_accumulator->d_most_recent_propname_read' is not NULL?
	if (*(d_accumulator->d_most_recent_propname_read) == property_name_fixed_ref_frame) {
		// We're dealing with the fixed ref-frame of the Total Recon Seq.
		boost::intrusive_ptr<GpmlPlateId> gpml_plate_id_ptr = &gpml_plate_id;
		d_accumulator->d_fixed_ref_frame = gpml_plate_id_ptr;
	} else if (*(d_accumulator->d_most_recent_propname_read) == property_name_moving_ref_frame) {
		// We're dealing with the moving ref-frame of the Total Recon Seq.
		boost::intrusive_ptr<GpmlPlateId> gpml_plate_id_ptr = &gpml_plate_id;
		d_accumulator->d_moving_ref_frame = gpml_plate_id_ptr;
	}
}


void
GPlatesModel::ReconstructionTreePopulator::visit_gpml_time_sample(
		GpmlTimeSample &gpml_time_sample) {
	gpml_time_sample.value()->accept_visitor(*this);
}


void
GPlatesModel::ReconstructionTreePopulator::visit_inline_property_container(
		InlinePropertyContainer &inline_property_container) {
	InlinePropertyContainer::const_iterator iter = inline_property_container.begin(); 
	InlinePropertyContainer::const_iterator end = inline_property_container.end(); 
	for ( ; iter != end; ++iter) {
		(*iter)->accept_visitor(*this);
	}
}
