/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <sstream>
#include <boost/cast.hpp>

#include "TranscriptionScribeContext.h"

#include "ScribeExceptions.h"
#include "TranscribeSequenceProtocol.h"

#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"


GPlatesScribe::TranscriptionScribeContext::TranscriptionScribeContext(
		const Transcription::non_null_ptr_type &transcription,
		bool is_saving_) :
	d_is_saving(is_saving_),
	d_next_save_object_id(ROOT_OBJECT_ID + 1),
	d_transcription(transcription)
{
	// Transcribing at the root scope will require an emulated root composite object.
	//
	// This is so calls such as 'Scribe::transcribe("my_first_object", my_first_object)'
	// will have a place to store the "my_first_object" object tag/key.
	push_transcribed_object(ROOT_OBJECT_ID);
}


void
GPlatesScribe::TranscriptionScribeContext::push_transcribed_object(
		object_id_type object_id)
{
	d_transcribed_object_stack.push(TranscribedObject(object_id));
}


void
GPlatesScribe::TranscriptionScribeContext::pop_transcribed_object()
{
	// After popping we should still have at least one transcribed object on the stack
	// that is the emulated root object.
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			d_transcribed_object_stack.size() >= 2,
			GPLATES_ASSERTION_SOURCE,
			"Popped too many transcribed objects off the stack.");

	if (is_saving())
	{
		// If nothing was transcribed for the current object id then create an empty composite object.
		// This is just so the parent object that references the current object will be able to find
		// something (eg, this is needed by Transcription::is_complete()).
		//
		// This usually happens when nothing is transcribed for an object (eg, an empty base class).
		TranscribedObject &transcribed_object = d_transcribed_object_stack.top();
		if (!transcribed_object.object_category)
		{
			d_transcription->add_composite_object(transcribed_object.object_id);
		}
	}

	d_transcribed_object_stack.pop();
}


GPlatesScribe::TranscriptionScribeContext::object_id_type
GPlatesScribe::TranscriptionScribeContext::allocate_save_object_id()
{
	const object_id_type save_object_id = d_next_save_object_id;

	// Increment the save object id for next time.
	++d_next_save_object_id;

	return save_object_id;
}


