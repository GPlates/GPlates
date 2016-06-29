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

#include "TranscribeQt.h"

#include "Scribe.h"
#include "ScribeExceptions.h"


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
	 * The QDataStream serialisation version used for streaming QVariant.
	 *
	 * NOTE: We are using Qt version 4.4 data streams so the QDataStream::setFloatingPointPrecision()
	 * function is not available (introduced in Qt 4.6).
	 * So the floating-point precision written depends on stream operator called
	 * (ie, whether 'float' or 'double' is written).
	 * We are using Qt 4.4 since that is the current minimum requirement for GPlates.
	 *
	 * WARNING: Changing this version may break backward/forward compatibility of projects/sessions.
	 */
	const unsigned int QVARIANT_QT_STREAM_VERSION = QDataStream::Qt_4_4;

	/**
	 * The QDataStream byte order used for streaming QVariant.
	 *
	 * Most hardware is little endian so it's more efficient in general.
	 *
	 * WARNING: Changing this version will break backward/forward compatibility of projects/sessions.
	 */
	const QDataStream::ByteOrder QVARIANT_QT_STREAM_BYTE_ORDER = QDataStream::LittleEndian;
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
	QString qdatetime_string;

	if (scribe.is_saving())
	{
		qdatetime_string = qdatetime_object.toString(C_LOCALE.dateTimeFormat());
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, qdatetime_string, "string"))
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
		data_array_writer.setVersion(QVARIANT_QT_STREAM_VERSION);
		data_array_writer.setByteOrder(QVARIANT_QT_STREAM_BYTE_ORDER);

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
		data_array_reader.setVersion(QVARIANT_QT_STREAM_VERSION);
		data_array_reader.setByteOrder(QVARIANT_QT_STREAM_BYTE_ORDER);

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
		data_array_reader.setVersion(QVARIANT_QT_STREAM_VERSION);
		data_array_reader.setByteOrder(QVARIANT_QT_STREAM_BYTE_ORDER);

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
