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
#include "utils/AnimationSequenceUtils.h"


void
GPlatesFileIO::ExportTemplateFilename::validate_filename_template(
		const QString &filename_template)
{
	ExportTemplateFilenameSequenceImpl::validate_filename_template(filename_template);
}


GPlatesFileIO::ExportTemplateFilenameSequence::ExportTemplateFilenameSequence(
		const QString &filename_template,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const GPlatesMaths::real_t &begin_reconstruction_time,
		const GPlatesMaths::real_t &end_reconstruction_time,
		const GPlatesMaths::real_t &reconstruction_time_increment,
		const bool include_trailing_frame_in_sequence)
{
	// Reconstruction time increment should not be zero.
	if (reconstruction_time_increment == 0)
	{
		throw ExportTemplateFilename::TimeIncrementZero(GPLATES_EXCEPTION_SOURCE);
	}

	// The sign of the reconstruction time increment should match the sign of
	// end minus begin reconstruction times.
	if (is_strictly_positive(reconstruction_time_increment) !=
		is_strictly_positive(end_reconstruction_time - begin_reconstruction_time))
	{
		throw ExportTemplateFilename::IncorrectTimeIncrementSign(GPLATES_EXCEPTION_SOURCE);
	}

	//
	// We've passed validity tests related to the constructor parameters
	// except 'filename_template' so create the sequence implementation.
	//

	GPlatesUtils::AnimationSequence::SequenceInfo sequence_info =
			GPlatesUtils::AnimationSequence::calculate_sequence(
					begin_reconstruction_time.dval(),
					end_reconstruction_time.dval(),
					std::fabs(reconstruction_time_increment.dval()),
					include_trailing_frame_in_sequence);

	// Constructor of ExportTemplateFilenameSequenceImpl can throw exceptions.
	// If it does then delete will get called by compiler and we're exception safe.
	d_impl.reset(new ExportTemplateFilenameSequenceImpl(
			filename_template,
			reconstruction_anchor_plate_id,
			begin_reconstruction_time.dval(),
			reconstruction_time_increment.dval(),
			sequence_info));
}




std::size_t
GPlatesFileIO::ExportTemplateFilenameSequence::size() const
{
	return d_impl->size();
}


GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator
GPlatesFileIO::ExportTemplateFilenameSequence::begin() const
{
	return ExportTemplateFilenameSequenceIterator(
			d_impl.get(), 0/*sequence_index*/);
}


GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator
GPlatesFileIO::ExportTemplateFilenameSequence::end() const
{
	return ExportTemplateFilenameSequenceIterator(
			d_impl.get(), d_impl->size()/*sequence_index*/);
}


const QString
GPlatesFileIO::ExportTemplateFilenameSequenceIterator::operator*() const
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
GPlatesFileIO::ExportTemplateFilename::UnrecognisedFormatString::write_message(
		std::ostream &os) const
{
	os
			<< "The beginning of '"
			<< d_format_string.toStdString()
			<< "' is not recognised as a valid format specifier.";
}
