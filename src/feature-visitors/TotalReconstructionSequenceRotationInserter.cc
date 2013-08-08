/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <boost/none.hpp>  // boost::none
#include <boost/ref.hpp>
#include <loki/ScopeGuard.h>
#include <QDebug>

#include "TotalReconstructionSequencePlateIdFinder.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "file-io/PlatesRotationFileProxy.h"

#include "TotalReconstructionSequenceRotationInserter.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "model/ModelUtils.h"

#include "presentation/Application.h"

#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTotalReconstructionPole.h"

/*
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
*/

GPlatesFeatureVisitors::TotalReconstructionSequenceRotationInserter::TotalReconstructionSequenceRotationInserter(
		const double &recon_time,
		const GPlatesMaths::Rotation &rotation_to_apply,
		const QString &comment):
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_rotation_to_apply(rotation_to_apply),
	d_is_expecting_a_finite_rotation(false),
	d_trp_time_matches_exactly(false),
	d_comment(comment),
	d_grot_proxy(NULL),
	d_moving_plate_id(0),
	d_fixed_plate_id(0)
{  }


bool
GPlatesFeatureVisitors::TotalReconstructionSequenceRotationInserter::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	using namespace GPlatesAppLogic;
	using namespace GPlatesModel;
	using namespace GPlatesFileIO;
	d_is_expecting_a_finite_rotation = false;
	d_trp_time_matches_exactly = false;
	d_finite_rotation = boost::none;

	TotalReconstructionSequencePlateIdFinder id_finder;
	id_finder.visit_feature(feature_handle.reference());
	boost::optional<GPlatesModel::integer_plate_id_type> 
		moving_plate_id = id_finder.moving_ref_frame_plate_id(),
		fixed_plate_id = id_finder.fixed_ref_frame_plate_id();
	if(moving_plate_id)
	{
		d_moving_plate_id = *moving_plate_id;

	}
	if(fixed_plate_id)
	{
		d_fixed_plate_id = *fixed_plate_id;
	}

	FeatureCollectionFileState &file_state = 
		GPlatesPresentation::Application::instance().get_application_state().get_feature_collection_file_state();
	std::vector<FeatureCollectionFileState::file_reference> loaded_files =
		file_state.get_loaded_files();

	std::vector<FeatureCollectionFileState::file_reference>::iterator 
		iter = loaded_files.begin(),
		end = loaded_files.end();
	for ( ; iter != end; ++iter) 
	{
		GPlatesModel::FeatureCollectionHandle::weak_ref fc = iter->get_file().get_feature_collection();
		GPlatesModel::FeatureCollectionHandle::iterator 
			fc_it = fc->begin(),
			fc_end = fc->end();
		for(; fc_it != fc_end; fc_it++)
		{
			if((*fc_it).get() == &feature_handle)
			{
				const boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> cfg = 
					iter->get_file().get_file_configuration();
				if(cfg)
				{
					boost::shared_ptr<const FeatureCollectionFileFormat::RotationFileConfiguration> rotation_cfg_const =
						boost::dynamic_pointer_cast<const FeatureCollectionFileFormat::RotationFileConfiguration>(*cfg);
					boost::shared_ptr<FeatureCollectionFileFormat::RotationFileConfiguration> rotation_cfg =
						boost::const_pointer_cast< FeatureCollectionFileFormat::RotationFileConfiguration>(rotation_cfg_const);
					if(rotation_cfg)
					{
						d_grot_proxy = &rotation_cfg->get_rotation_file_proxy();
						return true;
					}

				}
			}
		}
	}
	return true;
}


void
GPlatesFeatureVisitors::TotalReconstructionSequenceRotationInserter::visit_gpml_finite_rotation(
		GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation)
{
    qDebug() << "Visiting finite rotation";
	if (d_is_expecting_a_finite_rotation) {
		// The visitor was expecting a FiniteRotation, which means the structure of the
		// Total Reconstruction Sequence is (more or less) correct.
		if (d_trp_time_matches_exactly) {
			// The time of the total reconstruction pole (TRP) matches exactly, so
			// we'll update the finite rotation in place, right now.
			GPlatesMaths::FiniteRotation updated_finite_rotation =
					GPlatesMaths::compose(d_rotation_to_apply,
							gpml_finite_rotation.get_finite_rotation());
			gpml_finite_rotation.set_finite_rotation(updated_finite_rotation);
			GPlatesFileIO::RotationPoleData 
				new_pole(
						updated_finite_rotation,
						d_moving_plate_id,
						d_fixed_plate_id,
						d_recon_time.value()),
				old_pole(
						gpml_finite_rotation.get_finite_rotation(),
						d_moving_plate_id,
						d_fixed_plate_id,
						d_recon_time.value());
			if(d_grot_proxy)
			{
				d_grot_proxy->update_pole(old_pole, new_pole);
			}
		} else {
			// The finite rotation needs to be interpolated and a new time-sample needs
			// to be inserted.  That means this function will be called twice by
			// 'visit_gpml_irregular_sampling', first to obtain the finite rotation in
			// the time-sample immediately *before* the desired time, and then to
			// obtain the finite rotation in the time-sample immediately *after* the
			// desired time.
			//
			// Hence, we'll just fetch the finite rotation now, and the interpolation
			// and insertion will happen back in 'visit_gpml_irregular_sampling'.
			d_finite_rotation = gpml_finite_rotation.get_finite_rotation();
		}
		d_is_expecting_a_finite_rotation = false;
	} else {
		// FIXME:  Should we complain?
	}
}


