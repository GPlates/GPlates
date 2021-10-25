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
#include <limits>
#include <locale>

#include "ScribeTextArchiveWriter.h"

#include "Scribe.h"
#include "ScribeExceptions.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/Real.h"


GPlatesScribe::TextArchiveWriter::TextArchiveWriter(
		std::ostream &output_stream) :
	d_output_stream(output_stream),
	d_output_stream_flags_saver(output_stream),
	d_output_stream_precision_saver(output_stream),
	d_output_stream_locale_saver(output_stream)
{
	//
	// Set up the archive stream.
	//

	// Use the classic "C" locale to ensure the same behaviour reading and writing regardless of
	// the current global locale.
	// An application starts with the global local set to the classic "C" locale but it could
	// get changed after that by the application.
	d_output_stream.imbue(std::locale::classic());

	//
	// Write out the archive header.
	//

	// Write the archive signature string.
	//
	// Write out as individual characters instead of a string since the latter writes the number of
	// characters to the stream first and when reading wrong archive data this could be any number.
	const unsigned int archive_signature_size = ArchiveCommon::TEXT_ARCHIVE_SIGNATURE.size();
	for (unsigned int archive_signature_char_index = 0;
		archive_signature_char_index < archive_signature_size;
		++archive_signature_char_index)
	{
		write(ArchiveCommon::TEXT_ARCHIVE_SIGNATURE[archive_signature_char_index]);
	}

	// Write the text archive format version.
	const unsigned int text_archive_format_version = ArchiveCommon::TEXT_ARCHIVE_FORMAT_VERSION;
	write(text_archive_format_version);

	// Write the scribe version.
	const unsigned int scribe_version = Scribe::get_current_scribe_version();
	write(scribe_version);
}


void
GPlatesScribe::TextArchiveWriter::write_transcription(
		const Transcription &transcription)
{
	//
	// Write out the object tags.
	//

	const unsigned int num_object_tags = transcription.get_num_object_tags();
	write(num_object_tags);

	for (unsigned int object_tag_name_id = 0; object_tag_name_id < num_object_tags; ++object_tag_name_id)
	{
		write(transcription.get_object_tag_name(object_tag_name_id));
	}


	//
	// Write out the unique strings.
	//

	const unsigned int num_unique_strings = transcription.get_num_unique_string_objects();
	write(num_unique_strings);

	for (unsigned int unique_string_index = 0; unique_string_index < num_unique_strings; ++unique_string_index)
	{
		write(transcription.get_unique_string_object(unique_string_index));
	}


	//
	// Write out the objects.
	//

	const unsigned int num_object_ids = transcription.get_num_object_ids();

	Transcription::object_id_type object_id = 0;

	// Look for contiguous groups of object ids so that we don't have to write out the object id
	// for each object (instead writing the start object id and the number in group).
	while (object_id < num_object_ids)
	{
		// Skip past any unused object ids.
		do
		{
			Transcription::ObjectType object_type = transcription.get_object_type(object_id);
			if (object_type != Transcription::UNUSED)
			{
				break;
			}

			++object_id;
		}
		while (object_id < num_object_ids);

		if (object_id == num_object_ids)
		{
			break;
		}

		const Transcription::object_id_type start_object_id_in_group = object_id;
		unsigned int num_object_ids_in_group = 1;

		// Count a contiguous group of valid object ids.
		for (++object_id; object_id < num_object_ids; ++object_id, ++num_object_ids_in_group)
		{
			Transcription::ObjectType object_type = transcription.get_object_type(object_id);
			if (object_type == Transcription::UNUSED)
			{
				break;
			}
		}

		//
		// Write out the contiguous group of objects
		//

		write_object_group(transcription, start_object_id_in_group, num_object_ids_in_group);
	}

	// Write zero number of object ids in last group so reader can terminate looping over groups.
	write(static_cast<unsigned int>(0));
}


