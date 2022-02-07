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

#include <QByteArray>
#include <QString>

#include "ScribeXmlArchiveReader.h"

#include "Scribe.h"
#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"

#include "maths/Real.h"


const QLocale GPlatesScribe::XmlArchiveReader::C_LOCALE(QLocale::c());


GPlatesScribe::XmlArchiveReader::XmlArchiveReader(
		QXmlStreamReader &xml_stream_reader) :
	d_input_stream(xml_stream_reader),
	d_closed(false)
{
	//
	// Read the archive header.
	//

	// Move to the scribe serialization root XML element.
	read_start_element(ArchiveCommon::XML_ROOT_ELEMENT_NAME, true/*require*/);

	// XML attributes of the root XML element.
	const QXmlStreamAttributes root_element_attributes = d_input_stream.attributes();

	// Read the archive signature string as an attribute.
	const QStringRef archive_signature =
			root_element_attributes.value(ArchiveCommon::XML_ARCHIVE_SIGNATURE_ATTRIBUTE_NAME);

	// Throw exception if archive signature is invalid.
	GPlatesGlobal::Assert<Exceptions::InvalidArchiveSignature>(
			archive_signature ==
				QString::fromLatin1(
						ArchiveCommon::XML_ARCHIVE_SIGNATURE.data(),
						ArchiveCommon::XML_ARCHIVE_SIGNATURE.length()),
			GPLATES_ASSERTION_SOURCE);

	// Read the XML archive format version as an attribute.
	const QString xml_archive_format_version_string =
			root_element_attributes.value(ArchiveCommon::XML_ARCHIVE_FORMAT_VERSION_ATTRIBUTE_NAME).toString();
	bool read_xml_archive_format_version;
	const unsigned int xml_archive_format_version =
			C_LOCALE.toUInt(xml_archive_format_version_string, &read_xml_archive_format_version);

	// Throw exception if the XML archive format version used to write the archive is a future version.
	GPlatesGlobal::Assert<Exceptions::UnsupportedVersion>(
			read_xml_archive_format_version &&
				xml_archive_format_version <= ArchiveCommon::XML_ARCHIVE_FORMAT_VERSION,
			GPLATES_ASSERTION_SOURCE);

	// Read the scribe version as an attribute.
	const QString archive_scribe_version_string =
			root_element_attributes.value(ArchiveCommon::XML_SCRIBE_VERSION_ATTRIBUTE_NAME).toString();
	bool read_archive_scribe_version;
	const unsigned int archive_scribe_version =
			C_LOCALE.toUInt(archive_scribe_version_string, &read_archive_scribe_version);

	// Throw exception if the scribe version used to write the archive is a future version.
	GPlatesGlobal::Assert<Exceptions::UnsupportedVersion>(
			read_archive_scribe_version &&
				archive_scribe_version <= Scribe::get_current_scribe_version(),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesScribe::XmlArchiveReader::close()
{
	if (!d_closed)
	{
		//
		// Write out the end information.
		//

		// End the root serialization XML element.
		read_end_element(ArchiveCommon::XML_ROOT_ELEMENT_NAME, true/*require*/);

		d_closed = true;
	}
}


GPlatesScribe::Transcription::non_null_ptr_type
GPlatesScribe::XmlArchiveReader::read_transcription()
{
	//
	// Read the transcription from the XML archive.
	//
	// The following shows an example XML archive:
	//
	//		<scribe_serialization
	//				scribe_signature="GPlatesScribeArchive"
	//				scribe_xml_archive_format_version="0"
	//				scribe_version="0">
	//			<scribe_transcription>
	//				<scribe_object_tag_group>
	//					<tag>an_object</tag>
	//					<tag>my_int</tag>
	//					<tag>my_double</tag>
	//					<tag>my_string</tag>
	//				</scribe_object_tag_group>
	//
	//				<scribe_string_group>
	//					<string>my_string_value</string>
	//				</scribe_string_group>
	//
	//				<scribe_object_group>
	//					<composite oid="1">
	//						<key>
	//							<tag_id>0</tag_id>
	//							<tag_version>0</tag_version>
	//							<oid>2</oid>
	//						</key>
	//					</composite>
	//					<composite oid="2">
	//						<key>
	//							<tag_id>1</tag_id>
	//							<tag_version>0</tag_version>
	//							<oid>3</oid>
	//						</key>
	//						<key>
	//							<tag_id>2</tag_id>
	//							<tag_version>0</tag_version>
	//							<oid>4</oid>
	//						</key>
	//						<key>
	//							<tag_id>3</tag_id>
	//							<tag_version>0</tag_version>
	//							<oid>5</oid>
	//						</key>
	//					</composite>
	//					<signed oid="3">8</signed>
	//					<double oid="4">10.22</double>
	//					<string oid="5">0</string>
	//				</scribe_object_group>
	//			</scribe_transcription>
	//			
	//			<scribe_transcription>
	//				...
	//			</scribe_transcription>
	//		</scribe_serialization>
	//
	//
	// ...that results from transcribing the following class:
	//
	//
	//		struct Object
	//		{
	//			int my_int;
	//			double my_double;
	//			std::string my_string;
	//		};
	//		
	//		GPlatesScribe::TranscribeResult
	//		transcribe(
	//				GPlatesScribe::GPlatesScribe &scribe,
	//				Object &object,
	//				bool transcribed_construct_data)
	//		{
	//			if (!scribe.transcribe(TRANSCRIBE_SOURCE, object.my_int, "my_int") ||
	//				!scribe.transcribe(TRANSCRIBE_SOURCE, object.my_double, "my_double") ||
	//				!scribe.transcribe(TRANSCRIBE_SOURCE, object.my_string, "my_string"))
	//			{
	//				return scribe.get_transcribe_result();
	//			}
	//			
	//			return GPlatesScribe::TRANSCRIBE_SUCCESS;
	//		}
	//
	//		Object an_object(...);
	//      scribe.transcribe(TRANSCRIBE_SOURCE, an_object, "an_object");
	//


	Transcription::non_null_ptr_type transcription = Transcription::create();

	//
	// Read the start transcription element.
	//

	// Start the transcription XML element.
	read_start_element(ArchiveCommon::XML_TRANSCRIPTION_ELEMENT_NAME, true/*require*/);

	//
	// Read the object tags.
	//

	read_start_element(ArchiveCommon::XML_OBJECT_TAG_GROUP_ELEMENT_NAME, true/*require*/);

	while (read_start_element(ArchiveCommon::XML_OBJECT_TAG_ELEMENT_NAME))
	{
		transcription->add_object_tag_name(read_string());
		read_end_element(ArchiveCommon::XML_OBJECT_TAG_ELEMENT_NAME, true/*require*/);
	}

	//
	// Read the unique strings.
	//

	read_start_element(ArchiveCommon::XML_STRING_GROUP_ELEMENT_NAME, true/*require*/);

	while (read_start_element(ArchiveCommon::XML_STRING_ELEMENT_NAME))
	{
		transcription->add_unique_string_object(read_string());
		read_end_element(ArchiveCommon::XML_STRING_ELEMENT_NAME, true/*require*/);
	}

	//
	// Read the objects.
	//

	read_start_element(ArchiveCommon::XML_OBJECT_GROUP_ELEMENT_NAME, true/*require*/);

	while (read_start_element(ArchiveCommon::XML_OBJECT_ELEMENT_NAMES))
	{
		const Transcription::object_id_type object_id = read_object_id_attribute();

		const QString object_element_name = d_input_stream.name().toString();
		if (object_element_name == ArchiveCommon::XML_SIGNED_OBJECT_ELEMENT_NAME)
		{
			transcription->add_signed_integer(object_id, read_signed());
			read_end_element(ArchiveCommon::XML_SIGNED_OBJECT_ELEMENT_NAME, true/*require*/);
		}
		else if (object_element_name == ArchiveCommon::XML_UNSIGNED_OBJECT_ELEMENT_NAME)
		{
			transcription->add_unsigned_integer(object_id, read_unsigned());
			read_end_element(ArchiveCommon::XML_UNSIGNED_OBJECT_ELEMENT_NAME, true/*require*/);
		}
		else if (object_element_name == ArchiveCommon::XML_FLOAT_OBJECT_ELEMENT_NAME)
		{
			transcription->add_float(object_id, read_float());
			read_end_element(ArchiveCommon::XML_FLOAT_OBJECT_ELEMENT_NAME, true/*require*/);
		}
		else if (object_element_name == ArchiveCommon::XML_DOUBLE_OBJECT_ELEMENT_NAME)
		{
			transcription->add_double(object_id, read_double());
			read_end_element(ArchiveCommon::XML_DOUBLE_OBJECT_ELEMENT_NAME, true/*require*/);
		}
		else if (object_element_name == ArchiveCommon::XML_STRING_OBJECT_ELEMENT_NAME)
		{
			transcription->add_string_object(object_id, read_unsigned());
			read_end_element(ArchiveCommon::XML_STRING_OBJECT_ELEMENT_NAME, true/*require*/);
		}
		else if (object_element_name == ArchiveCommon::XML_COMPOSITE_OBJECT_ELEMENT_NAME)
		{
			transcription->add_composite_object(object_id);
			read_composite(transcription->get_composite_object(object_id));
		}
		else
		{
			GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
					false,
					GPLATES_ASSERTION_SOURCE,
					"Archive stream error detected reading object type.");
		}
	}

	//
	// Read the end of transcription element.
	//

	// End the transcription XML element.
	read_end_element(ArchiveCommon::XML_TRANSCRIPTION_ELEMENT_NAME, true/*require*/);

	return transcription;
}


void
GPlatesScribe::XmlArchiveReader::read_composite(
		Transcription::CompositeObject &composite_object)
{
	while (read_start_element(ArchiveCommon::XML_OBJECT_KEY_ELEMENT_NAME))
	{
		// Read the current child key.

		read_start_element(ArchiveCommon::XML_OBJECT_TAG_ID_ELEMENT_NAME, true/*required*/);
		const Transcription::object_tag_name_id_type object_tag_name_id = read_unsigned();
		read_end_element(ArchiveCommon::XML_OBJECT_TAG_ID_ELEMENT_NAME, true/*required*/);

		read_start_element(ArchiveCommon::XML_OBJECT_TAG_VERSION_ELEMENT_NAME, true/*required*/);
		const Transcription::object_tag_version_type object_tag_version = read_unsigned();
		read_end_element(ArchiveCommon::XML_OBJECT_TAG_VERSION_ELEMENT_NAME, true/*required*/);

		const Transcription::object_key_type object_key(object_tag_name_id, object_tag_version);

		for (unsigned int child_index = 0; read_start_element(ArchiveCommon::XML_OBJECT_ID); ++child_index)
		{
			const Transcription::object_id_type object_id = read_unsigned();
			composite_object.set_child(object_key, object_id, child_index);
			read_end_element(ArchiveCommon::XML_OBJECT_ID, true/*required*/);
		}
	}
}


int
GPlatesScribe::XmlArchiveReader::read_signed()
{
	read_next_token();

	// We should be at text/characters.
	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_input_stream.isCharacters(),
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected before reading signed.");

	// Read the current XML element.
	bool read_object_success;
	const int value = C_LOCALE.toInt(
			d_input_stream.text().toString(),
			&read_object_success);

	// The read should have been successful.
	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			read_object_success,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected while reading signed.");

	return value;
}


unsigned int
GPlatesScribe::XmlArchiveReader::read_unsigned()
{
	read_next_token();

	// We should be at text/characters.
	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_input_stream.isCharacters(),
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected before reading unsigned.");

	// Read the current XML element.
	bool read_object_success;
	const unsigned int value = C_LOCALE.toUInt(
			d_input_stream.text().toString(),
			&read_object_success);

	// The read should have been successful.
	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			read_object_success,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected while reading unsigned.");

	return value;
}


float
GPlatesScribe::XmlArchiveReader::read_float()
{
	read_next_token();

	// We should be at text/characters.
	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_input_stream.isCharacters(),
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected before reading float.");

	// Read the current XML element.
	const QString float_string = d_input_stream.text().toString().trimmed();

	float value;
	if (float_string == ArchiveCommon::XML_POSITIVE_INFINITY_VALUE)
	{
		value = GPlatesMaths::positive_infinity<float>();
	}
	else if (float_string == ArchiveCommon::XML_NEGATIVE_INFINITY_VALUE)
	{
		value = GPlatesMaths::negative_infinity<float>();
	}
	else if (float_string == ArchiveCommon::XML_NAN_VALUE)
	{
		value = GPlatesMaths::quiet_nan<float>();
	}
	else // finite ....
	{
		bool read_object_success;
		value = C_LOCALE.toFloat(float_string, &read_object_success);

		// The read should have been successful.
		GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
				read_object_success,
				GPLATES_ASSERTION_SOURCE,
				"Archive stream error detected while reading float.");
	}

	return value;
}


