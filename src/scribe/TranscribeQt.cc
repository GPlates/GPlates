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
#include <QDataStream>
#include <QLocale>
#include <QtGlobal>

#include "TranscribeQt.h"

#include "Scribe.h"
#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


namespace
{
	//
	// Use the "C" locale to convert QDateTime to and from the archive.
	//
	// This ensures that saving using one locale and loading using another
	// will not cause synchronization problems.
	//
	const QLocale C_LOCALE(QLocale::c());

	/**
	 * The QDataStream serialisation version used for streaming QVariant and QDateTime.
	 *
	 * NOTE: We are using Qt version 4.4 data streams so the QDataStream::setFloatingPointPrecision()
	 * function is not available (introduced in Qt 4.6).
	 * So the floating-point precision written depends on stream operator called
	 * (ie, whether 'float' or 'double' is written).
	 * We are using Qt 4.4 since that is the current minimum requirement for GPlates.
	 *
	 * WARNING: Changing this version may break backward/forward compatibility of projects/sessions.
	 */
	const unsigned int TRANSCRIBE_QT_STREAM_VERSION = QDataStream::Qt_4_4;

	/**
	 * The QDataStream byte order used for streaming QVariant and QDateTime.
	 *
	 * Most hardware is little endian so it's more efficient in general.
	 *
	 * WARNING: Changing this version will break backward/forward compatibility of projects/sessions.
	 */
	const QDataStream::ByteOrder TRANSCRIBE_QT_STREAM_BYTE_ORDER = QDataStream::LittleEndian;
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

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, base64, "base64"))
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
		QDateTime &qdatetime_object,
		bool transcribed_construct_data)
{
	// Starting with GPlates 2.1 we transcribe QDateTime by streaming to/from a QDataStream since
	// this avoids all locale issues. GPlates 2.0 transcribed by converting QDateTime to a localised QString.
	// However it only used the "C" locale (en_US) for the string *format* whereas the QDateTime
	// object itself was still converted using the system locale.
	//
	// So we now have two versions:
	// * version 1 - GPlates 2.1 (and above), and
	// * version 0 - GPlates 2.0 (and below).
	//
	// When saving we write out both version 0 and 1 tags.
	// When loading we attempt to load the version 1 tag, if that fails we then load version 0.
	// This provides compatibility with GPlates 2.0 (and below) in that GPlates 2.0 can load a
	// project/session we save (because we save version 0) and we can load projects/sessions it
	// saves (because we can load version 0).
	static const ObjectTag VERSION_0_OBJECT_TAG("string", 0/*tag_version*/);
	static const ObjectTag VERSION_1_OBJECT_TAG("string", 1/*tag_version*/);

	// This is the same as 'C_LOCALE.dateTimeFormat()' except with the " t" timezone part removed from the end.
	// GPlates 2.0 (and below) used 'QDateTime::toString()' and 'QDateTime::fromString()' which, in Qt 4.x,
	// don't support the timezone format 't' (Qt 5 supports it though) but QLocale does support it (in Qt 4.x).
	// Since we now use QLocale for version 0 transcribing we don't want it to convert the 't' format
	// otherwise GPlates 2.0 won't work (because it expects the 't' to be there and then ignores it).
	// So we only include the 't' *after* we've converted our QDateTime object to a string when saving,
	// and when loading we first remove the 't' *before* converting the string back to a QDateTime object.
	static const QString VERSION_0_DATE_TIME_FORMAT("dddd, d MMMM yyyy HH:mm:ss");

	if (scribe.is_saving())
	{
		//
		// Stream the QDateTime to an array using QDataStream.
		//
		QByteArray qdatetime_array;
		QDataStream qdatetime_array_writer(&qdatetime_array, QIODevice::WriteOnly);
		qdatetime_array_writer.setVersion(TRANSCRIBE_QT_STREAM_VERSION);
		qdatetime_array_writer.setByteOrder(TRANSCRIBE_QT_STREAM_BYTE_ORDER);

		// Convert to UTC since, prior to QDataStream version 13 (introduced in QT 5 - note we are using
		// version 10 here), the conversion to UTC is not done internally when streaming - which means
		// serialising in one time zone and deserialising in another is a problem when the QDateTime
		// object has a local time spec (since the local time zones might be different when saving and loading).
		//
		// Note: We save a QVariant so we can test (on loading) that the correct object type (QDateTime) was loaded.
		if (qdatetime_object.isValid())
		{
			qdatetime_array_writer << QVariant(qdatetime_object.toUTC());
		}
		else
		{
			// Just stream the invalid object - this is what Qt5 does inside its '<<' operator.
			qdatetime_array_writer << QVariant(qdatetime_object);
		}
		// Also serialise whether the time spec is local or not so we can return to local time spec
		// on deserialising (if needed).
		// Note: We're ignoring the other time specs - the deserialised QDateTime will either be UTC or local.
		qdatetime_array_writer << static_cast<bool>(qdatetime_object.timeSpec() == Qt::LocalTime);

		// This assertion should never fail - QDataStream should never fail to write to a QByteArray.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				qdatetime_array_writer.status() == QDataStream::Ok,
				GPLATES_ASSERTION_SOURCE,
				"Failed to stream QDateTime into QByteArray.");

		scribe.save(TRANSCRIBE_SOURCE, qdatetime_array, VERSION_1_OBJECT_TAG);

		//
		// For compatibility with earlier versions (GPlates 2.0 and prior) write QDateTime as a string.
		//
		// See comment above 'VERSION_0_DATE_TIME_FORMAT' for explanation of the 't' manipulation.
		const QString qdatetime_string = C_LOCALE.toString(qdatetime_object, VERSION_0_DATE_TIME_FORMAT);
		const QString qdatetime_string_with_timezone = qdatetime_string + " t";
		scribe.save(TRANSCRIBE_SOURCE, qdatetime_string_with_timezone, VERSION_0_OBJECT_TAG);
	}

	if (scribe.is_loading())
	{
		//
		// First attempt to load version 1, if that fails then load version 0.
		//
		QByteArray qdatetime_array;
		if (scribe.transcribe(TRANSCRIBE_SOURCE, qdatetime_array, VERSION_1_OBJECT_TAG))
		{
			//
			// Stream QDateTime from an array using QDataStream.
			//
			QDataStream qdatetime_array_reader(&qdatetime_array, QIODevice::ReadOnly);
			qdatetime_array_reader.setVersion(TRANSCRIBE_QT_STREAM_VERSION);
			qdatetime_array_reader.setByteOrder(TRANSCRIBE_QT_STREAM_BYTE_ORDER);

			// Read the UTC QDateTime and original time spec.
			// Note: We use a QVariant so we can test that the correct object type (QDateTime) was loaded.
			QVariant qdatetime_variant;
			qdatetime_array_reader >> qdatetime_variant;
			bool is_local_time_spec;
			qdatetime_array_reader >> is_local_time_spec;

			// If unable to stream QDateTime object then QByteArray must represent some other type of object.
			//
			// Note that we don't also test whether the QDateTime object itself is valid (upon successful streaming)
			// since it's possible that an invalid QDateTime was saved in the first place.
			if (qdatetime_array_reader.status() != QDataStream::Ok ||
				qdatetime_variant.type() != int(QMetaType::QDateTime))
			{
				return TRANSCRIBE_INCOMPATIBLE;
			}

			qdatetime_object = qdatetime_variant.toDateTime();

			// Convert from UTC to local time spec if the system that saved the project/session used a local time spec.
			// Note that the local timezones on the save and load systems might be different though.
			if (is_local_time_spec &&
				// Avoid conversion if invalid object - this is what Qt5 does inside its '<<' operator...
				qdatetime_object.isValid())
			{
				qdatetime_object = qdatetime_object.toLocalTime();
			}
		}
		else
		{
			QString qdatetime_string_with_timezone;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, qdatetime_string_with_timezone, VERSION_0_OBJECT_TAG))
			{
				// See comment above 'VERSION_0_DATE_TIME_FORMAT' for explanation of the 't' manipulation.
				QString qdatetime_string = qdatetime_string_with_timezone;
				if (qdatetime_string.endsWith(" t"))
				{
					qdatetime_string.chop(2);
				}

				// Get the QDateTime from the encoded string.
				qdatetime_object = C_LOCALE.toDateTime(qdatetime_string, VERSION_0_DATE_TIME_FORMAT);

				// If QDateTime decode was not successful then try the old GPlates 2.0 decode
				// (which incorrectly used the system locale).
				if (!qdatetime_object.isValid())
				{
					// GPlates 2.0 incorrectly saved using the system locale (instead of "C" locale - which is always en_US).
					// So, having failed above, we'll attempt to try again with the current system locale.
					// For example, this helps if a user with a Chinese locale saved using GPlates 2.0 and loads using 2.1 (or later).
					qdatetime_object = QDateTime::fromString(qdatetime_string, VERSION_0_DATE_TIME_FORMAT);
					if (!qdatetime_object.isValid())
					{
						return TRANSCRIBE_INCOMPATIBLE;
					}
				}
			}
			else
			{
				return scribe.get_transcribe_result();
			}
		}
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

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, utf8, "utf8"))
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
		QVariant &qvariant_object,
		bool transcribed_construct_data)
{
	//
	// Since QVariant does not expose the actual class type or 'std::type_info' of the object
	// contained in the QVariant, we rely on streaming it to/from a QByteArray using a QDataStream.
	// We then transcribe the QByteArray. This allows us to transcribe QVariant for its builtin
	// types as well as user types registered with QMetaType. Note that registration with QMetaType
	// must be both 'qRegisterMetaType()' and 'qRegisterMetaTypeStreamOperators()'.
	// And the QDataStream '<<' and '>>' operators must be implemented for the user type.
	//

	QByteArray data_array;

	if (scribe.is_saving())
	{
		// Used to write to the QByteArray.
		QDataStream data_array_writer(&data_array, QIODevice::WriteOnly);
		data_array_writer.setVersion(TRANSCRIBE_QT_STREAM_VERSION);
		data_array_writer.setByteOrder(TRANSCRIBE_QT_STREAM_BYTE_ORDER);

		// Save the QVariant to the QByteArray.
		data_array_writer << qvariant_object;

		// This assertion will probably never fail.
		// More likely to get a compile error due to 'Q_DECLARE_METATYPE()' macro missing for the type.
		//
		// If this assertion is triggered then it means:
		//   * The stored object's type (or a type it depends on) was not registered using
		//     'qRegisterMetaType()' and 'qRegisterMetaTypeStreamOperators()'.
		//
		GPlatesGlobal::Assert<Exceptions::UnregisteredQVariantMetaType>(
				data_array_writer.status() == QDataStream::Ok,
				GPLATES_ASSERTION_SOURCE,
				qvariant_object);

		//
		// We want to ensure that both 'qRegisterMetaType()' and 'qRegisterMetaTypeStreamOperators()'
		// have been called by the client. Although we're in the 'save' path, we want to ensure that
		// when the QVariant is later loaded (in the 'load' path) that is will not fail because
		// it hasn't been registered. They are not used on the save path but are needed on the load path.
		// It's important to trigger this error on the save path since it's better to fail on the
		// save path (with an exception) and have the programmer fix the problem than it is to fail
		// on the load path when it's too late to fix the problem.
		//
		// The best way to test this is just to do a test load of the QVariant just saved.
		// It seems using 'QMetaType::isRegistered()' is not sufficient to test this since it
		// only seems to check that 'Q_DECLARE_METATYPE()' is not missing, but we'll get a compile
		// error if that is missing anyway.
		//

		// Used to read from the QByteArray.
		QDataStream data_array_reader(&data_array, QIODevice::ReadOnly);
		data_array_reader.setVersion(TRANSCRIBE_QT_STREAM_VERSION);
		data_array_reader.setByteOrder(TRANSCRIBE_QT_STREAM_BYTE_ORDER);

		// Load the QVariant from the QByteArray into a temporary test QVariant.
		QVariant test_save;
		data_array_reader >> test_save;

		// Throw exception if the stored object's type has not been export registered.
		//
		// If this assertion is triggered then it means:
		//   * The stored object's type (or a type it depends on) was not registered using
		//     'qRegisterMetaType()' and 'qRegisterMetaTypeStreamOperators()'.
		//
		GPlatesGlobal::Assert<Exceptions::UnregisteredQVariantMetaType>(
				data_array_reader.status() == QDataStream::Ok,
				GPLATES_ASSERTION_SOURCE,
				qvariant_object);
	}

	// Transcribe the QByteArray containing the streamed QVariant.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, data_array, "qvariant_data"))
	{
		return scribe.get_transcribe_result();
	}

	if (scribe.is_loading())
	{
		// Used to read from the QByteArray.
		QDataStream data_array_reader(&data_array, QIODevice::ReadOnly);
		data_array_reader.setVersion(TRANSCRIBE_QT_STREAM_VERSION);
		data_array_reader.setByteOrder(TRANSCRIBE_QT_STREAM_BYTE_ORDER);

		// Load the QVariant from the QByteArray.
		data_array_reader >> qvariant_object;

		if (data_array_reader.status() != QDataStream::Ok)
		{
			// It's possible that, for the object type inside the QVariant,
			// 'qRegisterMetaType()' and 'qRegisterMetaTypeStreamOperators()' have not been called.
			//
			// If the object type has not been registered with Qt then it means either:
			//   * the archive was created by a future GPlates with an object type we don't know about, or
			//   * the archive was created by an old GPlates with an object type we have since removed (no longer register).
			//
			return TRANSCRIBE_UNKNOWN_TYPE;
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
