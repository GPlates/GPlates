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
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/lambda.hpp>
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


void
GPlatesUtils::ExportTemplateFilenameSequenceImpl::validate_filename_template(
		const QString &filename_template)
{
	FormatExtractor::validate_filename_template(filename_template);
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


template <class FormatType>
void
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::CreateFormat::operator()(
		Wrap<FormatType>)
{
	// Return if we've already found a matching format.
	if (d_format_info)
	{
		return;
	}

	// The filename template string starting from the current position.
	const QString rest_of_filename_template =
			d_format_extractor->d_filename_template.mid(
					d_format_extractor->d_filename_current_pos);

	const boost::optional<QString> format_string =
			match_format<FormatType>(rest_of_filename_template);

	// If format doesn't match then return early.
	if (!format_string)
	{
		return;
	}

	// Create the format object.
	const format_ptr_type format = d_format_extractor->create_format<FormatType>(*format_string);

	// Store format object and matched format string internally.
	d_format_info = std::make_pair(format, *format_string);
}


template <class FormatType>
void
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::ValidateFormat::operator()(
		Wrap<FormatType>)
{
	// Return if we've already found a matching format.
	if (d_format_info)
	{
		return;
	}

	const boost::optional<QString> format_string =
			match_format<FormatType>(d_rest_of_filename_template);

	// Return early if no matching format was found.
	if (!format_string)
	{
		return;
	}

	d_format_info = std::make_pair(*format_string, FormatType::VARIATION_TYPE);
}


void
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::validate_filename_template(
		const QString &filename_template)
{
	const QChar percent_char = QLatin1Char('%');
	bool filename_varies_with_reconstruction_time_or_frame = false;

	for (int filename_current_pos = 0; filename_current_pos < filename_template.size(); )
	{
		// See if current character is '%'.
		if (filename_template[filename_current_pos] != percent_char)
		{
			// Keep looking for '%' char.
			++filename_current_pos;
			continue;
		}

		// The filename template string starting from the current position.
		const QString rest_of_filename_template = filename_template.mid(filename_current_pos);

		// Search for a matching format.
		const validate_format_info_type format_info = validate_format(rest_of_filename_template);

		// We only need one matching format to vary with reconstruction time or frame
		// in order for the filename template to also.
		if (format_info.second == ExportTemplateFilename::Format::VARIES_WITH_RECONSTRUCTION_TIME_OR_FRAME)
		{
			filename_varies_with_reconstruction_time_or_frame = true;
		}

		// Advance the filename position by the size of the matching format string.
		filename_current_pos += format_info.first.size();

		// Continue looking for the next format pattern.
	}

	if (!filename_varies_with_reconstruction_time_or_frame)
	{
		// There are no format specifiers, in the filename template, that have filename variation
		// so there's no filename variation at all and this is an error.
		throw ExportTemplateFilename::NoFilenameVariation(GPLATES_EXCEPTION_SOURCE);
	}
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
		const create_format_info_type format_info = create_format();

		// Determine whether what to do with format object based on whether it
		// varies with reconstruction time, is constant over filename sequence or
		// varies across sequence iterators (but is constant across the sequence for
		// a specific iterator).
		handle_format(format_info.first, format_info.second);

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


template <class FormatType>
boost::optional<QString>
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::match_format(
		const QString &rest_of_filename_template)
{
	boost::optional<int> length_format_string = FormatType::match_format(rest_of_filename_template);

	if (!length_format_string)
	{
		return boost::none;
	}

	// Extract the format string matched by the format pattern.
	const QString format_string = rest_of_filename_template.left(*length_format_string);

	return format_string;
}


template <class FormatType>
GPlatesUtils::ExportTemplateFilenameSequenceImpl::format_ptr_type
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::create_format(
		const QString &format_string)
{
	return format_ptr_type(new FormatType(format_string));
}


namespace GPlatesUtils
{
	//
	// Template member function specialisations have to be defined in the same
	// namespace as they were declared.
	//

	template <>
	ExportTemplateFilenameSequenceImpl::format_ptr_type
	ExportTemplateFilenameSequenceImpl::FormatExtractor::create_format<
			GPlatesUtils::ExportTemplateFilename::PercentCharacterFormat>(
					const QString &/*format_string*/)
	{
		return format_ptr_type(new ExportTemplateFilename::PercentCharacterFormat());
	}

	template <>
	ExportTemplateFilenameSequenceImpl::format_ptr_type
	ExportTemplateFilenameSequenceImpl::FormatExtractor::create_format<
			GPlatesUtils::ExportTemplateFilename::PlaceholderFormat>(
					const QString &/*format_string*/)
	{
		return format_ptr_type(new ExportTemplateFilename::PlaceholderFormat());
	}


	template <>
	GPlatesUtils::ExportTemplateFilenameSequenceImpl::format_ptr_type
	GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::create_format<
			GPlatesUtils::ExportTemplateFilename::ReconstructionAnchorPlateIdFormat>(
					const QString &/*format_string*/)
	{
		return format_ptr_type(new ExportTemplateFilename::ReconstructionAnchorPlateIdFormat(
				d_reconstruction_anchor_plate_id));
	}


	template <>
	GPlatesUtils::ExportTemplateFilenameSequenceImpl::format_ptr_type
	GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::create_format<
			GPlatesUtils::ExportTemplateFilename::FrameNumberFormat>(
					const QString &format_string)
	{
		return format_ptr_type(new ExportTemplateFilename::FrameNumberFormat(
				format_string, d_sequence_info.duration_in_frames));
	}
}


GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::create_format_info_type
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::create_format()
{
	// Object to finding matching format and create it.
	CreateFormat create_format_object(this);

	// Iterate over all the format types and attempt to match/create a format.
	boost::mpl::for_each< ExportTemplateFilename::format_types, Wrap<boost::mpl::_1> >(
			boost::ref(create_format_object));

	const boost::optional<create_format_info_type> &format_info =
			create_format_object.get_format_info();

	// If there was no matching format then return an error.
	if (!format_info)
	{
		// The filename template string starting from the current position.
		const QString rest_of_filename_template = d_filename_template.mid(d_filename_current_pos);

		// No formats matched so we've got a substring starting with '%' that
		// we cannot match - this is an error.
		throw ExportTemplateFilename::UnrecognisedFormatString(GPLATES_EXCEPTION_SOURCE,
				rest_of_filename_template);
	}

	return *format_info;
}


GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::validate_format_info_type
GPlatesUtils::ExportTemplateFilenameSequenceImpl::FormatExtractor::validate_format(
		const QString &rest_of_filename_template)
{
	// Object to find matching format.
	ValidateFormat validate_format_object(rest_of_filename_template);

	// Iterate over all the format types and attempt to match a format.
	boost::mpl::for_each< ExportTemplateFilename::format_types, Wrap<boost::mpl::_1> >(
			boost::ref(validate_format_object));

	const boost::optional<validate_format_info_type> &format_info =
			validate_format_object.get_format_info();

	// If there was no matching format then return an error.
	if (!format_info)
	{
		// No formats matched so we've got a substring starting with '%' that
		// we cannot match - this is an error.
		throw ExportTemplateFilename::UnrecognisedFormatString(GPLATES_EXCEPTION_SOURCE,
				rest_of_filename_template);
	}

	return *format_info;
}