double
GPlatesScribe::XmlArchiveReader::read_double()
{
	read_next_token();

	// We should be at text/characters.
	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_input_stream.isCharacters(),
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected before reading double.");

	// Read the current XML element.
	const QString double_string = d_input_stream.text().toString().trimmed();

	double value;
	if (double_string == ArchiveCommon::XML_POSITIVE_INFINITY_VALUE)
	{
		value = GPlatesMaths::positive_infinity<double>();
	}
	else if (double_string == ArchiveCommon::XML_NEGATIVE_INFINITY_VALUE)
	{
		value = GPlatesMaths::negative_infinity<double>();
	}
	else if (double_string == ArchiveCommon::XML_NAN_VALUE)
	{
		value = GPlatesMaths::quiet_nan<double>();
	}
	else // finite ....
	{
		bool read_object_success;
		value = C_LOCALE.toDouble(double_string, &read_object_success);

		// The read should have been successful.
		GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
				read_object_success,
				GPLATES_ASSERTION_SOURCE,
				"Archive stream error detected while reading double.");
	}

	return value;
}


std::string
GPlatesScribe::XmlArchiveReader::read_string()
{
	read_next_token();

	// We should be at text/characters.
	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_input_stream.isCharacters(),
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected before reading string.");

	// Read the current XML element.
	const QString object_qstring = d_input_stream.text().toString();

	// Convert the QString to a string.
	const QByteArray object_byte_array = object_qstring.toLatin1();
	
	return std::string(object_byte_array.constData(), object_byte_array.size());
}


