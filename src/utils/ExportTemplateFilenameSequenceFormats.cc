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

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


bool
GPlatesUtils::ExportTemplateFilename::PercentCharacterFormat::match_format(
		const QString &rest_of_filename_template,
		int &length_format_string)
{
	if (rest_of_filename_template.startsWith("%%"))
	{
		length_format_string = 2;

		return true;
	}

	return false;
}


bool
GPlatesUtils::ExportTemplateFilename::ReconstructionAnchorPlateIdFormat::match_format(
		const QString &rest_of_filename_template,
		int &length_format_string)
{
	if (rest_of_filename_template.startsWith("%A"))
	{
		length_format_string = 2;

		return true;
	}

	return false;
}


QString
GPlatesUtils::ExportTemplateFilename::ReconstructionAnchorPlateIdFormat::expand_format_string(
		std::size_t /*sequence_index*/,
		const double &/*reconstruction_time*/,
		const QDateTime &/*date_time*/) const
{
	return QString("%1").arg(d_reconstruction_anchor_plate_id);
}


bool
GPlatesUtils::ExportTemplateFilename::FrameNumberFormat::match_format(
		const QString &rest_of_filename_template,
		int &length_format_string)
{
	if (rest_of_filename_template.startsWith("%n") ||
			rest_of_filename_template.startsWith("%u"))
	{
		length_format_string = 2;
		return true;
	}

	return false;
}


GPlatesUtils::ExportTemplateFilename::FrameNumberFormat::FrameNumberFormat(
		const QString &format_string,
		std::size_t sequence_size)
{
	d_use_frame_number = (format_string == "%n");

	calc_max_digits(sequence_size);
}


QString
GPlatesUtils::ExportTemplateFilename::FrameNumberFormat::expand_format_string(
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
GPlatesUtils::ExportTemplateFilename::FrameNumberFormat::calc_max_digits(
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

	GPlatesGlobal::Assert(d_max_digits < MAX_MAX_DIGITS,
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));
}


bool
GPlatesUtils::ExportTemplateFilename::ReconstructionTimePrintfFormat::match_format(
		const QString &rest_of_filename_template,
		int &length_format_string)
{
	QRegExp regex = get_full_regular_expression();

	if (regex.indexIn(rest_of_filename_template) == -1)
	{
		// Regular expression didn't match so this is not our format string.
		return false;
	}

	// Get the entire matched string.
	const QString format_string = regex.cap(0);
	length_format_string = format_string.size();

	return true;
}


GPlatesUtils::ExportTemplateFilename::ReconstructionTimePrintfFormat::ReconstructionTimePrintfFormat(
		const QString &format_string) :
	d_format_string(format_string.toStdString())
{
	QRegExp regex = get_integer_regular_expression();

	// See if we need to convert the reconstruction time to the nearest integer before
	// passing to printf-style format specifier.
	d_is_integer_format = (regex.indexIn(format_string) != -1);
}


QString
GPlatesUtils::ExportTemplateFilename::ReconstructionTimePrintfFormat::expand_format_string(
		std::size_t /*sequence_index*/,
		const double &reconstruction_time,
		const QDateTime &/*date_time*/) const
{
	if (d_is_integer_format)
	{
		// First round the reconstruction time to the nearest integer.
		// Note that reconstruction_time is always positive (need to know this since
		// static_cast truncates towards zero).
		const int reconstruction_time_int = static_cast<const int>(reconstruction_time + 0.5);
		return QString().sprintf(d_format_string.c_str(), reconstruction_time_int);
	}
	else
	{
		return QString().sprintf(d_format_string.c_str(), reconstruction_time);
	}
}


const QRegExp &
GPlatesUtils::ExportTemplateFilename::ReconstructionTimePrintfFormat::get_full_regular_expression()
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
GPlatesUtils::ExportTemplateFilename::ReconstructionTimePrintfFormat::get_integer_regular_expression()
{
	// Same as returned by 'get_full_regular_expression()' but only for
	// the '%d' integer specifier.
	static QRegExp s_regex("^%[ +-#0]*\\d*(?:\\.\\d+)?d");

	return s_regex;
}



const QString GPlatesUtils::ExportTemplateFilename::DateTimeFormat::HOURS_MINS_SECS_WITH_DASHES_SPECIFIER = "%T";
const QString GPlatesUtils::ExportTemplateFilename::DateTimeFormat::HOURS_MINS_SECS_WITH_COLONS_SPECIFIER = "%:";
const QString GPlatesUtils::ExportTemplateFilename::DateTimeFormat::YEAR_MONTH_DAY_WITH_DASHES_SPECIFIER = "%D";


bool
GPlatesUtils::ExportTemplateFilename::DateTimeFormat::match_format(
		const QString &rest_of_filename_template,
		int &length_format_string)
{
	if (rest_of_filename_template.startsWith(HOURS_MINS_SECS_WITH_DASHES_SPECIFIER))
	{
		length_format_string = HOURS_MINS_SECS_WITH_DASHES_SPECIFIER.size();
		return true;
	}

	if (rest_of_filename_template.startsWith(HOURS_MINS_SECS_WITH_COLONS_SPECIFIER))
	{
		length_format_string = HOURS_MINS_SECS_WITH_COLONS_SPECIFIER.size();
		return true;
	}

	if (rest_of_filename_template.startsWith(YEAR_MONTH_DAY_WITH_DASHES_SPECIFIER))
	{
		length_format_string = YEAR_MONTH_DAY_WITH_DASHES_SPECIFIER.size();
		return true;
	}

	return false;
}


GPlatesUtils::ExportTemplateFilename::DateTimeFormat::DateTimeFormat(
		const QString &format_string)
{
	if (format_string == HOURS_MINS_SECS_WITH_DASHES_SPECIFIER)
	{
		d_date_time_format = "hh-mm-ss";
	}
	else if (format_string == HOURS_MINS_SECS_WITH_COLONS_SPECIFIER)
	{
		d_date_time_format = "hh:mm:ss";
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
GPlatesUtils::ExportTemplateFilename::DateTimeFormat::expand_format_string(
		std::size_t /*sequence_index*/,
		const double &/*reconstruction_time*/,
		const QDateTime &date_time) const
{
	return date_time.toString(d_date_time_format);
}
