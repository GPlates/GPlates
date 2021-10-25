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

#include <boost/cast.hpp>

#include "TranscriptionScribeContext.h"

#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"


GPlatesScribe::TranscriptionScribeContext::TranscriptionScribeContext(
		const Transcription::non_null_ptr_type &transcription,
		object_id_type root_object_id,
		bool is_saving_) :
	d_is_saving(is_saving_),
	d_transcription(transcription)
{
	// Transcribing at the root scope will require an emulated root composite object.
	//
	// This is so calls such as 'Scribe::transcribe("my_first_object", my_first_object)'
	// will have a place to store the "my_first_object" object tag/key.
	push_transcribed_object(root_object_id);
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
		// This is just so the parent object that references the current object will be able to find something.
		// This usually happens when nothing is transcribed for an object (eg, an empty base class).
		TranscribedObject &transcribed_object = d_transcribed_object_stack.top();
		if (!transcribed_object.object_category)
		{
			d_transcription->add_composite_object(transcribed_object.object_id);
		}
	}

	d_transcribed_object_stack.pop();
}


bool
GPlatesScribe::TranscriptionScribeContext::transcribe_object_id(
		object_id_type &object_id,
		const char *object_tag,
		object_tag_version_type object_tag_version)
{
	TranscribedObject &transcribed_object = d_transcribed_object_stack.top();

	if (transcribed_object.object_category)
	{
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				transcribed_object.object_category.get() == TranscribedObject::COMPOSITE,
				GPLATES_ASSERTION_SOURCE,
				"Attempted to transcribe a child object into a primitive (non-composite object).");
	}
	else
	{
		// This is the first time a child object id is being transcribed into the current object.
		// So that makes the current object a composite object.

		transcribed_object.object_category = TranscribedObject::COMPOSITE;

		if (is_saving())
		{
			d_transcription->add_composite_object(transcribed_object.object_id);
		}
	}

	Transcription::CompositeObject &composite_object =
			d_transcription->get_composite_object(transcribed_object.object_id);

	if (is_saving())
	{
		// Convert the object tag/version into an object key.
		const Transcription::object_key_type object_key =
				d_transcription->get_or_create_object_key(object_tag, object_tag_version);

		// Save the object id associated with the object key.
		// Note that each key can contain multiple object ids.
		// For example, when arrays are transcribed each array element has the same key.
		transcribed_object.save_child_object_id(object_id, object_key, composite_object);
	}
	else // loading...
	{
		// Convert the object tag/version into an object key.
		boost::optional<Transcription::object_key_type> object_key =
				d_transcription->get_object_key(object_tag, object_tag_version);

		// If the object key doesn't even exist across all transcribed objects then
		// it won't exist as an object key of the current composite object.
		if (!object_key)
		{
			return false;
		}

		// Load the next object id associated with the object key.
		// Note that each key can contain multiple object ids.
		// For example, when arrays are transcribed each array element has the same key.
		if (!transcribed_object.load_child_object_id(object_id, object_key.get(), composite_object))
		{
			// Couldn't find object key, or ran out of object ids associated with object key.
			return false;
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
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
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
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
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
				object = boost::numeric_cast<float>(
						d_transcription->get_double(transcribed_object.object_id));
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
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
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


bool
GPlatesScribe::TranscriptionScribeContext::TranscribedObject::load_child_object_id(
		object_id_type &child_object_id,
		const Transcription::object_key_type &object_key,
		const Transcription::CompositeObject &composite_object)
{
	// See if we've already encountered the child object key.
	const std::pair<child_object_id_map_type::iterator, bool> object_key_inserted =
			d_child_object_id_map.insert(
					child_object_id_map_type::value_type(
							object_key,
							range_type(0,0)/*dummy*/));

	if (object_key_inserted.second)
	{
		// Insertion was successful so create a new child object key entry.

		// See if the composite object even has the child object key.
		const unsigned int num_children_with_key =
				composite_object.get_num_children_with_key(object_key);
		if (num_children_with_key == 0)
		{
			// Erase the entry we just inserted since no longer using it.
			d_child_object_id_map.erase(object_key_inserted.first);

			// Composite object has no child with that key.
			return false;
		}

		// Assign new child index range to the inserted map entry.
		object_key_inserted.first->second = range_type(0, num_children_with_key);
	}

	// The range of child object id indices associated with 'object_key'.
	range_type &range = object_key_inserted.first->second;

	// If there are no more child object ids associated with 'object_key' then fail.
	if (range.first == range.second)
	{
		return false;
	}

	// Read the next child object id.
	child_object_id = composite_object.get_child(object_key, range.first/*index*/);

	// Move to the next child object id (associated with 'object_key') for next time.
	++range.first;

	return true;
}
