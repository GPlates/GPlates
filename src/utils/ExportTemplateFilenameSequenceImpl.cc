/* $Id$ */

/**
 * \file Generates a sequence of filenames given a filename template.
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

#include <memory>
#include <string>
#include <QLatin1Char>

#include "ExportTemplateFilenameSequenceImpl.h"
#include "ExportTemplateFilenameSequence.h"
#include "ExportTemplateFilenameSequenceFormats.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"



GPlatesUtils::ExportTemplateFilenameSequenceImpl::ExportTemplateFilenameSequenceImpl(
		const QString &filename_template,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &begin_reconstruction_time,
		const double &reconstruction_time_increment,
		const GPlatesUtils::AnimationSequence::SequenceInfo sequence_info) :
	d_filename_template(filename_template),
	d_begin_reconstruction_time(begin_reconstruction_time),
	d_reconstruction_time_increment(reconstruction_time_increment),
	d_sequence_info(sequence_info)
{
	FormatExtractor format_extractor(
			d_filename_template,
			reconstruction_anchor_plate_id,
			d_format_seq,
			sequence_info);

	format_extractor.extract_formats_from_filename_template();
}


QString
GPlatesUtils::ExportTemplateFilenameSequenceImpl::get_filename(
		const std::size_t sequence_index,
		const QDateTime &date_time) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			sequence_index < d_sequence_info.duration_in_frames,
			GPLATES_ASSERTION_SOURCE);

	// Get the reconstruction time for the current sequence index.
	const double reconstruction_time = GPlatesUtils::AnimationSequence::calculate_time_for_frame(
			d_sequence_info, sequence_index);

	// Make a copy of the filename template as we are going to modify it.
	QString filename(d_filename_template);

	// Iterate through all the format patterns in the filename template
	// and replace them with the appropriate string based on the current
	// position in the sequence of filenames.
	format_seq_type::const_iterator format_iter;
	for (format_iter = d_format_seq.begin();
		format_iter != d_format_seq.end();
		++format_iter)
	{
		ExportTemplateFilename::Format *format = format_iter->get();

		// Get the format string to expand itself using the current
		// reconstruction time/frame.
		const QString expanded_format_string = format->expand_format_string(
				sequence_index,
				reconstruction_time,
				date_time);

		// Replace the first occurrence of %1, %2, etc in the filename
		// string with expanded format string.
		filename = filename.arg(expanded_format_string);
	}

	return filename;
}


void
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::extract_formats_from_filename_template()
{
	const QChar percent_char = QLatin1Char('%');

	for (d_filename_current_pos = 0;
		d_filename_current_pos < d_filename_template.size();
		)
	{
		// See if current character is '%'.
		if (d_filename_template[d_filename_current_pos] != percent_char)
		{
			// Keep looking for '%' char.
			++d_filename_current_pos;
			continue;
		}

		// Search for a matching format and create matching format object.
		QString format_string;
		format_ptr_type format = create_format(format_string);

		// Determine whether what to do with format object based on whether it
		// varies with reconstruction time, is constant over filename sequence or
		// varies across sequence iterators (but is constant across the sequence for
		// a specific iterator).
		handle_format(format, format_string);

		// Continue looking for the next format pattern.
	}

	check_filename_template_varies_with_reconstruction_time();
}


void
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::handle_format(
		format_ptr_type format,
		const QString &format_string)
{
	switch (format->get_variation_type())
	{
	case ExportTemplateFilename::Format::VARIES_WITH_RECONSTRUCTION_TIME_OR_FRAME:
	case ExportTemplateFilename::Format::VARIES_WITH_SEQUENCE_ITERATOR:
		handle_format_varies_with_reconstruction_time_or_iterator(format, format_string);
		break;

	case ExportTemplateFilename::Format::IS_CONSTANT:
		handle_format_is_constant(format, format_string);
		break;

	default:
		throw GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE);
	}
}


void
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::handle_format_is_constant(
		format_ptr_type format,
		const QString &format_string)
{
	//
	// Since there's no filename variation in this format, and it doesn't vary across iterators,
	// we might as well just expand the format directly into the filename template now rather
	// than doing it later for every reconstruction frame/time.
	//

	// Get the format string to expand itself.
	// Since this format is constant always we don't care what parameters we pass in
	// as they'll get ignored.
	const QString expanded_format_string = format->expand_format_string(0, 0, QDateTime());

	d_filename_template.replace(
			d_filename_current_pos,
			format_string.size(),
			expanded_format_string);

	// Skip past the replaced string so we can start looking for the next
	// format string beginning with the '%' char.
	d_filename_current_pos += expanded_format_string.size();
}


void
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::handle_format_varies_with_reconstruction_time_or_iterator(
		format_ptr_type format,
		const QString &format_string)
{
	// Add the new format to our sequence since we'll be using it later to expand
	// this format for different reconstruction frame/times.
	d_format_seq.push_back(format);

	// Replace the format string part of 'd_filename_template' with %1, %2, etc
	// so that we know where to insert into the filename string later.
	QString replace_string = QString("%%1").arg(d_format_index + 1);
	++d_format_index;

	d_filename_template.replace(
			d_filename_current_pos,
			format_string.size(),
			replace_string);

	// Skip past the replaced string so we can start looking for the next
	// format string beginning with the '%' char.
	d_filename_current_pos += replace_string.size();
}


void
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::check_filename_template_varies_with_reconstruction_time()
{
	// Iterate through our format objects and make sure at least one varies with reconstruction time.
	format_seq_type::const_iterator format_iter;
	for (format_iter = d_format_seq.begin();
		format_iter != d_format_seq.end();
		++format_iter)
	{
		ExportTemplateFilename::Format *format = format_iter->get();

		if (format->get_variation_type() ==
				ExportTemplateFilename::Format::VARIES_WITH_RECONSTRUCTION_TIME_OR_FRAME)
		{
			return;
		}
	}

	// There are no format specifiers, in the filename template, that have filename variation
	// so there's no filename variation at all and this is an error.
	throw ExportTemplateFilename::NoFilenameVariation(GPLATES_EXCEPTION_SOURCE);
}


GPlatesUtils::ExportTemplateFilenameSequenceImpl::format_ptr_type
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::create_format(
		QString &format_string)
{
	// The filename template string starting from the current position.
	const QString rest_of_filename_template = d_filename_template.mid(d_filename_current_pos);

	if (match_format<ExportTemplateFilename::PercentCharacterFormat>(
			rest_of_filename_template, format_string))
	{
		return format_ptr_type(new ExportTemplateFilename::PercentCharacterFormat());
	}

	if (match_format<ExportTemplateFilename::ReconstructionAnchorPlateIdFormat>(
			rest_of_filename_template, format_string))
	{
		return format_ptr_type(new ExportTemplateFilename::ReconstructionAnchorPlateIdFormat(
				d_reconstruction_anchor_plate_id));
	}

	if (match_format<ExportTemplateFilename::FrameNumberFormat>(
			rest_of_filename_template, format_string))
	{
		return format_ptr_type(new ExportTemplateFilename::FrameNumberFormat(
				format_string, d_sequence_info.duration_in_frames));
	}

	if (match_format<ExportTemplateFilename::DateTimeFormat>(
			rest_of_filename_template, format_string))
	{
		return format_ptr_type(new ExportTemplateFilename::DateTimeFormat(format_string));
	}

	// NOTE: Extract the printf-style format last in case we mistakenly add
	// a new format that overlaps with printf-style formatting.
	if (match_format<ExportTemplateFilename::ReconstructionTimePrintfFormat>(
			rest_of_filename_template, format_string))
	{
		return format_ptr_type(
				new ExportTemplateFilename::ReconstructionTimePrintfFormat(format_string));
	}

	// No formats matched so we've got a substring starting with '%' that
	// we cannot match - this is an error.
	throw ExportTemplateFilename::UnrecognisedFormatString(GPLATES_EXCEPTION_SOURCE,
			rest_of_filename_template);
}


template <class FormatType>
bool
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::match_format(
		const QString &rest_of_filename_template,
		QString &format_string)
{
	int length_format_string;
	if (!FormatType::match_format(rest_of_filename_template, length_format_string))
	{
		return false;
	}

	// Extract the format string matched by the format pattern.
	format_string = rest_of_filename_template.left(length_format_string);

	return true;
}