GPlatesScribe::Transcription::object_id_type
GPlatesScribe::XmlArchiveReader::read_object_id_attribute()
{
	// Read object id as an integer attribute of the current XML element.
	bool read_success;
	const Transcription::object_id_type object_id = C_LOCALE.toUInt(
			d_input_stream.attributes().value(ArchiveCommon::XML_OBJECT_ID).toString(),
			&read_success);

	// The read should have been successful.
	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			read_success,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected while reading object id attribute.");

	return object_id;
}


bool
GPlatesScribe::XmlArchiveReader::read_start_element(
		const QString &element_name,
		bool require)
{
	while (!d_input_stream.atEnd())
	{
		read_next_token();

		if (d_input_stream.isStartElement() ||
			d_input_stream.isEndElement())
		{
			break;
		}
	}

	// If successfully found start element with specified name.
	if (d_input_stream.isStartElement() &&
		d_input_stream.name() == element_name)
	{
		return true;
	}

	if (require)
	{
		GPlatesGlobal::Assert<GPlatesScribe::Exceptions::UnexpectedXmlElementName>(
				false,
				GPLATES_ASSERTION_SOURCE,
				element_name,
				true/*is_start_element*/);
	}

	return false;
}


bool
GPlatesScribe::XmlArchiveReader::read_start_element(
		const QStringList &element_names,
		bool require)
{
	while (!d_input_stream.atEnd())
	{
		read_next_token();

		if (d_input_stream.isStartElement() ||
			d_input_stream.isEndElement())
		{
			break;
		}
	}

	// If successfully found start element with one of the specified names.
	if (d_input_stream.isStartElement() &&
		element_names.contains(d_input_stream.name().toString()))
	{
		return true;
	}

	if (require)
	{
		GPlatesGlobal::Assert<GPlatesScribe::Exceptions::UnexpectedXmlElementName>(
				false,
				GPLATES_ASSERTION_SOURCE,
				element_names,
				true/*is_start_element*/);
	}

	return false;
}


