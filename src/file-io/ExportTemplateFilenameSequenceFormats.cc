/* $Id$ */

/**
 * \file Various formats used in @a ExportTemplateFilenameSequence.
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

#include <boost/numeric/conversion/cast.hpp>
#include <QRegExp>

#include "ExportTemplateFilenameSequenceFormats.h"

#include "ExportTemplateFilenameSequence.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


const GPlatesFileIO::ExportTemplateFilename::Format::Variation
GPlatesFileIO::ExportTemplateFilename::PercentCharacterFormat::VARIATION_TYPE =
		GPlatesFileIO::ExportTemplateFilename::Format::IS_CONSTANT;


boost::optional<int>
GPlatesFileIO::ExportTemplateFilename::PercentCharacterFormat::match_format(
		const QString &rest_of_filename_template)
{
	if (rest_of_filename_template.startsWith("%%"))
	{
		return 2;
	}

	return boost::none;
}


const GPlatesFileIO::ExportTemplateFilename::Format::Variation
GPlatesFileIO::ExportTemplateFilename::PlaceholderFormat::VARIATION_TYPE =
		GPlatesFileIO::ExportTemplateFilename::Format::IS_CONSTANT;


boost::optional<int>
GPlatesFileIO::ExportTemplateFilename::PlaceholderFormat::match_format(
		const QString &rest_of_filename_template)
{
	if (rest_of_filename_template.startsWith(
			ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING))
	{
		return ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING.size();
	}

	return boost::none;
}


QString
GPlatesFileIO::ExportTemplateFilename::PlaceholderFormat::expand_format_string(
		std::size_t sequence_index,
		const double &reconstruction_time,
		const QDateTime &date_time) const
{
	// Simply return the format string unmodified.
	// The client will be searching for this and expanding it themselves.
	return ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING;
}


const GPlatesFileIO::ExportTemplateFilename::Format::Variation
GPlatesFileIO::ExportTemplateFilename::ReconstructionAnchorPlateIdFormat::VARIATION_TYPE =
		GPlatesFileIO::ExportTemplateFilename::Format::IS_CONSTANT;


boost::optional<int>
GPlatesFileIO::ExportTemplateFilename::ReconstructionAnchorPlateIdFormat::match_format(
		const QString &rest_of_filename_template)
{
	if (rest_of_filename_template.startsWith("%A"))
	{
		return 2;
	}

	return boost::none;
}


QString
GPlatesFileIO::ExportTemplateFilename::ReconstructionAnchorPlateIdFormat::expand_format_string(
		std::size_t /*sequence_index*/,
		const double &/*reconstruction_time*/,
		const QDateTime &/*date_time*/) const
{
	return QString("%1").arg(d_reconstruction_anchor_plate_id);
}


const GPlatesFileIO::ExportTemplateFilename::Format::Variation
GPlatesFileIO::ExportTemplateFilename::DefaultReconstructionTreeLayerNameFormat::VARIATION_TYPE =
		GPlatesFileIO::ExportTemplateFilename::Format::IS_CONSTANT;


boost::optional<int>
GPlatesFileIO::ExportTemplateFilename::DefaultReconstructionTreeLayerNameFormat::match_format(
		const QString &rest_of_filename_template)
{
	if (rest_of_filename_template.startsWith("%R"))
	{
		return 2;
	}

	return boost::none;
}


QString
GPlatesFileIO::ExportTemplateFilename::DefaultReconstructionTreeLayerNameFormat::expand_format_string(
		std::size_t /*sequence_index*/,
		const double &/*reconstruction_time*/,
		const QDateTime &/*date_time*/) const
{
	return d_default_recon_tree_layer_name;
}


const GPlatesFileIO::ExportTemplateFilename::Format::Variation
GPlatesFileIO::ExportTemplateFilename::FrameNumberFormat::VARIATION_TYPE =
		GPlatesFileIO::ExportTemplateFilename::Format::VARIES_WITH_RECONSTRUCTION_TIME_OR_FRAME;


boost::optional<int>
GPlatesFileIO::ExportTemplateFilename::FrameNumberFormat::match_format(
		const QString &rest_of_filename_template)
{
	if (rest_of_filename_template.startsWith("%n") ||
			rest_of_filename_template.startsWith("%u"))
	{
		return 2;
	}

	return boost::none;
}


GPlatesFileIO::ExportTemplateFilename::FrameNumberFormat::FrameNumberFormat(
		const QString &format_string,
		std::size_t sequence_size)
{
	d_use_frame_number = (format_string == "%n");

	calc_max_digits(sequence_size);
}


QString
GPlatesFileIO::ExportTemplateFilename::FrameNumberFormat::expand_format_string(
		std::size_t sequence_index,
		const double &/*reconstruction_time*/,
		const QDateTime &/*date_time*/) const
{
	return QString("%1").arg(
			d_use_frame_number ? sequence_index + 1 : sequence_index,
			d_max_digits,
			10/*base*/,
			QLatin1Char('0'));
}


void
GPlatesFileIO::ExportTemplateFilename::FrameNumberFormat::calc_max_digits(
		std::size_t sequence_size)
{
	int max_frame = boost::numeric_cast<int>(
			d_use_frame_number ? sequence_size + 1 : sequence_size);

	// Enough to cover integer digits in a double. If the frame number gets
	// anywhere near this size then something is really wrong.
	const int MAX_MAX_DIGITS = 16;
	for (d_max_digits = 1; d_max_digits < MAX_MAX_DIGITS; ++d_max_digits)
	{
		max_frame /= 10;

		if (max_frame == 0)
		{
			break;
		}
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_max_digits < MAX_MAX_DIGITS,
			GPLATES_ASSERTION_SOURCE);
}


