/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "ScribeBinaryArchiveReader.h"

#include "Scribe.h"
#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


GPlatesScribe::BinaryArchiveReader::BinaryArchiveReader(
		QDataStream &input_stream) :
	d_input_stream(input_stream)
{
	//
	// Set up the archive stream.
	//

	d_input_stream.setVersion(ArchiveCommon::BINARY_ARCHIVE_QT_STREAM_VERSION);
	d_input_stream.setByteOrder(ArchiveCommon::BINARY_ARCHIVE_QT_STREAM_BYTE_ORDER);

	//
	// Read the archive header.
	//

	// Read the archive signature string.
	//
	// Read as individual characters instead of a string since the latter reads the number of
	// characters from the stream first and when reading wrong archive data this could be any number.
	const unsigned int archive_signature_size = ArchiveCommon::BINARY_ARCHIVE_SIGNATURE.size();
	for (unsigned int archive_signature_char_index = 0;
		archive_signature_char_index < archive_signature_size;
		++archive_signature_char_index)
	{
		// Read directly as unencoded integers (ie, not varints).
		// We want to make sure this is a scribe binary archive before we starting decoding integers.
		qint8 archive_signature_char;
		d_input_stream >> archive_signature_char;

		GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
				d_input_stream.status() == QDataStream::Ok,
				GPLATES_ASSERTION_SOURCE,
				"Archive stream error detected reading archive signature.");

		// Throw exception if archive signature is invalid.
		GPlatesGlobal::Assert<Exceptions::InvalidArchiveSignature>(
				archive_signature_char == ArchiveCommon::BINARY_ARCHIVE_SIGNATURE[archive_signature_char_index],
				GPLATES_ASSERTION_SOURCE);
	}

	// Read the binary archive format version.
	const unsigned int binary_archive_format_version = read_unsigned();

	// Throw exception if the binary archive format version used to write the archive is a future version.
	GPlatesGlobal::Assert<Exceptions::UnsupportedVersion>(
			binary_archive_format_version <= ArchiveCommon::BINARY_ARCHIVE_FORMAT_VERSION,
			GPLATES_ASSERTION_SOURCE);

	// Read the version of the Scribe used to create the archive being read.
	const unsigned int archive_scribe_version = read_unsigned();

	// Throw exception if the scribe version used to write the archive is a future version.
	GPlatesGlobal::Assert<Exceptions::UnsupportedVersion>(
			archive_scribe_version <= Scribe::get_current_scribe_version(),
			GPLATES_ASSERTION_SOURCE);
}


GPlatesScribe::Transcription::non_null_ptr_type
GPlatesScribe::BinaryArchiveReader::read_transcription()
{
	Transcription::non_null_ptr_type transcription = Transcription::create();

	//
	// Read the object tags.
	//

	const unsigned int num_object_tags = read_unsigned();

	for (unsigned int object_tag_id = 0; object_tag_id < num_object_tags; ++object_tag_id)
	{
		const std::string object_tag = read_string();

		transcription->add_object_tag(object_tag);
	}

	//
	// Read the unique strings.
	//

	const unsigned int num_unique_strings = read_unsigned();

	for (unsigned int unique_string_index = 0; unique_string_index < num_unique_strings; ++unique_string_index)
	{
		const std::string unique_string = read_string();

		transcription->add_unique_string_object(unique_string);
	}

	//
	// Read the objects.
	//

	// Look for contiguous groups of object ids so that we don't have to read in the object id
	// for each object (instead reading the start object id and the number in group).
	while (read_object_group(*transcription))
	{ }

	return transcription;
}


bool
GPlatesScribe::BinaryArchiveReader::read_object_group(
		Transcription &transcription)
{
	//
	// Read a contiguous group of objects
	//

	const unsigned int num_object_ids_in_group = read_unsigned();

	if (num_object_ids_in_group == 0)
	{
		return false;
	}

	const Transcription::object_id_type start_object_id_in_group = read_unsigned();

	Transcription::object_id_type object_id_in_group = start_object_id_in_group;
	for (unsigned int n = 0; n < num_object_ids_in_group; ++n, ++object_id_in_group)
	{
		// Read the object type integer code.
		const unsigned int object_type_code = read_unsigned();

		if (object_type_code == ArchiveCommon::SIGNED_INTEGER_CODE)
		{
			transcription.add_signed_integer(object_id_in_group, read_signed());
		}
		else if (object_type_code == ArchiveCommon::UNSIGNED_INTEGER_CODE)
		{
			transcription.add_unsigned_integer(object_id_in_group, read_unsigned());
		}
		else if (object_type_code == ArchiveCommon::FLOAT_CODE)
		{
			transcription.add_float(object_id_in_group, read_float());
		}
		else if (object_type_code == ArchiveCommon::DOUBLE_CODE)
		{
			transcription.add_double(object_id_in_group, read_double());
		}
		else if (object_type_code == ArchiveCommon::STRING_CODE)
		{
			transcription.add_string_object(object_id_in_group, read_unsigned());
		}
		else if (object_type_code == ArchiveCommon::COMPOSITE_CODE)
		{
			transcription.add_composite_object(object_id_in_group);
			read(transcription.get_composite_object(object_id_in_group));
		}
		else
		{
			GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
					false,
					GPLATES_ASSERTION_SOURCE,
					"Archive stream error detected reading object type.");
		}
	}

	return true;
}