void
GPlatesFeatureVisitors::TotalReconstructionSequenceRotationInserter::visit_gpml_finite_rotation_slerp(
		GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
{
	// FIXME:  We should use this for something... (Currently, FiniteRotation SLERP is the only
	// option, so the code below is hard-coded to perform a FiniteRotation SLERP.  But still,
	// we should do this properly.)
}


void
GPlatesFeatureVisitors::TotalReconstructionSequenceRotationInserter::visit_gpml_irregular_sampling(
		GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	using namespace GPlatesPropertyValues;
	using namespace GPlatesModel;

	// It is assumed that an IrregularSampling instance which has been reached by the visit
	// function of a TotalReconstructionSequenceRotationInserter instance will only ever contain
	// FiniteRotation instances.
	// FIXME:  Should we check this? (by looking at the 'value_type' value of the IrrSampling).

	// It is assumed that an IrregularSampling must contain at least one time sample, however,
	// that time sample might be disabled.

	// First, deal with times in the future.
	GeoTimeInstant present_day(0.0);
	if (d_recon_time.is_strictly_later_than(present_day)) {
		// FIXME:  Not yet implemented.
		return;
	}

	// A copy of the current time samples to work with.
	std::vector<GpmlTimeSample> time_samples = gpml_irregular_sampling.get_time_samples();
	// This needs to be set back onto the irregular sampling property when/if we're done making
	// modifications - we do this automatically at scope exit (to cover all the 'return' paths).
	Loki::ScopeGuard gpml_irregular_sampling_set_time_samples_guard = Loki::MakeGuard(
			&GPlatesPropertyValues::GpmlIrregularSampling::set_time_samples,
			gpml_irregular_sampling,
			boost::cref(time_samples));
	gpml_irregular_sampling_set_time_samples_guard.silence_unused_variable_warning();

	// Otherwise, the reconstruction time is either the present-day, or in the past.
	// First, let's see whether the reconstruction time matches the time of the most-recent
	// (non-disabled) time sample.

	// So, let's get to the most-recent non-disabled time sample.
	std::vector<GpmlTimeSample>::iterator iter = time_samples.begin();
	std::vector<GpmlTimeSample>::iterator end = time_samples.end();
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

	if (d_recon_time.is_strictly_later_than(iter->get_valid_time()->get_time_position())) {
		// The requested reconstruction time is later than the time of the most-recent
		// non-disabled time sample.  Hence, it is not valid to reconstruct to the
		// requested reconstruction time.
		// FIXME:  Should we complain about this?
		return;
	}
	if (d_recon_time.is_coincident_with((iter->get_valid_time()->get_time_position()))) {
		// An exact match!  Hence, we can use the FiniteRotation of this time sample
		// directly, without need for interpolation.

		// Let's visit the time sample, to update (what we expect to be) the
		// FiniteRotation inside it.
		d_is_expecting_a_finite_rotation = true;
		d_trp_time_matches_exactly = true;
		iter->get_value()->accept_visitor(*this);

		// And update the comment field.
		boost::intrusive_ptr<XsString> description =
				XsString::create(GPlatesUtils::make_icu_string_from_qstring(d_comment)).get();
		iter->set_description(description);

		// Did the visitor successfully collect the FiniteRotation?
		if ( ! d_finite_rotation) {
			// No, it didn't.
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

		if (d_recon_time.is_strictly_later_than(iter->get_valid_time()->get_time_position())) {
			// The requested reconstruction time is later than (ie, less far in the
			// past than) the time of the current time sample, which must mean that it
			// lies "on the rail" between the current time sample and the time sample
			// before it in the sequence.
			//
			// The current time sample will be more temporally-distant than the
			// previous time sample.

			boost::optional<GPlatesMaths::FiniteRotation> current_finite_rotation;
			boost::optional<GPlatesMaths::FiniteRotation> previous_finite_rotation;

			// Let's visit the time sample, to collect (what we expect to be) the
			// FiniteRotation inside it.
			d_is_expecting_a_finite_rotation = true;
			iter->get_value()->accept_visitor(*this);

			// Did the visitor successfully collect the FiniteRotation?
			if ( ! d_finite_rotation) {
				// No, it didn't.
				// FIXME:  Should we complain?
				return;
			} else {
				// Success!
				current_finite_rotation = d_finite_rotation;
			}

			// Now let's visit the _previous_ non-disabled time sample, to collect
			// (what we expect to be) the FiniteRotation inside it.
			d_is_expecting_a_finite_rotation = true;
			prev->get_value()->accept_visitor(*this);

			// Did the visitor successfully collect the FiniteRotation?
			if ( ! d_finite_rotation) {
				// No, it didn't.
				// FIXME:  Should we complain?
				return;
			} else {
				// Success!
				previous_finite_rotation = d_finite_rotation;
			}

			GPlatesMaths::real_t current_time =
					iter->get_valid_time()->get_time_position().value();
			GPlatesMaths::real_t previous_time =
					prev->get_valid_time()->get_time_position().value();
			GPlatesMaths::real_t target_time =
					d_recon_time.value();

			// If either of the finite rotations has an axis hint, use it.
			boost::optional<GPlatesMaths::UnitVector3D> axis_hint;
			if (previous_finite_rotation->axis_hint()) {
				axis_hint = previous_finite_rotation->axis_hint();
			} else if (current_finite_rotation->axis_hint()) {
				axis_hint = current_finite_rotation->axis_hint();
			}
			GPlatesMaths::FiniteRotation interpolated_finite_rotation =
					GPlatesMaths::interpolate(
						*previous_finite_rotation, *current_finite_rotation,
						previous_time, current_time, target_time,
						axis_hint);

			// Now we have the updated (interpolated) finite rotation, which is to be
			// inserted back into the irregular sampling, in a new time-sample.
			GPlatesMaths::FiniteRotation updated_finite_rotation =
					GPlatesMaths::compose(d_rotation_to_apply, interpolated_finite_rotation);

			// Create the new time-sample.
			boost::optional<PropertyValue::non_null_ptr_type> value_opt;
			if(dynamic_cast<GpmlTotalReconstructionPole*>(iter->get_value().get()))
			{
				//if the rotation feature is from a .grot file, 
				//we need to create GpmlTotalReconstructionPole instead of GpmlFiniteRotation.
				value_opt = GpmlTotalReconstructionPole::create(updated_finite_rotation);
			}
			else
			{
				value_opt = GpmlFiniteRotation::create(updated_finite_rotation);
			}
			PropertyValue::non_null_ptr_type value = *value_opt;
			GmlTimeInstant::non_null_ptr_type valid_time =
					ModelUtils::create_gml_time_instant(d_recon_time);
			boost::intrusive_ptr<XsString> description =
					XsString::create(GPlatesUtils::make_icu_string_from_qstring(d_comment)).get();
			StructuralType value_type =
					StructuralType::create_gpml("FiniteRotation");
			GpmlTimeSample new_time_sample(value, valid_time, description, value_type);

			// Now insert the time-sample at the appropriate position.
			time_samples.insert(iter, new_time_sample);
			GPlatesFileIO::RotationPoleData data(
					updated_finite_rotation,
					d_moving_plate_id,
					d_fixed_plate_id,
					d_recon_time.value());
			if(d_grot_proxy)
			{
				d_grot_proxy->insert_pole(data);
			}
			// OK, our task is complete.  Hence, we're going to exit from this loop by
			// returning out of this function.  Hence, we won't be iterating through
			// the vector of time-samples any more.  Hence, there's no need to correct
			// the iterators.
			return;
		}
		if (d_recon_time.is_coincident_with(iter->get_valid_time()->get_time_position())) {
			// An exact match!  Hence, we can use the FiniteRotation of this time
			// sample directly, without need for interpolation.

			// Let's visit the time sample, to update (what we expect to be) the
			// FiniteRotation inside it.
			d_is_expecting_a_finite_rotation = true;
			d_trp_time_matches_exactly = true;
			iter->get_value()->accept_visitor(*this);

                        // Update the comment field too.
                        boost::intrusive_ptr<XsString> description =
                                        XsString::create(GPlatesUtils::make_icu_string_from_qstring(d_comment)).get();
                        iter->set_description(description);

			// Did the visitor successfully collect the FiniteRotation?
			if ( ! d_finite_rotation) {
				// No, it didn't.
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

