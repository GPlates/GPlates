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

#include <limits>
#include <QString>

#include "ScribeXmlArchiveWriter.h"

#include "Scribe.h"
#include "ScribeExceptions.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


const QLocale GPlatesScribe::XmlArchiveWriter::C_LOCALE(QLocale::c());


GPlatesScribe::XmlArchiveWriter::XmlArchiveWriter(
		QXmlStreamWriter &xml_stream_writer) :
	d_output_stream(xml_stream_writer),
	d_closed(false)
{
	//
	// Set up the archive stream.
	//

	// Format the XML so it's human-readable (even though it can't/shouldn't be modified by users).
	d_output_stream.setAutoFormatting(true);

	//
	// Write out the archive header.
	//

	// Start the root serialization XML element.
	d_output_stream.writeStartElement(ArchiveCommon::XML_ROOT_ELEMENT_NAME);

	// Write the archive signature string as an attribute.
	d_output_stream.writeAttribute(
			ArchiveCommon::XML_ARCHIVE_SIGNATURE_ATTRIBUTE_NAME,
			QString::fromLatin1(
					ArchiveCommon::XML_ARCHIVE_SIGNATURE.data(),
					ArchiveCommon::XML_ARCHIVE_SIGNATURE.length()));

	// Write the XML archive format version as an attribute.
	d_output_stream.writeAttribute(
			ArchiveCommon::XML_ARCHIVE_FORMAT_VERSION_ATTRIBUTE_NAME,
			C_LOCALE.toString(ArchiveCommon::XML_ARCHIVE_FORMAT_VERSION));

	// Write the scribe version as an attribute.
	d_output_stream.writeAttribute(
			ArchiveCommon::XML_SCRIBE_VERSION_ATTRIBUTE_NAME,
			C_LOCALE.toString(Scribe::get_current_scribe_version()));
}


GPlatesScribe::XmlArchiveWriter::~XmlArchiveWriter()
{
	if (!d_closed)
	{
		// Since this is a destructor we cannot let any exceptions escape.
		// If one is thrown we just have to lump it and continue on.
		try
		{
			close();
		}
		catch (...)
		{
		}
	}
}


void
GPlatesScribe::XmlArchiveWriter::close()
{
	if (!d_closed)
	{
		//
		// Write out the end information.
		//

		// End the root serialization XML element.
		d_output_stream.writeEndElement(); // XML_ROOT_ELEMENT_NAME

		d_closed = true;
	}
}


void
GPlatesScribe::XmlArchiveWriter::write_transcription(
		const Transcription &transcription)
{
	//
	// Write the transcription to the XML archive.
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


	//
	// Write out the start transcription information.
	//

	// Start the transcription XML element.
	d_output_stream.writeStartElement(ArchiveCommon::XML_TRANSCRIPTION_ELEMENT_NAME);

	//
	// Write out the object tags.
	//

	d_output_stream.writeStartElement(ArchiveCommon::XML_OBJECT_TAG_GROUP_ELEMENT_NAME);

	const unsigned int num_object_tags = transcription.get_num_object_tags();
	for (unsigned int object_tag_id = 0; object_tag_id < num_object_tags; ++object_tag_id)
	{
		d_output_stream.writeStartElement(ArchiveCommon::XML_OBJECT_TAG_ELEMENT_NAME);

		write(transcription.get_object_tag(object_tag_id));

		d_output_stream.writeEndElement(); // XML_OBJECT_TAG_ELEMENT_NAME
	}

	d_output_stream.writeEndElement(); // XML_OBJECT_TAG_GROUP_ELEMENT_NAME

	//
	// Write out the unique strings.
	//

	d_output_stream.writeStartElement(ArchiveCommon::XML_STRING_GROUP_ELEMENT_NAME);

	const unsigned int num_unique_strings = transcription.get_num_unique_string_objects();
	for (unsigned int unique_string_index = 0; unique_string_index < num_unique_strings; ++unique_string_index)
	{
		d_output_stream.writeStartElement(ArchiveCommon::XML_STRING_ELEMENT_NAME);

		write(transcription.get_unique_string_object(unique_string_index));

		d_output_stream.writeEndElement(); // XML_STRING_ELEMENT_NAME
	}

	d_output_stream.writeEndElement(); // XML_STRING_GROUP_ELEMENT_NAME


	//
	// Write out the objects.
	//

	d_output_stream.writeStartElement(ArchiveCommon::XML_OBJECT_GROUP_ELEMENT_NAME);

	const unsigned int num_object_ids = transcription.get_num_object_ids();
	for (Transcription::object_id_type object_id = 0; object_id < num_object_ids; ++object_id)
	{
		const Transcription::ObjectType object_type = transcription.get_object_type(object_id);

		// Skip past any unused object ids.
		if (object_type == Transcription::UNUSED)
		{
			continue;
		}

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			d_output_stream.writeStartElement(ArchiveCommon::XML_SIGNED_OBJECT_ELEMENT_NAME);
			d_output_stream.writeAttribute(ArchiveCommon::XML_OBJECT_ID, C_LOCALE.toString(object_id));
			write(transcription.get_signed_integer(object_id));
			d_output_stream.writeEndElement();
			break;

		case Transcription::UNSIGNED_INTEGER:
			d_output_stream.writeStartElement(ArchiveCommon::XML_UNSIGNED_OBJECT_ELEMENT_NAME);
			d_output_stream.writeAttribute(ArchiveCommon::XML_OBJECT_ID, C_LOCALE.toString(object_id));
			write(transcription.get_unsigned_integer(object_id));
			d_output_stream.writeEndElement();
			break;

		case Transcription::FLOAT:
			d_output_stream.writeStartElement(ArchiveCommon::XML_FLOAT_OBJECT_ELEMENT_NAME);
			d_output_stream.writeAttribute(ArchiveCommon::XML_OBJECT_ID, C_LOCALE.toString(object_id));
			write(transcription.get_float(object_id));
			d_output_stream.writeEndElement();
			break;

		case Transcription::DOUBLE:
			d_output_stream.writeStartElement(ArchiveCommon::XML_DOUBLE_OBJECT_ELEMENT_NAME);
			d_output_stream.writeAttribute(ArchiveCommon::XML_OBJECT_ID, C_LOCALE.toString(object_id));
			write(transcription.get_double(object_id));
			d_output_stream.writeEndElement();
			break;

		case Transcription::STRING:
			d_output_stream.writeStartElement(ArchiveCommon::XML_STRING_OBJECT_ELEMENT_NAME);
			d_output_stream.writeAttribute(ArchiveCommon::XML_OBJECT_ID, C_LOCALE.toString(object_id));
			write(transcription.get_string_object(object_id));
			d_output_stream.writeEndElement();
			break;

		case Transcription::COMPOSITE:
			d_output_stream.writeStartElement(ArchiveCommon::XML_COMPOSITE_OBJECT_ELEMENT_NAME);
			d_output_stream.writeAttribute(ArchiveCommon::XML_OBJECT_ID, C_LOCALE.toString(object_id));
			write(transcription.get_composite_object(object_id));
			d_output_stream.writeEndElement();
			break;

		default:
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					false,
					GPLATES_ASSERTION_SOURCE);
			break;
		}
	}

	d_output_stream.writeEndElement(); // XML_OBJECT_GROUP_ELEMENT_NAME

	//
	// Write out the transcription end information.
	//

	// End the transcription XML element.
	d_output_stream.writeEndElement(); // XML_TRANSCRIPTION_ELEMENT_NAME;
}