const GPlatesFileIO::ExportTemplateFilename::Format::Variation
GPlatesFileIO::ExportTemplateFilename::ReconstructionTimePrintfFormat::VARIATION_TYPE =
		GPlatesFileIO::ExportTemplateFilename::Format::VARIES_WITH_RECONSTRUCTION_TIME_OR_FRAME;


boost::optional<int>
GPlatesFileIO::ExportTemplateFilename::ReconstructionTimePrintfFormat::match_format(
		const QString &rest_of_filename_template)
{
	QRegExp regex = get_full_regular_expression();

	if (regex.indexIn(rest_of_filename_template) == -1)
	{
		// Regular expression didn't match so this is not our format string.
		return boost::none;
	}

	// Get the entire matched string.
	const QString format_string = regex.cap(0);
	return format_string.size();
}


GPlatesFileIO::ExportTemplateFilename::ReconstructionTimePrintfFormat::ReconstructionTimePrintfFormat(
		const QString &format_string) :
	d_format_string(format_string.toStdString())
{
	QRegExp regex = get_integer_regular_expression();

	// See if we need to convert the reconstruction time to the nearest integer before
	// passing to printf-style format specifier.
	d_is_integer_format = (regex.indexIn(format_string) != -1);
}


QString
GPlatesFileIO::ExportTemplateFilename::ReconstructionTimePrintfFormat::expand_format_string(
		std::size_t /*sequence_index*/,
		const double &reconstruction_time,
		const QDateTime &/*date_time*/) const
{
	if (d_is_integer_format)
	{
		// First round the reconstruction time to the nearest integer.
		// Note that reconstruction_time is always positive (need to know this since
		// static_cast truncates towards zero).
		const int reconstruction_time_int = static_cast<int>(reconstruction_time + 0.5);
		return QString().sprintf(d_format_string.c_str(), reconstruction_time_int);
	}
	else
	{
		return QString().sprintf(d_format_string.c_str(), reconstruction_time);
	}
}


const QRegExp &
GPlatesFileIO::ExportTemplateFilename::ReconstructionTimePrintfFormat::get_full_regular_expression()
{
	// QString::sprintf() does not support the length modifiers (eg, h for short, ll for long long)
	// so we'll omit those from the regular expression.
	// Format looks like:
	//    %[flags][width][.precision][length]specifier
	// Where flags is one or more of space, +, -, 0, #.
	// And 'length' has been omitted.
	// And 'specifier' is limited to 'd' and 'f'.
	static QRegExp s_regex("^%[ +-#0]*\\d*(?:\\.\\d+)?[df]");

	return s_regex;
}


const QRegExp &
GPlatesFileIO::ExportTemplateFilename::ReconstructionTimePrintfFormat::get_integer_regular_expression()
{
	// Same as returned by 'get_full_regular_expression()' but only for
	// the '%d' integer specifier.
	static QRegExp s_regex("^%[ +-#0]*\\d*(?:\\.\\d+)?d");

	return s_regex;
}


const GPlatesFileIO::ExportTemplateFilename::Format::Variation
GPlatesFileIO::ExportTemplateFilename::DateTimeFormat::VARIATION_TYPE =
		GPlatesFileIO::ExportTemplateFilename::Format::VARIES_WITH_SEQUENCE_ITERATOR;

const QString GPlatesFileIO::ExportTemplateFilename::DateTimeFormat::HOURS_MINS_SECS_WITH_DASHES_SPECIFIER = "%T";
const QString GPlatesFileIO::ExportTemplateFilename::DateTimeFormat::YEAR_MONTH_DAY_WITH_DASHES_SPECIFIER = "%D";


boost::optional<int>
GPlatesFileIO::ExportTemplateFilename::DateTimeFormat::match_format(
		const QString &rest_of_filename_template)
{
	if (rest_of_filename_template.startsWith(HOURS_MINS_SECS_WITH_DASHES_SPECIFIER))
	{
		return HOURS_MINS_SECS_WITH_DASHES_SPECIFIER.size();
	}

	if (rest_of_filename_template.startsWith(YEAR_MONTH_DAY_WITH_DASHES_SPECIFIER))
	{
		return YEAR_MONTH_DAY_WITH_DASHES_SPECIFIER.size();
	}

	return boost::none;
}


GPlatesFileIO::ExportTemplateFilename::DateTimeFormat::DateTimeFormat(
		const QString &format_string)
{
	if (format_string == HOURS_MINS_SECS_WITH_DASHES_SPECIFIER)
	{
		d_date_time_format = "hh-mm-ss";
	}
	else if (format_string == YEAR_MONTH_DAY_WITH_DASHES_SPECIFIER)
	{
		d_date_time_format = "yyyy-MM-dd";
	}
	else
	{
		throw GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE);
	}
}


QString
GPlatesFileIO::ExportTemplateFilename::DateTimeFormat::expand_format_string(
		std::size_t /*sequence_index*/,
		const double &/*reconstruction_time*/,
		const QDateTime &date_time) const
{
	return date_time.toString(d_date_time_format);
}
