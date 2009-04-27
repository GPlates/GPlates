/* $Id$ */

/**
 * \file Generates a sequence of filenames given a filename template.
 * 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <cmath>
#include <ostream>

#include "ExportTemplateFilenameSequence.h"

#include "ExportTemplateFilenameSequenceImpl.h"
#include "global/UninitialisedIteratorException.h"


GPlatesUtils::ExportTemplateFilenameSequence::ExportTemplateFilenameSequence(
		const QString &filename_template,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const GPlatesMaths::real_t &begin_reconstruction_time,
		const GPlatesMaths::real_t &end_reconstruction_time,
		const GPlatesMaths::real_t &reconstruction_time_increment,
		const bool include_end_time_in_sequence)
{
	// Begin/end reconstruction should not be the same.
	if (begin_reconstruction_time == end_reconstruction_time)
	{
		throw ExportTemplateFilename::BeginEndTimesEqual(GPLATES_EXCEPTION_SOURCE);
	}

	// Reconstruction time increment should not be zero.
	if (reconstruction_time_increment == 0)
	{
		throw ExportTemplateFilename::TimeIncrementZero(GPLATES_EXCEPTION_SOURCE);
	}

	// The sign of the reconstruction time increment should match the sign of
	// end minus begin reconstruction times.
	if (isPositive(reconstruction_time_increment) !=
		isPositive(end_reconstruction_time - begin_reconstruction_time))
	{
		throw ExportTemplateFilename::IncorrectTimeIncrementSign(GPLATES_EXCEPTION_SOURCE);
	}

	//
	// We've passed validity tests related to the constructor parameters
	// except 'filename_template' so create the sequence implementation.
	//

	const std::size_t sequence_size = calc_sequence_size(
			begin_reconstruction_time.dval(),
			end_reconstruction_time.dval(),
			reconstruction_time_increment.dval(),
			include_end_time_in_sequence);

	// Constructor of ExportTemplateFilenameSequenceImpl can throw exceptions.
	// If it does then delete will get called by compiler and we're exception safe.
	d_impl.reset(new ExportTemplateFilenameSequenceImpl(
			filename_template,
			reconstruction_anchor_plate_id,
			begin_reconstruction_time.dval(),
			reconstruction_time_increment.dval(),
			sequence_size));
}


std::size_t
GPlatesUtils::ExportTemplateFilenameSequence::calc_sequence_size(
		const double &begin_reconstruction_time,
		const double &end_reconstruction_time,
		const double &reconstruction_time_increment,
		const bool include_end_time_in_sequence)
{
	// Determine ratio of reconstruction time range over the time increment.
	const double delta_begin_end_time = (end_reconstruction_time - begin_reconstruction_time);
	const double floating_point_ratio_delta_over_time_increment =
			delta_begin_end_time / reconstruction_time_increment;

	// Truncate to integer (cast truncates towards zero).
	const std::size_t integer_ratio_delta_over_time_increment =
			static_cast<std::size_t>(floating_point_ratio_delta_over_time_increment);

	// The end reconstruction time is bound by two multiplies of the time increment.
	const double previous_multiple = integer_ratio_delta_over_time_increment *
			reconstruction_time_increment;
	const double next_multiple = (integer_ratio_delta_over_time_increment + 1) *
			reconstruction_time_increment;

	// If reconstruction time range is close enough to a multiple of the time increment then
	// we need to consider the 'include_end_time_in_sequence' flag.
	// 1 percent is considered close enough - it's not too small to interact with the
	// floating-point precision of a double and not too large.
	const double epsilon = 1e-2;

	if (std::abs(delta_begin_end_time - previous_multiple) < epsilon * std::abs(reconstruction_time_increment))
	{
		// Time range is close to a multiple of 'previous_multiple'.
		return integer_ratio_delta_over_time_increment +
				(include_end_time_in_sequence ? 1 : 0);
	}
	else if (std::abs(delta_begin_end_time - next_multiple) < epsilon * std::abs(reconstruction_time_increment))
	{
		// Time range is close to a multiple of 'next_multiple'.
		return integer_ratio_delta_over_time_increment + 1 +
				(include_end_time_in_sequence ? 1 : 0);
	}

	// Time range is somewhere between a multiple of 'previous_multiple' and 'next_multiple'.
	// Hence the end time is not really close enough to a multiple of the time increment to be
	// included so don't include it.
	return integer_ratio_delta_over_time_increment + 1;
}


std::size_t
GPlatesUtils::ExportTemplateFilenameSequence::size() const
{
	return d_impl->size();
}


GPlatesUtils::ExportTemplateFilenameSequence::const_iterator
GPlatesUtils::ExportTemplateFilenameSequence::begin() const
{
	return ExportTemplateFilenameSequenceIterator(
			d_impl.get(), 0/*sequence_index*/);
}


GPlatesUtils::ExportTemplateFilenameSequence::const_iterator
GPlatesUtils::ExportTemplateFilenameSequence::end() const
{
	return ExportTemplateFilenameSequenceIterator(
			d_impl.get(), d_impl->size()/*sequence_index*/);
}


const QString
GPlatesUtils::ExportTemplateFilenameSequenceIterator::operator*() const
{
	if (d_sequence_impl == NULL)
	{
		throw GPlatesGlobal::UninitialisedIteratorException(GPLATES_EXCEPTION_SOURCE,
				"Attempted to dereference an uninitialised iterator.");
	}

	// Get the date/time when this iterator is first dereferenced.
	// From now on this iterator will have this same constant date/time.
	if (d_first_dereference)
	{
		d_first_dereference = false;

		d_date_time = QDateTime::currentDateTime();
	}

	return d_sequence_impl->get_filename(d_sequence_index, d_date_time);
}


void
GPlatesUtils::ExportTemplateFilename::UnrecognisedFormatString::write_message(
		std::ostream &os) const
{
	os
			<< "The beginning of '"
			<< d_format_string.toStdString()
			<< "' is not recognised as a valid format specifier.";
}