void
GPlatesScribe::XmlArchiveWriter::write(
		const Transcription::CompositeObject &composite_object)
{
	const unsigned int num_keys = composite_object.get_num_keys();

	// Write out the child keys.
	for (unsigned int key_index = 0; key_index < num_keys; ++key_index)
	{
		d_output_stream.writeStartElement(ArchiveCommon::XML_OBJECT_KEY_ELEMENT_NAME);

		// Write the current child key.
		const Transcription::object_key_type object_key = composite_object.get_key(key_index);

		d_output_stream.writeStartElement(ArchiveCommon::XML_OBJECT_TAG_ID_ELEMENT_NAME);
		write(object_key.first);
		d_output_stream.writeEndElement(); // XML_OBJECT_TAG_ID_ELEMENT_NAME

		d_output_stream.writeStartElement(ArchiveCommon::XML_OBJECT_TAG_VERSION_ELEMENT_NAME);
		write(object_key.second);
		d_output_stream.writeEndElement(); // XML_OBJECT_TAG_VERSION_ELEMENT_NAME

		// Write out the child object ids associated with the current child key.
		const unsigned int num_children_with_key = composite_object.get_num_children_with_key(object_key);
		for (unsigned int child_index = 0; child_index < num_children_with_key; ++child_index)
		{
			const Transcription::object_id_type object_id =
					composite_object.get_child(object_key, child_index);

			d_output_stream.writeStartElement(ArchiveCommon::XML_OBJECT_ID);
			write(object_id);
			d_output_stream.writeEndElement(); // XML_OBJECT_ID
		}

		d_output_stream.writeEndElement(); // XML_OBJECT_KEY_ELEMENT_NAME
	}
}


void
GPlatesScribe::XmlArchiveWriter::write(
		const Transcription::int32_type object)
{
	d_output_stream.writeCharacters(C_LOCALE.toString(object));
}


void
GPlatesScribe::XmlArchiveWriter::write(
		const Transcription::uint32_type object)
{
	d_output_stream.writeCharacters(C_LOCALE.toString(object));
}


void
GPlatesScribe::XmlArchiveWriter::write(
		const float object)
{
	// Ensure enough precision is written out.
	d_output_stream.writeCharacters(
			C_LOCALE.toString(object, 'g', std::numeric_limits<float>::digits10 + 2));
}


void
GPlatesScribe::XmlArchiveWriter::write(
		const double &object)
{
	// Ensure enough precision is written out.
	d_output_stream.writeCharacters(
			C_LOCALE.toString(object, 'g', std::numeric_limits<double>::digits10 + 2));
}


void
GPlatesScribe::XmlArchiveWriter::write(
		const std::string &object)
{
	d_output_stream.writeCharacters(
			QString::fromLatin1(object.data(), object.length()));
}
