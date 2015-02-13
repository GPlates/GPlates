/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <string>
#include <QByteArray>
#include <QLocale>

#include "TranscribeQt.h"

#include "Scribe.h"


namespace
{
	//
	// Use the "C" locale to convert QDateTime to and from the archive.
	//
	// This ensures that saving using one locale and loading using another
	// will not cause synchronization problems.
	//
	const QLocale C_LOCALE(QLocale::c());
}


GPlatesScribe::TranscribeResult
GPlatesScribe::transcribe(
		Scribe &scribe,
		QByteArray &qbytearray_object,
		bool transcribed_construct_data)
{
	std::string base64;

	if (scribe.is_saving())
	{
		const QByteArray base64_array = qbytearray_object.toBase64();
		// Need to specify the array size in case array has embedded zeros,
		// otherwise the first zero found will terminate the string assignment.
		base64.assign(base64_array.data(), base64_array.size());
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, base64, "base64", DONT_TRACK))
	{
		return scribe.get_transcribe_result();
	}

	if (scribe.is_loading())
	{
		const QByteArray base64_array(base64.data(), base64.size());
		// Need to specify the string size in case string has embedded zeros,
		// otherwise the first zero found will terminate the string assignment.
		qbytearray_object = QByteArray::fromBase64(base64_array);
	}

	return TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesScribe::transcribe(
		Scribe &scribe,
		QString &qstring_object,
		bool transcribed_construct_data)
{
	std::string utf8;

	if (scribe.is_saving())
	{
		const QByteArray utf8_array = qstring_object.toUtf8();
		utf8.assign(utf8_array.data(), utf8_array.size());
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, utf8, "utf8", DONT_TRACK))
	{
		return scribe.get_transcribe_result();
	}

	if (scribe.is_loading())
	{
		qstring_object = QString::fromUtf8(utf8.data(), utf8.size());
	}

	return TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesScribe::transcribe(
		Scribe &scribe,
		QDateTime &qdatetime_object,
		bool transcribed_construct_data)
{
	QString qdatetime_string;

	if (scribe.is_saving())
	{
		qdatetime_string = qdatetime_object.toString(C_LOCALE.dateTimeFormat());
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, qdatetime_string, "string", DONT_TRACK))
	{
		return scribe.get_transcribe_result();
	}

	if (scribe.is_loading())
	{
		// Get the QDateTime from the encoded string.
		qdatetime_object = QDateTime::fromString(qdatetime_string, C_LOCALE.dateTimeFormat());

		// The QDateTime decode should have been successful.
		if (!qdatetime_object.isValid())
		{
			return TRANSCRIBE_INCOMPATIBLE;
		}
	}

	return TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesScribe::transcribe(
		Scribe &scribe,
		QStringList &string_list_object,
		bool transcribed_construct_data)
{
	return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, string_list_object);
}