void
GPlatesScribe::BinaryArchiveReader::read(
		Transcription::CompositeObject &composite_object)
{
	const unsigned int num_keys = read_unsigned();

	// Read the child keys.
	for (unsigned int key_index = 0; key_index < num_keys; ++key_index)
	{
		// Read the current child key.
		const Transcription::object_tag_id_type object_tag_id = read_unsigned();
		const Transcription::object_tag_version_type object_tag_version = read_unsigned();
		const Transcription::object_key_type object_key(object_tag_id, object_tag_version);

		const unsigned int num_children_with_key = read_unsigned();

		// Read the child object ids associated with the current child key.
		for (unsigned int child_index = 0; child_index < num_children_with_key; ++child_index)
		{
			const Transcription::object_id_type object_id = read_unsigned();

			composite_object.add_child(object_key, object_id);
		}
	}
}


int
GPlatesScribe::BinaryArchiveReader::read_signed()
{
	// Decode unsigned as signed according to:
	//
	// 0 ->  0
	// 1 -> -1
	// 2 ->  1
	// 3 -> -2
	// 4 ->  2
	// ...
	// 4294967294 ->  2147483647   // 0xfffffffe -> 0x7fffffff
	// 4294967295 -> -2147483648   // 0xffffffff -> 0x80000000
	//
	// ...this ensures signed integer values near zero get decoded as varints with fewer bytes.
	//
	const quint32 unsigned_object = read_unsigned();
	if ((unsigned_object & 1) == 0)
	{
		// Perform unsigned decoding.
		// Then interpret the 32-bit unsigned bit pattern as signed.
		return static_cast<qint32>(unsigned_object >> 1);
	}
	else // 'object' is negative...
	{
		// Perform unsigned decoding.
		// Then interpret the 32-bit unsigned bit pattern as signed.
		return static_cast<qint32>(0xffffffff - (unsigned_object >> 1));
	}
}


unsigned int
GPlatesScribe::BinaryArchiveReader::read_unsigned()
{
	// Decode the integer from a variable number of bytes.
	//
	// See Google's Protocol Buffers for more details on varints:
	//   https://developers.google.com/protocol-buffers/docs/encoding#varints
	//
	quint32 object = 0;

	unsigned int varint_shift = 0;

	// We shouldn't need more than 9 iterations to decode an 8-byte unsigned integer.
	// If we get more iterations then the archive must be corrupted.
	// We should only be getting 4-byte integers with the current binary archive writer but
	// we assume 8-byte in case future writers support 8-byte integers.
	const unsigned int max_count = 9;
	unsigned int count;
	for (count = 0; count < max_count; ++count)
	{
		// Each byte stores 7 bits of integer and one flag bit.
		quint8 object_byte;
		d_input_stream >> object_byte;

		GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
				d_input_stream.status() == QDataStream::Ok,
				GPLATES_ASSERTION_SOURCE,
				"Archive stream error detected reading unsigned.");

		// Mask out flag bit and shift 7 integer bits to correct location inside integer.
		object |= ((object_byte & 0x7f) << varint_shift);

		// If there are no more bytes to follow then we're done.
		// Check the most-significant bit of byte for this.
		if ((object_byte & 0x80) == 0)
		{
			break;
		}

		// Next 7 bits of integer will need to be shifted 7 bits further left.
		varint_shift += 7;
	}

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			count < max_count,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected decoding unsigned.");

	return object;
}


float
GPlatesScribe::BinaryArchiveReader::read_float()
{
	float object;
	d_input_stream >> object;

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_input_stream.status() == QDataStream::Ok,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected reading float.");

	return object;
}


double
GPlatesScribe::BinaryArchiveReader::read_double()
{
	double object;
	d_input_stream >> object;

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_input_stream.status() == QDataStream::Ok,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected reading double.");

	return object;
}


std::string
GPlatesScribe::BinaryArchiveReader::read_string()
{
	std::string object;

	const unsigned int size = read_unsigned();

	if (size > 0)
	{
		object.resize(size);

		for (unsigned int n = 0; n < size; ++n)
		{
			quint8 elem;
			d_input_stream >> elem;

			GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
					d_input_stream.status() == QDataStream::Ok,
					GPLATES_ASSERTION_SOURCE,
					"Archive stream error detected reading string.");

			object[n] = static_cast<char>(elem);
		}
	}

	return object;
}