boost::optional<GPlatesScribe::TranscriptionScribeContext::object_id_type>
GPlatesScribe::TranscriptionScribeContext::is_in_transcription(
		const ObjectTag &object_tag) const
{
	const TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	// Check if the object category (if determined yet) is a composite.
	if (transcribed_object.object_category &&
		transcribed_object.object_category.get() != TranscribedObject::COMPOSITE)
	{
		return boost::none;
	}

	// Check the object type is a composite.
	// The object type retrieval itself shouldn't throw because the transcription
	// should be complete (when it was written in the save path it was tested for
	// completeness which ensures no composites reference dangling child object IDs).
	if (d_transcription->get_object_type(transcribed_object.object_id) != Transcription::COMPOSITE)
	{
		return boost::none;
	}

	object_id_type object_id;

	// Using a pointer so we can change it as we iterate through the sections of the object tag.
	Transcription::CompositeObject *section_composite_object =
			&d_transcription->get_composite_object(transcribed_object.object_id);

	// Iterate over all sections of the object tag.
	// Note that there is guaranteed to be at least one section.
	const std::vector<ObjectTag::Section> &object_tag_sections = object_tag.get_sections();
	const unsigned int num_object_tag_sections = object_tag_sections.size();
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			num_object_tag_sections > 0,
			GPLATES_ASSERTION_SOURCE,
			"Expected at least one section in object tag.");

	for (unsigned int object_tag_section_index = 0;
		object_tag_section_index < num_object_tag_sections;
		++object_tag_section_index)
	{
		const ObjectTag::Section &object_tag_section = object_tag_sections[object_tag_section_index];

		// The last section of the object tag (or the only section if object tag has one section)
		// is treated differently because we don't know if the object it refers to is going to be a
		// composite object or a primitive object. Whereas prior sections (to the last section) have a
		// subsequent section and hence we know they will be composites.
		boost::optional<object_id_type &> object_id_ref;
		if (object_tag_section_index == num_object_tag_sections - 1) // is last section ?
		{
			// Only the last section will transcribe the actual object ID.
			object_id_ref = object_id;
		}

		switch (object_tag_section.get_type())
		{
		case ObjectTag::TAG_SECTION:
			if (!load_tag_section(
					object_tag_section.get_tag_name(),
					object_tag_section.get_tag_version(),
					section_composite_object,
					object_id_ref))
			{
				return boost::none;
			}
			break;

		case ObjectTag::ARRAY_INDEX_SECTION:
			if (!load_array_index_section(
					object_tag_section.get_tag_name(),
					object_tag_section.get_tag_version(),
					object_tag_section.get_array_index(),
					section_composite_object,
					object_id_ref))
			{
				return boost::none;
			}
			break;

		case ObjectTag::ARRAY_SIZE_SECTION:
			if (!load_array_size_section(
					object_tag_section.get_tag_name(),
					object_tag_section.get_tag_version(),
					section_composite_object,
					object_id_ref))
			{
				return boost::none;
			}
			break;

		default:
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					false,
					GPLATES_ASSERTION_SOURCE,
					"Expecting object tag to contain only tags and arrays (indices/sizes).");
			break;
		}
	}

	return object_id;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe_object_id(
		object_id_type &object_id,
		const ObjectTag &object_tag)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	if (transcribed_object.object_category)
	{
		// Note that we throw exception on load path (as well as save path) instead of returning
		// false because it indicates a programming error - it shouldn't actually be possible to
		// get here if we're transcribing a primitive object (ie, if 'object_category' is PRIMITIVE)
		// because class Scribe will never call 'transcribe_object_id()' in that case.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				transcribed_object.object_category.get() == TranscribedObject::COMPOSITE,
				GPLATES_ASSERTION_SOURCE,
				"Attempted to transcribe a child object into a primitive (non-composite object).");
	}
	else
	{
		// This is the first time a child object id is being transcribed into the current object.
		// So that makes the current object a composite object.

		if (is_saving())
		{
			d_transcription->add_composite_object(transcribed_object.object_id);
		}
		else // loading...
		{
			// Check the object type is a composite.
			// The object type retrieval itself shouldn't throw because the transcription
			// should be complete (when it was written in the save path it was tested for
			// completeness which ensures no composites reference dangling child object IDs).
			if (d_transcription->get_object_type(transcribed_object.object_id) != Transcription::COMPOSITE)
			{
				return false;
			}
		}

		transcribed_object.object_category = TranscribedObject::COMPOSITE;
	}

	// Using a pointer so we can change it as we iterate through the sections of the object tag.
	Transcription::CompositeObject *section_composite_object =
			&d_transcription->get_composite_object(transcribed_object.object_id);

	// Iterate over all sections of the object tag.
	// Note that there is guaranteed to be at least one section.
	const std::vector<ObjectTag::Section> &object_tag_sections = object_tag.get_sections();
	const unsigned int num_object_tag_sections = object_tag_sections.size();
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			num_object_tag_sections > 0,
			GPLATES_ASSERTION_SOURCE,
			"Expected at least one section in object tag.");

	for (unsigned int object_tag_section_index = 0;
		object_tag_section_index < num_object_tag_sections;
		++object_tag_section_index)
	{
		const ObjectTag::Section &object_tag_section = object_tag_sections[object_tag_section_index];

		// The last section of the object tag (or the only section if object tag has one section)
		// is treated differently because we don't know if the object it refers to is going to be a
		// composite object or a primitive object. Whereas prior sections (to the last section) have a
		// subsequent section and hence we know they will be composites.
		boost::optional<object_id_type &> object_id_ref;
		if (object_tag_section_index == num_object_tag_sections - 1) // is last section ?
		{
			// Only the last section will transcribe the actual object ID.
			object_id_ref = object_id;
		}

		if (is_saving())
		{
			switch (object_tag_section.get_type())
			{
			case ObjectTag::TAG_SECTION:
				save_tag_section(
						object_tag_section.get_tag_name(),
						object_tag_section.get_tag_version(),
						section_composite_object,
						object_id_ref);
				break;

			case ObjectTag::ARRAY_INDEX_SECTION:
				save_array_index_section(
						object_tag_section.get_tag_name(),
						object_tag_section.get_tag_version(),
						object_tag_section.get_array_index(),
						section_composite_object,
						object_id_ref);
				break;

			case ObjectTag::ARRAY_SIZE_SECTION:
				save_array_size_section(
						object_tag_section.get_tag_name(),
						object_tag_section.get_tag_version(),
						section_composite_object,
						object_id_ref);
				break;

			default:
				GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
						false,
						GPLATES_ASSERTION_SOURCE,
						"Expecting object tag to contain only tags and arrays (indices/sizes).");
				break;
			}
		}
		else // loading...
		{
			switch (object_tag_section.get_type())
			{
			case ObjectTag::TAG_SECTION:
				if (!load_tag_section(
						object_tag_section.get_tag_name(),
						object_tag_section.get_tag_version(),
						section_composite_object,
						object_id_ref))
				{
					return false;
				}
				break;

			case ObjectTag::ARRAY_INDEX_SECTION:
				if (!load_array_index_section(
						object_tag_section.get_tag_name(),
						object_tag_section.get_tag_version(),
						object_tag_section.get_array_index(),
						section_composite_object,
						object_id_ref))
				{
					return false;
				}
				break;

			case ObjectTag::ARRAY_SIZE_SECTION:
				if (!load_array_size_section(
						object_tag_section.get_tag_name(),
						object_tag_section.get_tag_version(),
						section_composite_object,
						object_id_ref))
				{
					return false;
				}
				break;

			default:
				GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
						false,
						GPLATES_ASSERTION_SOURCE,
						"Expecting object tag to contain only tags and arrays (indices/sizes).");
				break;
			}
		}
	}

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		std::string &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		d_transcription->add_string(transcribed_object.object_id, object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		if (object_type != Transcription::STRING)
		{
			return false;
		}

		object = d_transcription->get_string(transcribed_object.object_id);
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		bool &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		const Transcription::int32_type int32_object =
				static_cast<Transcription::int32_type>(object);

		d_transcription->add_signed_integer(transcribed_object.object_id, int32_object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		// Test all integral/floating-point types for zero.
		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			object = static_cast<bool>(
					d_transcription->get_signed_integer(transcribed_object.object_id));
			break;

		case Transcription::UNSIGNED_INTEGER:
			object = static_cast<bool>(
					d_transcription->get_unsigned_integer(transcribed_object.object_id));
			break;

		case Transcription::FLOAT:
			object = GPlatesMaths::are_almost_exactly_equal(
					d_transcription->get_float(transcribed_object.object_id),
					0.0f);
			break;

		case Transcription::DOUBLE:
			object = GPlatesMaths::are_almost_exactly_equal(
					d_transcription->get_double(transcribed_object.object_id),
					0.0);
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		char &object)
{
	signed char signed_char_object;

	if (is_saving())
	{
		signed_char_object = object;
	}

	// Re-use 'transcribe()' for 'signed char'.
	if (!transcribe(signed_char_object))
	{
		return false;
	}

	if (is_loading())
	{
		object = signed_char_object;
	}

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		signed char &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		// 'signed char' is 8-bits.
		const Transcription::int32_type int32_object =
				static_cast<Transcription::int32_type>(object);

		d_transcription->add_signed_integer(transcribed_object.object_id, int32_object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			try
			{
				// 'signed char' is 8-bits.
				object = boost::numeric_cast<signed char>(
						d_transcription->get_signed_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		case Transcription::UNSIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<signed char>(
						d_transcription->get_unsigned_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		unsigned char &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		// 'unsigned char' is 8-bits.
		const Transcription::uint32_type uint32_object =
				static_cast<Transcription::uint32_type>(object);

		d_transcription->add_unsigned_integer(transcribed_object.object_id, uint32_object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<unsigned char>(
						d_transcription->get_signed_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		case Transcription::UNSIGNED_INTEGER:
			try
			{
				// 'unsigned char' is 8-bits.
				object = boost::numeric_cast<unsigned char>(
						d_transcription->get_unsigned_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		short &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		// 'short' is 16-bits.
		const Transcription::int32_type int32_object =
				static_cast<Transcription::int32_type>(object);

		d_transcription->add_signed_integer(transcribed_object.object_id, int32_object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			try
			{
				// 'short' is 16-bits.
				object = boost::numeric_cast<short>(
						d_transcription->get_signed_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		case Transcription::UNSIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<short>(
						d_transcription->get_unsigned_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		unsigned short &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		// 'unsigned short' is 16-bits.
		const Transcription::uint32_type uint32_object =
				static_cast<Transcription::uint32_type>(object);

		d_transcription->add_unsigned_integer(transcribed_object.object_id, uint32_object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<unsigned short>(
						d_transcription->get_signed_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		case Transcription::UNSIGNED_INTEGER:
			try
			{
				// 'unsigned short' is 16-bits.
				object = boost::numeric_cast<unsigned short>(
						d_transcription->get_unsigned_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		int &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		// 'int' is 32-bits or less.
		const Transcription::int32_type int32_object =
				static_cast<Transcription::int32_type>(object);

		d_transcription->add_signed_integer(transcribed_object.object_id, int32_object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			try
			{
				// In case 'int' is 16-bits.
				object = boost::numeric_cast<int>(
						d_transcription->get_signed_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		case Transcription::UNSIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<int>(
						d_transcription->get_unsigned_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		unsigned int &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		// 'unsigned int' is 32-bits or less.
		const Transcription::uint32_type uint32_object =
				static_cast<Transcription::uint32_type>(object);

		d_transcription->add_unsigned_integer(transcribed_object.object_id, uint32_object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<unsigned int>(
						d_transcription->get_signed_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		case Transcription::UNSIGNED_INTEGER:
			try
			{
				// In case 'unsigned int' is 16-bits.
				object = boost::numeric_cast<unsigned int>(
						d_transcription->get_unsigned_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		long &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		// If we get any 'long' values that are greater than the range of 'int32_type'
		// then we'll get a 'boost::numeric::bad_numeric_cast' exception.
		Transcription::int32_type int32_object;
		try
		{
			int32_object = boost::numeric_cast<Transcription::int32_type>(object);
		}
		catch (boost::numeric::bad_numeric_cast &)
		{
			// Throw as one of our exceptions instead.
			GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
					false,
					GPLATES_ASSERTION_SOURCE,
					"'long' value is out of range of 32-bit signed integer.");
		}

		d_transcription->add_signed_integer(transcribed_object.object_id, int32_object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			// No overflow possible for 'int32_type' to 'long'
			// since latter is at least 32-bits (C++ standard).
			object = d_transcription->get_signed_integer(transcribed_object.object_id);
			break;

		case Transcription::UNSIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<long>(
						d_transcription->get_unsigned_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		unsigned long &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		// If we get any 'unsigned long' values that are greater than the range of 'uint32_type'
		// then we'll get a 'boost::numeric::bad_numeric_cast' exception.
		Transcription::uint32_type uint32_object;
		try
		{
			uint32_object = boost::numeric_cast<Transcription::uint32_type>(object);
		}
		catch (boost::numeric::bad_numeric_cast &)
		{
			// Throw as one of our exceptions instead.
			GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
					false,
					GPLATES_ASSERTION_SOURCE,
					"'unsigned long' value is out of range of 32-bit unsigned integer.");
		}

		d_transcription->add_unsigned_integer(transcribed_object.object_id, uint32_object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<unsigned long>(
						d_transcription->get_signed_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		case Transcription::UNSIGNED_INTEGER:
			// No overflow possible for 'uint32_type' to 'unsigned long'
			// since latter is at least 32-bits (C++ standard).
			object = d_transcription->get_unsigned_integer(transcribed_object.object_id);
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		float &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		d_transcription->add_float(transcribed_object.object_id, object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<float>(
						d_transcription->get_signed_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		case Transcription::UNSIGNED_INTEGER:
			try
			{
				object = boost::numeric_cast<float>(
						d_transcription->get_unsigned_integer(transcribed_object.object_id));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		case Transcription::FLOAT:
			object = d_transcription->get_float(transcribed_object.object_id);
			break;

		case Transcription::DOUBLE:
			try
			{
				const double double_object = d_transcription->get_double(transcribed_object.object_id);

				// 'boost::numeric_cast' from double to float will not cast Infinity and NaN
				// from double to float so we use a regular cast for that.
				if (GPlatesMaths::is_finite(double_object))
				{
					object = boost::numeric_cast<float>(double_object);
				}
				else
				{
					object = static_cast<float>(double_object);
				}
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				// Range overflow.
				return false;
			}
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		double &object)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			!transcribed_object.object_category,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to transcribe the same object twice.");

	if (is_saving())
	{
		d_transcription->add_double(transcribed_object.object_id, object);
	}
	else // loading...
	{
		const Transcription::ObjectType object_type =
				d_transcription->get_object_type(transcribed_object.object_id);

		// No range overflow possible converting to 'double'.
		switch (object_type)
		{
		case Transcription::SIGNED_INTEGER:
			object = d_transcription->get_signed_integer(transcribed_object.object_id);
			break;

		case Transcription::UNSIGNED_INTEGER:
			object = d_transcription->get_unsigned_integer(transcribed_object.object_id);
			break;

		case Transcription::FLOAT:
			object = d_transcription->get_float(transcribed_object.object_id);
			break;

		case Transcription::DOUBLE:
			object = d_transcription->get_double(transcribed_object.object_id);
			break;

		default:
			return false;
		}
	}

	transcribed_object.object_category = TranscribedObject::PRIMITIVE;

	return true;
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe(
		long double &object)
{
	double double_object;

	if (is_saving())
	{
		// If we get any 'long double' values that are greater than the range of 'double'
		// then we'll get a 'boost::numeric::bad_numeric_cast' exception.
		try
		{
			double_object = boost::numeric_cast<double>(object);
		}
		catch (boost::numeric::bad_numeric_cast &)
		{
			// Throw as one of our exceptions instead.
			GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
					false,
					GPLATES_ASSERTION_SOURCE,
					"'long double' value is out of range of 'double'.");
		}
	}

	// Re-use 'transcribe()' for 'double'.
	// Ie, we treat 'long double' as 'double' - both are actually the same on Windows.
	if (!transcribe(double_object))
	{
		return false;
	}

	if (is_loading())
	{
		object = double_object;
	}

	return true;
}


void
GPlatesScribe::TranscriptionScribeContext::save_tag_section(
		const std::string &tag_name,
		unsigned int tag_version,
		Transcription::CompositeObject *&section_composite_object,
		boost::optional<object_id_type &> object_id)
{
	// Convert the section tag name/version into an object key.
	const Transcription::object_key_type section_key =
			d_transcription->get_or_create_object_key(tag_name, tag_version);

	const unsigned int num_children_with_key =
			section_composite_object->get_num_children_with_key(section_key);

	if (object_id)
	{
		// Should not have any children yet.
		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				num_children_with_key == 0,
				GPLATES_ASSERTION_SOURCE,
				std::string("An object has already been saved using the same object tag '") + tag_name + "'");

		// Save the object id associated with the object key.
		section_composite_object->set_child(section_key, object_id.get());

		return;
	}
	// else not the last section ...

	if (num_children_with_key == 0)
	{
		// There are no children for the current section tag which means this is the
		// first time we've visited this section, so create a new composite object
		// for the next section.
		const object_id_type next_section_object_id = allocate_save_object_id();
		Transcription::CompositeObject &next_section_composite_object =
				d_transcription->add_composite_object(next_section_object_id);
		section_composite_object->set_child(section_key, next_section_object_id);

		// Move onto the next section.
		section_composite_object = &next_section_composite_object;

		return;
	}

	// Should only have one child. If there are more then the tag is being
	// used to store an array, but this is very unlikely to happen.
	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			num_children_with_key == 1,
			GPLATES_ASSERTION_SOURCE,
			std::string("Object tag '") + tag_name + "' already used for an array - so cannot use for non-array.");

	// We've visited the next section before so we just re-use it because we're just trying to
	// traverse through the object tag until we get to the end of it and can transcribe the object ID.
	const object_id_type next_section_object_id =
			section_composite_object->get_child(section_key);

	// We've visited the next section before so we don't need to create a
	// composite object for it - just retrieve the previously created one.
	//
	// It's possible a composite object has not yet been created (even though
	// its object ID has been transcribed). If that's the case then it's an
	// error in using the scribe system (and an exception will be thrown).
	// It shouldn't happen if the scribe system is used correctly.
	Transcription::CompositeObject &next_section_composite_object =
			d_transcription->get_composite_object(next_section_object_id);

	// Move onto the next section.
	section_composite_object = &next_section_composite_object;
}


bool
GPlatesScribe::TranscriptionScribeContext::load_tag_section(
		const std::string &tag_name,
		unsigned int tag_version,
		Transcription::CompositeObject *&section_composite_object,
		boost::optional<object_id_type &> object_id) const
{
	// Convert the section tag name/version into an object key.
	boost::optional<Transcription::object_key_type> section_key =
			d_transcription->get_object_key(tag_name, tag_version);

	// If the object key doesn't even exist across all transcribed objects then
	// it won't exist as an object key of the current composite object.
	if (!section_key)
	{
		return false;
	}

	const unsigned int num_children_with_key =
			section_composite_object->get_num_children_with_key(section_key.get());

	if (num_children_with_key != 1)
	{
		// Either couldn't find section key or there were multiple children.
		// The latter should have been caught when the transcription was written.
		return false;
	}
	// else there was exactly one child object corresponding to the next section ...

	if (object_id)
	{
		// Load the object id associated with the object key.
		object_id.get() = section_composite_object->get_child(section_key.get());

		return true;
	}
	// else not the last section ...

	const object_id_type next_section_object_id = section_composite_object->get_child(section_key.get());

	// Check the object type is a composite.
	// The object type retrieval itself shouldn't throw because the transcription
	// should be complete (when it was written in the save path it was tested for
	// completeness which ensures no composites reference dangling child object IDs).
	if (d_transcription->get_object_type(next_section_object_id) != Transcription::COMPOSITE)
	{
		// We're expecting a composite object.
		return false;
	}

	Transcription::CompositeObject &next_section_composite_object =
			d_transcription->get_composite_object(next_section_object_id);

	// Move onto the next section.
	section_composite_object = &next_section_composite_object;

	return true;
}


void
GPlatesScribe::TranscriptionScribeContext::save_array_index_section(
		const std::string &array_item_tag_name,
		unsigned int array_item_tag_version,
		unsigned int array_index,
		Transcription::CompositeObject *&section_composite_object,
		boost::optional<object_id_type &> object_id)
{
	// Convert the array item name/version into an object key.
	const Transcription::object_key_type array_item_key =
			d_transcription->get_or_create_object_key(array_item_tag_name, array_item_tag_version);

	// Set the array item (either the next section or the actual object being transcribed if last section).
	if (object_id)
	{
		// Should not already have a child at the requested array index.
		if (section_composite_object->has_valid_child(array_item_key, array_index))
		{
			std::stringstream string_stream;
			string_stream << "An object has already been saved using the same object tag '"
					<< array_item_tag_name << "' and array index '" << array_index << "'.";

			GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
					false,
					GPLATES_ASSERTION_SOURCE,
					string_stream.str());
		}

		// Save the object id at the requested array index (the index of child associated
		// with the array item key).
		section_composite_object->set_child(array_item_key, object_id.get(), array_index);

		return;
	}
	// else not the last section ...

	boost::optional<object_id_type> next_section_object_id =
			section_composite_object->has_valid_child(array_item_key, array_index);
	if (!next_section_object_id)
	{
		next_section_object_id = allocate_save_object_id();
		Transcription::CompositeObject &next_section_composite_object =
				d_transcription->add_composite_object(next_section_object_id.get());
		section_composite_object->set_child(array_item_key, next_section_object_id.get(), array_index);

		// Move onto the next section.
		section_composite_object = &next_section_composite_object;

		return;
	}

	// Move onto the next section.
	section_composite_object = &d_transcription->get_composite_object(next_section_object_id.get());
}


bool
GPlatesScribe::TranscriptionScribeContext::load_array_index_section(
		const std::string &array_item_tag_name,
		unsigned int array_item_tag_version,
		unsigned int array_index,
		Transcription::CompositeObject *&section_composite_object,
		boost::optional<object_id_type &> object_id) const
{
	// Convert the array item name/version into an object key.
	boost::optional<Transcription::object_key_type> array_item_key =
			d_transcription->get_object_key(array_item_tag_name, array_item_tag_version);

	// If the object key doesn't even exist across all transcribed objects then
	// it won't exist as an object key of the current composite object.
	if (!array_item_key)
	{
		return false;
	}

	// Get the array item (either the next section or the actual object being transcribed if last section).
	if (object_id)
	{
		boost::optional<object_id_type> object_id_opt =
				section_composite_object->has_valid_child(array_item_key.get(), array_index);
		if (!object_id_opt)
		{
			return false;
		}

		object_id.get() = object_id_opt.get();

		return true;
	}
	// else not the last section ...

	boost::optional<object_id_type> next_section_object_id =
			section_composite_object->has_valid_child(array_item_key.get(), array_index);
	if (!next_section_object_id)
	{
		return false;
	}

	// Check the object type is a composite.
	// The object type retrieval itself shouldn't throw because the transcription
	// should be complete (when it was written in the save path it was tested for
	// completeness which ensures no composites reference dangling child object IDs).
	if (d_transcription->get_object_type(next_section_object_id.get()) != Transcription::COMPOSITE)
	{
		// We're expecting a composite object.
		return false;
	}

	Transcription::CompositeObject &next_section_composite_object =
			d_transcription->get_composite_object(next_section_object_id.get());

	// Move onto the next section.
	section_composite_object = &next_section_composite_object;

	return true;
}


void
GPlatesScribe::TranscriptionScribeContext::save_array_size_section(
		const std::string &array_size_tag_name,
		unsigned int array_size_tag_version,
		Transcription::CompositeObject *&section_composite_object,
		boost::optional<object_id_type &> object_id)
{
	// Convert the array size name/version into an object key.
	const Transcription::object_key_type array_size_key =
			d_transcription->get_or_create_object_key(array_size_tag_name, array_size_tag_version);

	const unsigned int num_children_with_array_size_key =
			section_composite_object->get_num_children_with_key(array_size_key);

	// Should not have any children yet.
	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			num_children_with_array_size_key == 0,
			GPLATES_ASSERTION_SOURCE,
			std::string("An object has already been saved using the same object tag '") + array_size_tag_name + "'");

	// The array size section should be the last section in the object tag.
	// Class ObjectTag should ensure that.
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_id,
			GPLATES_ASSERTION_SOURCE,
			"Expecting object tag array length to be the last section.");

	section_composite_object->set_child(array_size_key, object_id.get());
}


bool
GPlatesScribe::TranscriptionScribeContext::load_array_size_section(
		const std::string &array_size_tag_name,
		unsigned int array_size_tag_version,
		Transcription::CompositeObject *&section_composite_object,
		boost::optional<object_id_type &> object_id) const
{
	// Convert the array size name/version into an object key.
	boost::optional<Transcription::object_key_type> array_size_key =
			d_transcription->get_object_key(array_size_tag_name, array_size_tag_version);

	// If the object key doesn't even exist across all transcribed objects then
	// it won't exist as an object key of the current composite object.
	if (!array_size_key)
	{
		return false;
	}

	const unsigned int num_children_with_array_size_key =
			section_composite_object->get_num_children_with_key(array_size_key.get());

	// Should only have one child associated with array size tag.
	if (num_children_with_array_size_key != 1)
	{
		return false;
	}

	// The array size section should be the last section in the object tag.
	// Class ObjectTag should ensure that.
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_id,
			GPLATES_ASSERTION_SOURCE,
			"Expecting object tag array length to be the last section.");

	object_id.get() = section_composite_object->get_child(array_size_key.get());

	return true;
}
