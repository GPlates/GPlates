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

#include <iomanip>
#include <locale>

#include "ScribeTextArchiveReader.h"

#include "Scribe.h"
#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


GPlatesScribe::TextArchiveReader::TextArchiveReader(
		std::istream &input_stream) :
	d_input_stream(input_stream),
	d_input_stream_flags_saver(input_stream),
	d_input_stream_precision_saver(input_stream),
	d_input_stream_locale_saver(input_stream)
{
	//
	// Set up the archive stream.
	//

	// Use the classic "C" locale to ensure the same behaviour reading and writing regardless of
	// the current global locale.
	// An application starts with the global local set to the classic "C" locale but it could
	// get changed after that by the application.
	d_input_stream.imbue(std::locale::classic());

	// Make sure leading whitespace is skipped when using 'operator>>'.
	d_input_stream >> std::skipws;

	//
	// Read the archive header.
	//

	// Read the archive signature string.
	//
	// Read as individual characters instead of a string since the latter reads the number of
	// characters from the stream first and when reading wrong archive data this could be any number.
	const unsigned int archive_signature_size = ArchiveCommon::TEXT_ARCHIVE_SIGNATURE.size();
	for (unsigned int archive_signature_char_index = 0;
		archive_signature_char_index < archive_signature_size;
		++archive_signature_char_index)
	{
		const int archive_signature_char = read<int>();

		// Throw exception if archive signature is invalid.
		GPlatesGlobal::Assert<Exceptions::InvalidArchiveSignature>(
				archive_signature_char == ArchiveCommon::TEXT_ARCHIVE_SIGNATURE[archive_signature_char_index],
				GPLATES_ASSERTION_SOURCE);
	}

	// Read the text archive format version.
	const unsigned int text_archive_format_version = read<unsigned int>();

	// Throw exception if the text archive format version used to write the archive is a future version.
	GPlatesGlobal::Assert<Exceptions::UnsupportedVersion>(
			text_archive_format_version <= ArchiveCommon::TEXT_ARCHIVE_FORMAT_VERSION,
			GPLATES_ASSERTION_SOURCE);

	// Read the version of the Scribe used to create the archive being read.
	const unsigned int archive_scribe_version = read<unsigned int>();

	// Throw exception if the scribe version used to write the archive is a future version.
	GPlatesGlobal::Assert<Exceptions::UnsupportedVersion>(
			archive_scribe_version <= Scribe::get_current_scribe_version(),
			GPLATES_ASSERTION_SOURCE);
}


GPlatesScribe::Transcription::non_null_ptr_type
GPlatesScribe::TextArchiveReader::read_transcription()
{
	Transcription::non_null_ptr_type transcription = Transcription::create();

	//
	// Read the object tags.
	//

	const unsigned int num_object_tags = read<unsigned int>();

	for (unsigned int object_tag_id = 0; object_tag_id < num_object_tags; ++object_tag_id)
	{
		const std::string object_tag = read<std::string>();

		transcription->add_object_tag(object_tag);
	}

	//
	// Read the unique strings.
	//

	const unsigned int num_unique_strings = read<unsigned int>();

	for (unsigned int unique_string_index = 0; unique_string_index < num_unique_strings; ++unique_string_index)
	{
		const std::string unique_string = read<std::string>();

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
GPlatesScribe::TextArchiveReader::read_object_group(
		Transcription &transcription)
{
	//
	// Read a contiguous group of objects
	//

	const unsigned int num_object_ids_in_group = read<unsigned int>();

	if (num_object_ids_in_group == 0)
	{
		return false;
	}

	const Transcription::object_id_type start_object_id_in_group =
			read<Transcription::object_id_type>();

	Transcription::object_id_type object_id_in_group = start_object_id_in_group;
	for (unsigned int n = 0; n < num_object_ids_in_group; ++n, ++object_id_in_group)
	{
		// Read the object type integer code.
		const unsigned int object_type_code = read<unsigned int>();

		if (object_type_code == ArchiveCommon::SIGNED_INTEGER_CODE)
		{
			transcription.add_signed_integer(
					object_id_in_group,
					read<Transcription::int32_type>());
		}
		else if (object_type_code == ArchiveCommon::UNSIGNED_INTEGER_CODE)
		{
			transcription.add_unsigned_integer(
					object_id_in_group,
					read<Transcription::uint32_type>());
		}
		else if (object_type_code == ArchiveCommon::FLOAT_CODE)
		{
			transcription.add_float(object_id_in_group, read<float>());
		}
		else if (object_type_code == ArchiveCommon::DOUBLE_CODE)
		{
			transcription.add_double(object_id_in_group, read<double>());
		}
		else if (object_type_code == ArchiveCommon::STRING_CODE)
		{
			transcription.add_string_object(object_id_in_group, read<unsigned int>());
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
GPlatesScribe::TextArchiveReader::read(
		Transcription::CompositeObject &composite_object)
{
	const unsigned int num_keys = read<unsigned int>();

	// Read the child keys.
	for (unsigned int key_index = 0; key_index < num_keys; ++key_index)
	{
		// Read the current child key.
		const Transcription::object_tag_id_type object_tag_id =
				read<Transcription::object_tag_id_type>();
		const Transcription::object_tag_version_type object_tag_version =
				read<Transcription::object_tag_version_type>();
		const Transcription::object_key_type object_key(object_tag_id, object_tag_version);

		const unsigned int num_children_with_key = read<unsigned int>();

		// Read the child object ids associated with the current child key.
		for (unsigned int child_index = 0; child_index < num_children_with_key; ++child_index)
		{
			const Transcription::object_id_type object_id =
					read<Transcription::object_id_type>();

			composite_object.add_child(object_key, object_id);
		}
	}
}


template <typename ObjectType>
ObjectType
GPlatesScribe::TextArchiveReader::read()
{
	ObjectType object;
	d_input_stream >> object;

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_input_stream,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected reading integral/floating-point primitive.");

	return object;
}


template <>
std::string
GPlatesScribe::TextArchiveReader::read<std::string>()
{
	std::string object;

	const unsigned int size = read<unsigned int>();

	// Read the space character.
	d_input_stream.get();

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_input_stream,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected reading string.");

	if (size > 0)
	{
		object.resize(size);
		d_input_stream.read(&*object.begin(), size);

		GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
				d_input_stream,
				GPLATES_ASSERTION_SOURCE,
				"Archive stream error detected reading string.");
	}

	return object;
}