void
GPlatesScribe::TextArchiveWriter::write_object_group(
		const Transcription &transcription,
		Transcription::object_id_type start_object_id_in_group,
		unsigned int num_object_ids_in_group)
{
	//
	// Write out the contiguous group of objects
	//

	write(num_object_ids_in_group);

	if (num_object_ids_in_group == 0)
	{
		return;
	}

	write(start_object_id_in_group);

	Transcription::object_id_type object_id_in_group = start_object_id_in_group;
	for (unsigned int n = 0; n < num_object_ids_in_group; ++n, ++object_id_in_group)
	{
		const Transcription::ObjectType object_type =
				transcription.get_object_type(object_id_in_group);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			write(ArchiveCommon::SIGNED_INTEGER_CODE);
			write(transcription.get_signed_integer(object_id_in_group));
			break;

		case Transcription::UNSIGNED_INTEGER:
			write(ArchiveCommon::UNSIGNED_INTEGER_CODE);
			write(transcription.get_unsigned_integer(object_id_in_group));
			break;

		case Transcription::FLOAT:
			write(ArchiveCommon::FLOAT_CODE);
			write(transcription.get_float(object_id_in_group));
			break;

		case Transcription::DOUBLE:
			write(ArchiveCommon::DOUBLE_CODE);
			write(transcription.get_double(object_id_in_group));
			break;

		case Transcription::STRING:
			write(ArchiveCommon::STRING_CODE);
			write(transcription.get_string_object(object_id_in_group));
			break;

		case Transcription::COMPOSITE:
			write(ArchiveCommon::COMPOSITE_CODE);
			write(transcription.get_composite_object(object_id_in_group));
			break;

		default: // Transcription::UNUSED ...
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					false,
					GPLATES_ASSERTION_SOURCE);
			break;
		}
	}
}


void
GPlatesScribe::TextArchiveWriter::write(
		const Transcription::CompositeObject &composite_object)
{
	const unsigned int num_keys = composite_object.get_num_keys();
	write(num_keys);

	// Write out the child keys.
	for (unsigned int key_index = 0; key_index < num_keys; ++key_index)
	{
		// Write the current child key.
		const Transcription::object_key_type object_key = composite_object.get_key(key_index);
		write(object_key.first);
		write(object_key.second);

		const unsigned int num_children_with_key = composite_object.get_num_children_with_key(object_key);
		write(num_children_with_key);

		// Write out the child object ids associated with the current child key.
		for (unsigned int child_index = 0; child_index < num_children_with_key; ++child_index)
		{
			const Transcription::object_id_type object_id =
					composite_object.get_child(object_key, child_index);
			write(object_id);
		}
	}
}


void
GPlatesScribe::TextArchiveWriter::write(
		const Transcription::int32_type object)
{
	d_output_stream.put(' ');
	d_output_stream << object;

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_output_stream,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected writing int.");
}


void
GPlatesScribe::TextArchiveWriter::write(
		const Transcription::uint32_type object)
{
	d_output_stream.put(' ');
	d_output_stream << object;

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_output_stream,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected writing unsigned int.");
}


void
GPlatesScribe::TextArchiveWriter::write(
		const float object)
{
	d_output_stream.put(' ');

	if (GPlatesMaths::is_finite(object))
	{
		// Ensure enough precision is written out.
		d_output_stream << std::setprecision(std::numeric_limits<float>::digits10 + 2);

		d_output_stream << object;
	}
	else if (GPlatesMaths::is_positive_infinity(object))
	{
		d_output_stream << ArchiveCommon::TEXT_POSITIVE_INFINITY_VALUE;
	}
	else if (GPlatesMaths::is_negative_infinity(object))
	{
		d_output_stream << ArchiveCommon::TEXT_NEGATIVE_INFINITY_VALUE;
	}
	else
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				GPlatesMaths::is_nan(object),
				GPLATES_ASSERTION_SOURCE);

		d_output_stream << ArchiveCommon::TEXT_NAN_VALUE;
	}

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_output_stream,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected writing float.");
}


void
GPlatesScribe::TextArchiveWriter::write(
		const double &object)
{
	d_output_stream.put(' ');

	if (GPlatesMaths::is_finite(object))
	{
		// Ensure enough precision is written out.
		d_output_stream << std::setprecision(std::numeric_limits<double>::digits10 + 2);

		d_output_stream << object;
	}
	else if (GPlatesMaths::is_positive_infinity(object))
	{
		d_output_stream << ArchiveCommon::TEXT_POSITIVE_INFINITY_VALUE;
	}
	else if (GPlatesMaths::is_negative_infinity(object))
	{
		d_output_stream << ArchiveCommon::TEXT_NEGATIVE_INFINITY_VALUE;
	}
	else
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				GPlatesMaths::is_nan(object),
				GPLATES_ASSERTION_SOURCE);

		d_output_stream << ArchiveCommon::TEXT_NAN_VALUE;
	}

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_output_stream,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected writing double.");
}


void
GPlatesScribe::TextArchiveWriter::write(
		const std::string &object)
{
	const Transcription::uint32_type size = object.size();
	write(size);

	d_output_stream.put(' ');

	GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
			d_output_stream,
			GPLATES_ASSERTION_SOURCE,
			"Archive stream error detected writing string.");

	if (size > 0)
	{
		d_output_stream.write(object.data(), size);

		GPlatesGlobal::Assert<Exceptions::ArchiveStreamError>(
				d_output_stream,
				GPLATES_ASSERTION_SOURCE,
				"Archive stream error detected writing string.");
	}
}
