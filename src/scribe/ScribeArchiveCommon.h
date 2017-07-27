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

#ifndef GPLATES_SCRIBE_SCRIBEARCHIVECOMMON_H
#define GPLATES_SCRIBE_SCRIBEARCHIVECOMMON_H

#include <string>
#include <QDataStream>
#include <QString>
#include <QStringList>


namespace GPlatesScribe
{
	/**
	 * Common types and functions shared by archive readers and writers.
	 */
	namespace ArchiveCommon
	{
		/**
		 * The signature string that's written/read to a text archive to ensure it's a GPlates archive.
		 *
		 * NOTE: This should never be modified, otherwise archive will become unreadable.
		 */
		const std::string TEXT_ARCHIVE_SIGNATURE = std::string("GPlatesScribeTextArchive");
		/**
		 * The signature string that's written/read to a binary archive to ensure it's a GPlates archive.
		 *
		 * NOTE: This should never be modified, otherwise archive will become unreadable.
		 */
		const std::string BINARY_ARCHIVE_SIGNATURE = std::string("GPlatesScribeBinaryArchive");
		/**
		 * The signature string that's written/read to a XML archive to ensure it's a GPlates archive.
		 *
		 * NOTE: This should never be modified, otherwise archive will become unreadable.
		 */
		const std::string XML_ARCHIVE_SIGNATURE = std::string("GPlatesScribeXmlArchive");


		/**
		 * Version of the *text* archive format.
		 *
		 * This version gets incremented when modifications are made to the text archive format
		 * that break forward compatibility (when newly created archives cannot be read by older
		 * archive readers built into older versions of GPlates).
		 */
		const unsigned int TEXT_ARCHIVE_FORMAT_VERSION = 0;

		/**
		 * Version of the *binary* archive format.
		 *
		 * This version gets incremented when modifications are made to the text archive format
		 * that break forward compatibility (when newly created archives cannot be read by older
		 * archive readers built into older versions of GPlates).
		 */
		const unsigned int BINARY_ARCHIVE_FORMAT_VERSION = 0;

		/**
		 * Version of the *XML* archive format.
		 *
		 * This version gets incremented when modifications are made to the XML archive format
		 * that break forward compatibility (when newly created archives cannot be read by older
		 * archive readers built into older versions of GPlates).
		 */
		const unsigned int XML_ARCHIVE_FORMAT_VERSION = 0;


		//
		// Integer codes for the primitive types (and composite type).
		//
		const unsigned int SIGNED_INTEGER_CODE = 0;
		const unsigned int UNSIGNED_INTEGER_CODE = 1;
		const unsigned int FLOAT_CODE = 2;
		const unsigned int DOUBLE_CODE = 3;
		const unsigned int STRING_CODE = 4;
		const unsigned int COMPOSITE_CODE = 5;

		//
		// XML element names for the primitive types (and composite type).
		//
		const QString XML_SIGNED_OBJECT_ELEMENT_NAME = QString::fromLatin1("signed");
		const QString XML_UNSIGNED_OBJECT_ELEMENT_NAME = QString::fromLatin1("unsigned");
		const QString XML_FLOAT_OBJECT_ELEMENT_NAME = QString::fromLatin1("float");
		const QString XML_DOUBLE_OBJECT_ELEMENT_NAME = QString::fromLatin1("double");
		const QString XML_STRING_OBJECT_ELEMENT_NAME = QString::fromLatin1("string");
		const QString XML_COMPOSITE_OBJECT_ELEMENT_NAME = QString::fromLatin1("composite");

		// All the above element names in a list.
		const QStringList XML_OBJECT_ELEMENT_NAMES = QStringList()
				<< XML_SIGNED_OBJECT_ELEMENT_NAME
				<< XML_UNSIGNED_OBJECT_ELEMENT_NAME
				<< XML_FLOAT_OBJECT_ELEMENT_NAME
				<< XML_DOUBLE_OBJECT_ELEMENT_NAME
				<< XML_STRING_OBJECT_ELEMENT_NAME
				<< XML_COMPOSITE_OBJECT_ELEMENT_NAME;


		/**
		 * The QDataStream serialisation version used for binary archives.
		 *
		 * NOTE: We are using Qt version 4.4 data streams so the QDataStream::setFloatingPointPrecision()
		 * function is not available (introduced in Qt 4.6).
		 * So the floating-point precision written depends on stream operator called
		 * (ie, whether 'float' or 'double' is written).
		 * We are using Qt 4.4 since that is the current minimum requirement for GPlates.
		 */
		const unsigned int BINARY_ARCHIVE_QT_STREAM_VERSION = QDataStream::Qt_4_4;