bool
GPlatesScribe::XmlArchiveReader::read_end_element(
		const QString &element_name,
		bool require)
{
	while (!d_input_stream.atEnd())
	{
		read_next_token();

		if (d_input_stream.isStartElement() ||
			d_input_stream.isEndElement())
		{
			break;
		}
	}

	// If successfully found end element with specified name.
	if (d_input_stream.isEndElement() &&
		d_input_stream.name() == element_name)
	{
		return true;
	}

	if (require)
	{
		GPlatesGlobal::Assert<GPlatesScribe::Exceptions::UnexpectedXmlElementName>(
				false,
				GPLATES_ASSERTION_SOURCE,
				element_name,
				false/*is_start_element*/);
	}

	return false;
}


bool
GPlatesScribe::XmlArchiveReader::read_end_element(
		const QStringList &element_names,
		bool require)
{
	while (!d_input_stream.atEnd())
	{
		read_next_token();

		if (d_input_stream.isStartElement() ||
			d_input_stream.isEndElement())
		{
			break;
		}
	}

	// If successfully found end element with one of the specified names.
	if (d_input_stream.isEndElement() &&
		element_names.contains(d_input_stream.name().toString()))
	{
		return true;
	}

	if (require)
	{
		GPlatesGlobal::Assert<GPlatesScribe::Exceptions::UnexpectedXmlElementName>(
				false,
				GPLATES_ASSERTION_SOURCE,
				element_names,
				false/*is_start_element*/);
	}

	return false;
}


void
GPlatesScribe::XmlArchiveReader::read_next_token()
{
	d_input_stream.readNext();

	if (d_input_stream.hasError())
	{
		GPlatesGlobal::Assert<Exceptions::XmlStreamParseError>(
				false,
				GPLATES_ASSERTION_SOURCE,
				d_input_stream.errorString());
	}
}
