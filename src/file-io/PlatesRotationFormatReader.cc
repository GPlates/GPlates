/* $Id$ */

/**
 * @file
 * Contains the definitions of the member functions of the class PlatesRotationFormatReader.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 James Boyden <jboy@jboy.id.au>
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
 * Copyright (C) 2007 Geological Survey of Norway
 *
 * This file is derived from the source file "PlatesRotationFormatReader.cc",
 * which is part of the ReconTreeViewer software:
 *  http://sourceforge.net/projects/recontreeviewer/
 *
 * ReconTreeViewer is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2, as published
 * by the Free Software Foundation.
 *
 * ReconTreeViewer is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sstream>
#include <string>
#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <loki/ScopeGuard.h>
#include <QFile>

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "PlatesRotationFormatReader.h"
#include "PlatesRotationFileProxy.h"
#include "LineReader.h"

#include "app-logic/RotationUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "model/ChangesetHandle.h"
#include "model/Model.h"
#include "model/ModelUtils.h"

#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/StructuralType.h"

#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"


namespace
{
	// FIXME:  Should this use some member function of GeoTimeInstant?
	bool
	geo_time_instants_are_approx_equal(
			const GPlatesPropertyValues::GeoTimeInstant &t1,
			const GPlatesPropertyValues::GeoTimeInstant &t2)
	{
		if (( ! t1.is_real()) || ( ! t2.is_real())) {
			// One or both time-instants are in the distant past or distant future; in
			// such a case, comparisons for equality are meaningless.
			return false;
		}
		return GPlatesMaths::are_geo_times_approximately_equal(t1.value(), t2.value());
	}


	// FIXME:  Should this be some sort of utility function in GPlatesModel::ModelUtils?
	inline
	bool
	gml_time_instants_are_approx_equal(
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_to_const_type t1,
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_to_const_type t2)
	{
		return geo_time_instants_are_approx_equal(t1->time_position(), t2->time_position());
	}


	/**
	 * From the remainder of an input line from a PLATES rotation-format file, strip any
	 * leading whitespace, then extract the comment, which is supposed to commence with an
	 * exclamation mark ('!').
	 */
	void
	extract_comment(
			std::istringstream &iss,
			std::string &comment,
			boost::shared_ptr<GPlatesFileIO::DataSource> data_source,
			unsigned line_num,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		using namespace GPlatesFileIO;

		std::string remainder;

		// This function is provided by the standard string classes (see Josuttis,  11.2.10
		// and 11.3.10).  It reads all characters until the line delimiter or end-of-file
		// is reached.
		std::getline(iss, remainder);
		std::string::size_type index_of_first_non_whitespace =
				remainder.find_first_not_of(" \t");
		if (index_of_first_non_whitespace == std::string::npos) {
			// No non-whitespace characters were found in the remainder, not even an
			// exclamation mark.  Let's handle the problem by creating an empty comment
			// for the user.
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::NoCommentFound;
			ReadErrors::Result res = ReadErrors::EmptyCommentCreated;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_warnings.push_back(read_error);
		} else {
			// Non-whitespace characters were found.  We'll assume these are intended
			// to be the start of the comment.  Is the first character an exclamation
			// mark?
			if (remainder[index_of_first_non_whitespace] != '!') {
				// No, it's not an exclamation mark.  Let's handle the problem by
				// pretending that the first character *was* an exclamation mark.
				boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
				ReadErrors::Description descr = ReadErrors::NoExclMarkToStartComment;
				ReadErrors::Result res = ReadErrors::ExclMarkInsertedAtCommentStart;
				ReadErrorOccurrence read_error(data_source, location, descr, res);
				read_errors.d_warnings.push_back(read_error);

				comment = remainder.substr(index_of_first_non_whitespace);
			} else {
				comment = remainder.substr(index_of_first_non_whitespace + 1);
			}
		}
	}


	// FIXME:  Give this a better name (and do the exception properly).
	struct PoleParsingException {  };


	/**
	 * Parse a single total reconstruction pole from a line of a PLATES rotation-format file.
	 *
	 * If parsing is unsuccessful, a PoleParsingException will be thrown.
	 */
	GPlatesPropertyValues::GpmlTimeSample
	parse_pole(
			std::istringstream &iss,
			GPlatesModel::integer_plate_id_type &fixed_plate_id,
			GPlatesModel::integer_plate_id_type &moving_plate_id,
			boost::shared_ptr<GPlatesFileIO::DataSource> data_source,
			unsigned line_num,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		using namespace GPlatesFileIO;
		using namespace GPlatesModel;
		using namespace GPlatesPropertyValues;

		// Firstly, let's read the six integer and floating-point fields.  (Note that the
		// variables for the moving and fixed plate IDs were passed into this function as
		// return-parameters.)
		double geo_time;
		double pole_latitude;
		double pole_longitude;
		double rotation_angle;

		if ( ! (iss >> moving_plate_id)) {
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::ErrorReadingMovingPlateId;
			ReadErrors::Result res = ReadErrors::PoleDiscarded;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_recoverable_errors.push_back(read_error);

			throw PoleParsingException();
		}
		if ( ! (iss >> geo_time)) {
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::ErrorReadingGeoTime;
			ReadErrors::Result res = ReadErrors::PoleDiscarded;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_recoverable_errors.push_back(read_error);

			throw PoleParsingException();
		}
		if ( ! (iss >> pole_latitude)) {
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::ErrorReadingPoleLatitude;
			ReadErrors::Result res = ReadErrors::PoleDiscarded;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_recoverable_errors.push_back(read_error);

			throw PoleParsingException();
		}
		if ( ! (iss >> pole_longitude)) {
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::ErrorReadingPoleLongitude;
			ReadErrors::Result res = ReadErrors::PoleDiscarded;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_recoverable_errors.push_back(read_error);

			throw PoleParsingException();
		}
		if ( ! (iss >> rotation_angle)) {
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::ErrorReadingRotationAngle;
			ReadErrors::Result res = ReadErrors::PoleDiscarded;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_recoverable_errors.push_back(read_error);

			throw PoleParsingException();
		}
		if ( ! (iss >> fixed_plate_id)) {
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::ErrorReadingFixedPlateId;
			ReadErrors::Result res = ReadErrors::PoleDiscarded;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_recoverable_errors.push_back(read_error);

			throw PoleParsingException();
		}

		// Now, from the remainder of the input line, extract the comment.
		std::string comment;
		extract_comment(iss, comment, data_source, line_num, read_errors);

		// Did the pole have valid lat and lon?
		if ( ! GPlatesMaths::LatLonPoint::is_valid_latitude(pole_latitude))
		{
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::InvalidPoleLatitude;
			ReadErrors::Result res = ReadErrors::PoleDiscarded;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_recoverable_errors.push_back(read_error);

			throw PoleParsingException();
		}
		if ( ! GPlatesMaths::LatLonPoint::is_valid_longitude(pole_longitude))
		{
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::InvalidPoleLongitude;
			ReadErrors::Result res = ReadErrors::PoleDiscarded;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_recoverable_errors.push_back(read_error);

			throw PoleParsingException();
		}

		std::pair<double, double> lon_lat_euler_pole(pole_longitude, pole_latitude);

		GpmlFiniteRotation::non_null_ptr_type value =
				GpmlFiniteRotation::create(lon_lat_euler_pole, rotation_angle);

		GeoTimeInstant geo_time_instant(geo_time);
		GmlTimeInstant::non_null_ptr_type valid_time =
				ModelUtils::create_gml_time_instant(geo_time_instant);

		boost::intrusive_ptr<XsString> description;
		if ( ! comment.empty()) {
			description = (XsString::create(comment.c_str())).get();
		}

		StructuralType value_type = 
			StructuralType::create_gpml("FiniteRotation");

		// Finally, as we're creating the GpmlTimeSample, don't forget to check whether the
		// sample should be disabled.
		if (moving_plate_id == 999) {
			return GpmlTimeSample(value, valid_time, description, value_type, true);
		} else {
			return GpmlTimeSample(value, valid_time, description, value_type);
		}
	}


	void
	warn_user_about_new_overlapping_sequence(
			const GPlatesPropertyValues::GpmlTimeSample &time_sample,
			const GPlatesPropertyValues::GpmlTimeSample &prev_time_sample,
			boost::shared_ptr<GPlatesFileIO::DataSource> data_source,
			unsigned line_num,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		using namespace GPlatesFileIO;

		if (gml_time_instants_are_approx_equal(time_sample.valid_time(), prev_time_sample.valid_time())) {
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::SamePlateIdsButDuplicateGeoTime;
			ReadErrors::Result res = ReadErrors::NewOverlappingSequenceBegun;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_warnings.push_back(read_error);
		} else {
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::SamePlateIdsButEarlierGeoTime;
			ReadErrors::Result res = ReadErrors::NewOverlappingSequenceBegun;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_warnings.push_back(read_error);
		}
	}


	struct TotalReconSeqProperties
	{
		TotalReconSeqProperties()
		{  }

		boost::intrusive_ptr<GPlatesPropertyValues::GpmlIrregularSampling> d_irregular_sampling;
		GPlatesModel::FeatureHandle::iterator d_irregular_sampling_iter;
		GPlatesModel::integer_plate_id_type d_fixed_plate_id;
		GPlatesModel::integer_plate_id_type d_moving_plate_id;
	};


	void
	create_total_recon_seq(
			GPlatesModel::FeatureCollectionHandle::weak_ref &rotations,
			GPlatesModel::FeatureHandle::weak_ref &current_total_recon_seq,
			TotalReconSeqProperties &props_in_current_trs,
			const GPlatesPropertyValues::GpmlTimeSample &time_sample,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id)
	{
		using namespace GPlatesModel;
		using namespace GPlatesPropertyValues;

		// Create a new total reconstruction sequence in the feature collection.
		FeatureType feature_type = FeatureType::create_gpml("TotalReconstructionSequence");
		current_total_recon_seq = GPlatesModel::FeatureHandle::create(rotations, feature_type);

		GpmlInterpolationFunction::non_null_ptr_type gpml_finite_rotation_slerp =
				GpmlFiniteRotationSlerp::create(time_sample.value_type());
		GpmlIrregularSampling::non_null_ptr_type gpml_irregular_sampling =
				GpmlIrregularSampling::create(time_sample,
						GPlatesUtils::get_intrusive_ptr(gpml_finite_rotation_slerp),
						time_sample.value_type());

		// We retain an iterator that points to the property in the model. This is
		// because we cannot modify the model's copy of the property directly and we
		// need to modify a copy of the property outside the model. The iterator then
		// allows us to "set" the property in the feature after we're done with
		// modifying the property.
		// Note that the "gpml:totalReconstructionPole" property has to come first
		// otherwise the PlatesRotationFormatWriter barfs.
		props_in_current_trs.d_irregular_sampling_iter = current_total_recon_seq->add(
				TopLevelPropertyInline::create(
					PropertyName::create_gpml("totalReconstructionPole"),
					gpml_irregular_sampling));
		props_in_current_trs.d_irregular_sampling = gpml_irregular_sampling.get();

		GpmlPlateId::non_null_ptr_type fixed_ref_frame =
				GpmlPlateId::create(fixed_plate_id);
		current_total_recon_seq->add(
				TopLevelPropertyInline::create(
					PropertyName::create_gpml("fixedReferenceFrame"),
					fixed_ref_frame));
		props_in_current_trs.d_fixed_plate_id = fixed_plate_id;

		GpmlPlateId::non_null_ptr_type moving_ref_frame =
				GpmlPlateId::create(moving_plate_id);
		current_total_recon_seq->add(
				TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("movingReferenceFrame"),
					moving_ref_frame));
		props_in_current_trs.d_moving_plate_id = moving_plate_id;
	}


	/**
	 * Add a time sample to an irregular sequence.
	 *
	 * Also adjusts the pole, if necessary, so that the stage rotation relative to previous pole
	 * takes the short way around the globe (instead of long way).
	 * Also emits a read error (warning) if an adjustment was made.
	 */
	void
	add_time_sample(
			std::vector<GPlatesPropertyValues::GpmlTimeSample> &time_samples,
			GPlatesPropertyValues::GpmlTimeSample &time_sample,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &data_source,
			unsigned line_num,
			GPlatesFileIO::ReadErrorAccumulation &read_errors,
			bool &contains_unsaved_changes)
	{
		using namespace GPlatesFileIO;
		using namespace GPlatesModel;
		using namespace GPlatesPropertyValues;

		//
		// Adjust the time sample's total pole, if necessary, so that the stage rotation from the
		// previous pole takes the short rotation path instead of the long path.
		//
		// Both poles must be enabled before this adjustment is attempted.
		//
		if (!time_sample.is_disabled())
		{
			// Search backwards for most recently added time sample (that's enabled).
			for (unsigned int n = 0; n < time_samples.size(); ++n)
			{
				const GpmlTimeSample &prev_enabled_time_sample = time_samples[time_samples.size() - n - 1];
				if (prev_enabled_time_sample.is_disabled())
				{
					continue;
				}

				const GpmlFiniteRotation *prev_gpml_finite_rotation =
						dynamic_cast<const GpmlFiniteRotation *>(
								prev_enabled_time_sample.value().get());
				GpmlFiniteRotation *curr_gpml_finite_rotation =
						dynamic_cast<GpmlFiniteRotation *>(
								time_sample.value().get());
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						prev_gpml_finite_rotation && curr_gpml_finite_rotation,
						GPLATES_ASSERTION_SOURCE);

				// Make sure the stage rotation (relative to previous total pole) takes the short path.
				boost::optional<GPlatesMaths::FiniteRotation> adjusted_curr_finite_rotation =
						GPlatesAppLogic::RotationUtils::calculate_short_path_final_rotation(
								curr_gpml_finite_rotation->finite_rotation(),
								prev_gpml_finite_rotation->finite_rotation());
				if (adjusted_curr_finite_rotation)
				{
					// Change the current finite rotation for short path.
					curr_gpml_finite_rotation->set_finite_rotation(adjusted_curr_finite_rotation.get());

					// The loaded finite rotation differs from what was read from the file.
					contains_unsaved_changes = true;

					// Warn the user that the change was made.
					boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
					ReadErrors::Description descr = ReadErrors::PoleTakesLongRotationPathRelativeToPrevPole;
					ReadErrors::Result res = ReadErrors::PoleAdjustedToShortRotationPathRelativeToPrevPole;
					ReadErrorOccurrence read_error(data_source, location, descr, res);
					read_errors.d_warnings.push_back(read_error);
				}

				break;
			}
		}

		time_samples.push_back(time_sample);
	}


	// FIXME:  Give this a better name (and do the exception properly).
	struct UnexpectedlyNullIrregularSampling {  };


	void
	append_pole_to_data_set(
			GPlatesModel::FeatureCollectionHandle::weak_ref &rotations,
			GPlatesModel::FeatureHandle::weak_ref &current_total_recon_seq,
			TotalReconSeqProperties &props_in_current_trs,
			GPlatesPropertyValues::GpmlTimeSample &time_sample,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			boost::shared_ptr<GPlatesFileIO::DataSource> data_source,
			unsigned line_num,
			GPlatesFileIO::ReadErrorAccumulation &read_errors,
			bool &contains_unsaved_changes)
	{
		using namespace GPlatesFileIO;
		using namespace GPlatesModel;
		using namespace GPlatesPropertyValues;

		// We're going to use some messy code logic to handle the rather arbitrary
		// interactions of various fields in the file format.  Don't blame me -- I've tried
		// repeatedly to impose order, but each time I think I've devised a simple, elegant
		// system which accurately captures the interactions, I discover an exception which
		// breaks my system.
		//
		// Currently, the basic structure is provided by the geo-time of the poles:  If the
		// geo-time of the current pole is less-than or equal-to the geo-time of the
		// previous pole, we assume it's the start of a new sequence, since the geo-times
		// of the poles within a sequence must be monotonically increasing.  (If the plate
		// IDs of the current pole are the same as the corresponding plate IDs of the
		// previous pole, then a warning is logged to inform the user that a new sequence
		// was begun, which overlaps with the previous sequence.)
		//
		// After that, we consider the moving plate ID and fixed plate ID of the pole:  If
		// either is different to the corresponding plate ID of the previous pole, then it
		// is the start of a new sequence (by definition, since a total reconstruction
		// sequence is defined to be an interpolatable sequence of poles, and it is only
		// valid to interpolate between poles whose corresponding plate IDs are the same),
		// UNLESS the fixed plate ID of the current pole is the same as the fixed plate ID
		// of the previous pole and the moving plate ID of the current pole is 999.  (It
		// was observed that runs of poles, with the same corresponding plate IDs and
		// monotonically-increasing geo-times, were being interrupted by poles which would
		// have fit into the sequence had they not had moving plate IDs of 999.  Since 999
		// is the plate ID to denote comments, it was assumed that these poles which had a
		// moving plate ID of 999 were meant to be part of the sequence, but "commented
		// out".)  If the fixed plate ID of the current pole is the same as the fixed plate
		// ID of the previous pole and the moving plate ID of the current pole is 999, it
		// is assumed that the current pole was intended by the user to be commented-out
		// but still part of the sequence.  The moving plate ID of the current pole is
		// changed to be the same as the moving plate ID of the previous pole, the current
		// pole is set to be commented-out, and a warning is logged to inform the user that
		// this interpretation was made.

		if ( ! current_total_recon_seq.is_valid())
		{

			// There are not yet any total reconstruction sequences in the feature
			// collection, which means that we need to create the first one.

			create_total_recon_seq(rotations, current_total_recon_seq,
					props_in_current_trs, time_sample, fixed_plate_id, moving_plate_id);

			// Since this was the very first pole in the very first sequence, we don't
			// need to worry about comparing with the previous sequence of poles.
			return;
		}

		// Otherwise, the feature collection is not empty; we've already created (at least)
		// one total reconstruction sequence, which is what 'current_total_recon_seq' is
		// referencing.

		// Now let's compare the valid-time of the current time-sample with the valid-time
		// of the previous time-sample.

		// Firstly, we need to extract the previous time-sample from the irregular
		// sampling in the current TRS.

		// FIXME:  Refactor the next if-statement out into a different function.
		if (props_in_current_trs.d_irregular_sampling.get() == NULL)
		{
			// There's a problem here:  The pointer is NULL (which means that we don't
			// have a pointer to an irregular sampling) but the pointer should be NULL
			// if and only if 'current_total_recon_seq' is invalid for dereferencing.
			// But we've reached this point in the code precisely because
			// 'current_total_recon_seq' *is* valid for dereferencing.  Thus, there's
			// been some sort of internal error.
			throw UnexpectedlyNullIrregularSampling();
		}

		// The current time samples.
		std::vector<GpmlTimeSample> &time_samples =
				props_in_current_trs.d_irregular_sampling->time_samples();

		// The previous time sample (disabled or enabled).
		const GpmlTimeSample prev_time_sample = time_samples.back();

		if (gml_time_instants_are_approx_equal(time_sample.valid_time(), prev_time_sample.valid_time()))
		{
			// We'll assume it's the start of a new sequence.  Since we're cautious
			// programmers, let's just double-check whether the plate IDs are the same.

			// Let's be more lenient if the current pole has the same geo-time as the
			// previous commented-out pole when the previous pole (with the same fixed
			// plate ID and the same moving plate ID), since one might assume that the
			// current pole is intended to replace the previous pole.  Let's be
			// similarly lenient if the current pole is the commented-out one, and the
			// previous pole is the non-commented-out one.
			//
			// FIXME:  Re-read that first sentence.  What does it mean?
			if (prev_time_sample.is_disabled() &&
					props_in_current_trs.d_fixed_plate_id == fixed_plate_id &&
					props_in_current_trs.d_moving_plate_id == moving_plate_id)
			{
				add_time_sample(time_samples, time_sample, data_source, line_num, read_errors, contains_unsaved_changes);
			}
			else if (moving_plate_id == 999 &&
					props_in_current_trs.d_fixed_plate_id == fixed_plate_id)
			{
				// Let's assume the current pole was intended to be part of the
				// sequence, but commented-out.
				add_time_sample(time_samples, time_sample, data_source, line_num, read_errors, contains_unsaved_changes);

				// Don't forget to warn the user that the moving plate ID of the
				// pole was changed as part of this interpretation.

				boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
				ReadErrors::Description descr = ReadErrors::CommentMovingPlateIdAfterNonCommentSequence;
				ReadErrors::Result res = ReadErrors::MovingPlateIdChangedToMatchEarlierSequence;
				ReadErrorOccurrence read_error(data_source, location, descr, res);
				read_errors.d_warnings.push_back(read_error);
			}
			else
			{
				if (props_in_current_trs.d_fixed_plate_id == fixed_plate_id &&
						props_in_current_trs.d_moving_plate_id == moving_plate_id)
				{

					// The plate IDs of the current pole are the same as the
					// corresponding plate IDs of the previous pole.  We'll
					// warn the user that a new sequence has been begun, which
					// overlaps with the previous sequence.
					//
					// EXCEPT that there's no point warning the user if both
					// the fixed and moving plate IDs are 999, since the lines
					// are just comments.
					if ( ! (moving_plate_id == 999 && fixed_plate_id == 999))
					{
						warn_user_about_new_overlapping_sequence(
								time_sample, prev_time_sample,
								data_source, line_num, read_errors);
					}
				}
				create_total_recon_seq(rotations, current_total_recon_seq,
						props_in_current_trs, time_sample, fixed_plate_id,
						moving_plate_id);
			}
		}
		else if (time_sample.valid_time()->time_position().value() <
				prev_time_sample.valid_time()->time_position().value())
		{
			// We'll assume it's the start of a new sequence.  Since we're cautious
			// programmers, let's just double-check whether the plate IDs are the same.

			// Ignore commented-out poles.
			if (props_in_current_trs.d_moving_plate_id == moving_plate_id &&
					moving_plate_id != 999)
			{
				// The moving plate ID of the current pole is the same as the
				// moving plate ID of the previous pole.  We'll warn the user that
				// a new sequence has been begun, which overlaps with the previous
				// sequence.
				//
				// EXCEPT that there's no point warning the user if both the fixed
				// and moving plate IDs are 999, since the lines are just comments.
				if ( ! (moving_plate_id == 999 && fixed_plate_id == 999))
				{
					warn_user_about_new_overlapping_sequence(time_sample,
							prev_time_sample, data_source, line_num,
							read_errors);
				}
			}
			create_total_recon_seq(rotations, current_total_recon_seq,
					props_in_current_trs, time_sample, fixed_plate_id,
					moving_plate_id);
		}
		else
		{
			// The geo-time of the current pole is greater-than the geo-time of the
			// previous pole.  Let's compare the plate IDs of the current pole with the
			// plate IDs of the previous pole.

			// First, let's check for the special case when the moving plate ID == 999
			// and the fixed plate ID is the same as the previous fixed plate ID.
			if (moving_plate_id == 999 &&
					props_in_current_trs.d_fixed_plate_id == fixed_plate_id)
			{
				// OK, it's the special case.  Let's assume the current pole was
				// intended to be part of the sequence, but commented-out.
				add_time_sample(time_samples, time_sample, data_source, line_num, read_errors, contains_unsaved_changes);

				// Don't forget to warn the user that the moving plate ID of the
				// pole was changed as part of this interpretation.

				boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
				ReadErrors::Description descr = ReadErrors::CommentMovingPlateIdAfterNonCommentSequence;
				ReadErrors::Result res = ReadErrors::MovingPlateIdChangedToMatchEarlierSequence;
				ReadErrorOccurrence read_error(data_source, location, descr, res);
				read_errors.d_warnings.push_back(read_error);
				*(props_in_current_trs.d_irregular_sampling_iter) =
					GPlatesModel::TopLevelPropertyInline::create(
							GPlatesModel::PropertyName::create_gpml("totalReconstructionPole"),
							props_in_current_trs.d_irregular_sampling.get());
				return;
			}

			// OK, now to handle the regular cases -- comparing the plate IDs to
			// determine whether the pole should be part of the sequence or not.
			if (props_in_current_trs.d_fixed_plate_id != fixed_plate_id ||
					props_in_current_trs.d_moving_plate_id != moving_plate_id)
			{
				// The sequence has a different fixed ref frame or moving ref frame
				// to those of the pole, so we need to commence a *new* sequence.
				create_total_recon_seq(rotations, current_total_recon_seq,
						props_in_current_trs, time_sample, fixed_plate_id,
						moving_plate_id);
			}
			else
			{
				add_time_sample(time_samples, time_sample, data_source, line_num, read_errors, contains_unsaved_changes);
			}
		}

		// Now that we've finished modifying the property, let's set the model's
		// copy of the property to our modified copy.
		*(props_in_current_trs.d_irregular_sampling_iter) =
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("totalReconstructionPole"),
					props_in_current_trs.d_irregular_sampling.get());
	}


	void
	handle_parsed_pole(
			GPlatesModel::FeatureCollectionHandle::weak_ref &rotations,
			GPlatesModel::FeatureHandle::weak_ref &current_total_recon_seq,
			TotalReconSeqProperties &props_in_current_trs,
			GPlatesPropertyValues::GpmlTimeSample &time_sample,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			boost::shared_ptr<GPlatesFileIO::DataSource> data_source,
			unsigned line_num,
			GPlatesFileIO::ReadErrorAccumulation &read_errors,
			bool &contains_unsaved_changes)
	{
		using namespace GPlatesFileIO;

		if (fixed_plate_id == moving_plate_id && moving_plate_id != 999) {
			boost::shared_ptr<LocationInDataSource> location(new LineNumber(line_num));
			ReadErrors::Description descr = ReadErrors::MovingPlateIdEqualsFixedPlateId;
			ReadErrors::Result res = ReadErrors::PoleDiscarded;
			ReadErrorOccurrence read_error(data_source, location, descr, res);
			read_errors.d_recoverable_errors.push_back(read_error);

			return;
		}

		append_pole_to_data_set(rotations, current_total_recon_seq,
				props_in_current_trs, time_sample, fixed_plate_id,
				moving_plate_id, data_source, line_num, read_errors, contains_unsaved_changes);
	}


	/**
	 * Populate the feature collection @a rotations with the contents of a PLATES
	 * rotation-format file contained within @a line_buffer.
	 */
	void
	populate_rotations(
			GPlatesModel::FeatureCollectionHandle::weak_ref &rotations,
			GPlatesFileIO::LineReader &line_buffer,
			boost::shared_ptr<GPlatesFileIO::DataSource> data_source,
			GPlatesFileIO::ReadErrorAccumulation &read_errors,
			bool &contains_unsaved_changes)
	{
		std::string line_of_input;

		// When this iterator is default-constructed, it is not valid for dereferencing.
		GPlatesModel::FeatureHandle::weak_ref current_total_recon_seq;

		TotalReconSeqProperties props_in_current_trs;

		while (line_buffer.getline(line_of_input)) {
			std::istringstream line_stream(line_of_input);
			GPlatesModel::integer_plate_id_type fixed_plate_id, moving_plate_id;

			try {
				GPlatesPropertyValues::GpmlTimeSample time_sample =
						parse_pole(line_stream, fixed_plate_id, moving_plate_id,
								data_source, line_buffer.line_number(),
								read_errors);

				handle_parsed_pole(rotations, current_total_recon_seq,
						props_in_current_trs, time_sample,
						fixed_plate_id, moving_plate_id, data_source,
						line_buffer.line_number(), read_errors, contains_unsaved_changes);
			} catch (PoleParsingException &) {
				// The argument name in the above expression was removed to
				// prevent "unreferenced local variable" compiler warnings under MSVC

				// There was some error parsing the pole from the line.
				continue;
			}
		}
	}

}


void
GPlatesFileIO::PlatesRotationFormatReader::read_file(
		File::Reference &file,
		ReadErrorAccumulation &read_errors,
		bool &contains_unsaved_changes)
{
	PROFILE_FUNC();

	contains_unsaved_changes = false;

	const FileInfo &fileinfo = file.get_file_info();

	QString filename = fileinfo.get_qfileinfo().absoluteFilePath();
	// Open the file for reading.
	QFile input(filename);
	if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}
	LineReader line_buffer(input);
	boost::shared_ptr<DataSource> data_source(
			new LocalFileDataSource(filename, DataFormats::PlatesRotation));

	GPlatesModel::FeatureCollectionHandle::weak_ref rotations = file.get_feature_collection();

	try
	{
		populate_rotations(rotations, line_buffer, data_source, read_errors, contains_unsaved_changes);
	} catch (UnexpectedlyNullIrregularSampling &) {
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC

		// There was an internal error, after which we can't really proceed.
		// FIXME:  Handle this exception properly, with logging of the exception, etc.
	}
}