		/**
		 * The QDataStream byte order used for binary archives.
		 *
		 * Most hardware is little endian so it's more efficient in general.
		 */
		const QDataStream::ByteOrder BINARY_ARCHIVE_QT_STREAM_BYTE_ORDER = QDataStream::LittleEndian;


		/**
		 * The name of the root XML element containing the serialization stream.
		 */
		const QString XML_ROOT_ELEMENT_NAME = QString::fromLatin1("scribe_serialization");

		/**
		 * The name of the XML attribute containing the archive signature.
		 */
		const QString XML_ARCHIVE_SIGNATURE_ATTRIBUTE_NAME = QString::fromLatin1("scribe_signature");

		/**
		 * The name of the XML attribute containing the XML archive format version.
		 */
		const QString XML_ARCHIVE_FORMAT_VERSION_ATTRIBUTE_NAME = QString::fromLatin1("scribe_xml_archive_format_version");

		/**
		 * The name of the XML attribute containing the archive signature.
		 */
		const QString XML_SCRIBE_VERSION_ATTRIBUTE_NAME = QString::fromLatin1("scribe_version");

		/**
		 * The name of the root XML element containing a transcription stream.
		 */
		const QString XML_TRANSCRIPTION_ELEMENT_NAME = QString::fromLatin1("scribe_transcription");

		/**
		 * The name of the XML element containing the group of object tags.
		 */
		const QString XML_OBJECT_TAG_GROUP_ELEMENT_NAME = QString::fromLatin1("scribe_object_tag_group");

		/**
		 * The name of the XML element containing a single object tag.
		 */
		const QString XML_OBJECT_TAG_ELEMENT_NAME = QString::fromLatin1("tag");

		/**
		 * The name of the XML element containing the group of unique strings.
		 */
		const QString XML_STRING_GROUP_ELEMENT_NAME = QString::fromLatin1("scribe_string_group");

		/**
		 * The name of the XML element containing a single unique string.
		 */
		const QString XML_STRING_ELEMENT_NAME = QString::fromLatin1("string");

		/**
		 * The name of the XML element containing the group of objects.
		 */
		const QString XML_OBJECT_GROUP_ELEMENT_NAME = QString::fromLatin1("scribe_object_group");

		/**
		 * The name of the XML element containing a single object key.
		 */
		const QString XML_OBJECT_KEY_ELEMENT_NAME = QString::fromLatin1("key");

		/**
		 * The name of the XML element containing a single object tag id.
		 */
		const QString XML_OBJECT_TAG_ID_ELEMENT_NAME = QString::fromLatin1("tag_id");

		/**
		 * The name of the XML element containing a single object tag version.
		 */
		const QString XML_OBJECT_TAG_VERSION_ELEMENT_NAME = QString::fromLatin1("tag_version");

		/**
		 * Used to read/write the object id element name and attribute from/to XML archive.
		 */
		const QString XML_OBJECT_ID = QString::fromLatin1("oid");

		/**
		 * Used to read/write positive infinity floating-point value from/to XML archive.
		 */
		const QString XML_POSITIVE_INFINITY_VALUE = QString::fromLatin1("inf");

		/**
		 * Used to read/write positive infinity floating-point value from/to XML archive.
		 */
		const QString XML_NEGATIVE_INFINITY_VALUE = QString::fromLatin1("-inf");

		/**
		 * Used to read/write NaN floating-point value from/to XML archive.
		 */
		const QString XML_NAN_VALUE = QString::fromLatin1("nan");


		/**
		 * Used to read/write positive infinity floating-point value from/to text archive.
		 */
		const std::string TEXT_POSITIVE_INFINITY_VALUE = std::string("inf");

		/**
		 * Used to read/write positive infinity floating-point value from/to text archive.
		 */
		const std::string TEXT_NEGATIVE_INFINITY_VALUE = std::string("-inf");

		/**
		 * Used to read/write NaN floating-point value from/to text archive.
		 */
		const std::string TEXT_NAN_VALUE = std::string("nan");


		/**
		 * Converts a string to an XML element name and optionally checks XML name validity.
		 *
		 * If the string begins with a number or valid punctuation character ('-' or '.') then the
		 * string is prefixed with the '_' character to avoid an invalid XML element name.
		 *
		 * Valid characters include [A-Z], [a-z], [0-9], '_', '-' and '.'.
		 *
		 * Throws @a InvalidXmlElementName exception if using invalid characters for an XML element name.
		 */
		QString
		get_xml_element_name(
				std::string xml_element_name,
				bool validate_all_chars = false);
	}
}

#endif // GPLATES_SCRIBE_SCRIBEARCHIVECOMMON_H
