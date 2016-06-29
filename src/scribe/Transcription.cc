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

#include <cmath>
#include <limits>
#include <boost/optional.hpp>
#include <QDebug>
#include <QString>

#include "Transcription.h"

#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"


namespace
{
	/**
	 * Returns true if two floating-point numbers are almost equal.
	 */
	bool
	are_almost_equal(
			const double value1,
			const double value2,
			const double max_relative_error)
	{
		if (GPlatesMaths::is_finite(value1))
		{
			if (!GPlatesMaths::is_finite(value2))
			{
				return false;
			}

			// Avoid divide-by-zero later (which would have happened if both values are zero).
			if (std::fabs(value1 - value2) < (std::numeric_limits<double>::min)())
			{
				return true;
			}

			const double relative_error = (std::fabs(value1) > std::fabs(value2))
					? std::fabs((value1 - value2) / value1)
					: std::fabs((value1 - value2) / value2);

			return relative_error <= max_relative_error;
		}
		else if (GPlatesMaths::is_positive_infinity(value1))
		{
			return GPlatesMaths::is_positive_infinity(value2);
		}
		else if (GPlatesMaths::is_negative_infinity(value1))
		{
			return GPlatesMaths::is_negative_infinity(value2);
		}
		else // NaN ...
		{
			return GPlatesMaths::is_nan(value2);
		}
	}
}


// Using the maximum integer value since it is too high to ever get used by client.
const GPlatesScribe::Transcription::object_id_type GPlatesScribe::Transcription::UNUSED_OBJECT_ID =
		(std::numeric_limits<GPlatesScribe::Transcription::object_id_type>::max)();


GPlatesScribe::Transcription::object_id_type
GPlatesScribe::Transcription::get_num_object_ids() const
{
	return d_object_locations.size();
}


GPlatesScribe::Transcription::ObjectType
GPlatesScribe::Transcription::get_object_type(
		object_id_type object_id) const
{
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_id < d_object_locations.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object id is out of bounds.");

	return d_object_locations[object_id].type;
}


GPlatesScribe::Transcription::int32_type
GPlatesScribe::Transcription::get_signed_integer(
		object_id_type object_id) const
{
	const ObjectLocation &object_location = get_object_location(object_id, SIGNED_INTEGER);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.index < d_signed_integer_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object index out of bounds.");

	return d_signed_integer_objects[object_location.index];
}


void
GPlatesScribe::Transcription::add_signed_integer(
		object_id_type object_id,
		int32_type value)
{
	ObjectLocation &object_location = add_object_location(object_id, SIGNED_INTEGER);

	object_location.index = d_signed_integer_objects.size();

	d_signed_integer_objects.push_back(value);
}


void
GPlatesScribe::Transcription::set_signed_integer(
		object_id_type object_id,
		int32_type value)
{
	ObjectLocation &object_location = get_object_location(object_id, SIGNED_INTEGER);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.index < d_signed_integer_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object index out of bounds.");

	d_signed_integer_objects[object_location.index] = value;
}


GPlatesScribe::Transcription::uint32_type
GPlatesScribe::Transcription::get_unsigned_integer(
		object_id_type object_id) const
{
	const ObjectLocation &object_location = get_object_location(object_id, UNSIGNED_INTEGER);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.index < d_unsigned_integer_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object index out of bounds.");

	return d_unsigned_integer_objects[object_location.index];
}


void
GPlatesScribe::Transcription::add_unsigned_integer(
		object_id_type object_id,
		uint32_type value)
{
	ObjectLocation &object_location = add_object_location(object_id, UNSIGNED_INTEGER);

	object_location.index = d_unsigned_integer_objects.size();

	d_unsigned_integer_objects.push_back(value);
}


void
GPlatesScribe::Transcription::set_unsigned_integer(
		object_id_type object_id,
		uint32_type value)
{
	ObjectLocation &object_location = get_object_location(object_id, UNSIGNED_INTEGER);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.index < d_unsigned_integer_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object index out of bounds.");

	d_unsigned_integer_objects[object_location.index] = value;
}


float
GPlatesScribe::Transcription::get_float(
		object_id_type object_id) const
{
	const ObjectLocation &object_location = get_object_location(object_id, FLOAT);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.index < d_float_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object index out of bounds.");

	return d_float_objects[object_location.index];
}


void
GPlatesScribe::Transcription::add_float(
		object_id_type object_id,
		float value)
{
	ObjectLocation &object_location = add_object_location(object_id, FLOAT);

	object_location.index = d_float_objects.size();

	d_float_objects.push_back(value);
}


double
GPlatesScribe::Transcription::get_double(
		object_id_type object_id) const
{
	const ObjectLocation &object_location = get_object_location(object_id, DOUBLE);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.index < d_double_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object index out of bounds.");

	return d_double_objects[object_location.index];
}


void
GPlatesScribe::Transcription::add_double(
		object_id_type object_id,
		const double &value)
{
	ObjectLocation &object_location = add_object_location(object_id, DOUBLE);

	object_location.index = d_double_objects.size();

	d_double_objects.push_back(value);
}


std::string
GPlatesScribe::Transcription::get_string(
		object_id_type object_id) const
{
	const unsigned int unique_string_index = get_string_object(object_id);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			unique_string_index < d_unique_string_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"String object index out of bounds.");

	return d_unique_string_objects[unique_string_index];
}


void
GPlatesScribe::Transcription::add_string(
		object_id_type object_id,
		const std::string &value)
{
	// Find the object tag in the list of unique tags encountered so far.
	const std::pair<string_object_index_map_type::iterator, bool> string_object_inserted =
			d_string_object_index_map.insert(
					string_object_index_map_type::value_type(
							value,
							0/*dummy*/));

	if (string_object_inserted.second)
	{
		// Insertion was successful so create a new string object entry.

		const unsigned int string_object_index = d_unique_string_objects.size();

		d_unique_string_objects.push_back(value);

		// Assign new string object index to the inserted map entry.
		string_object_inserted.first->second = string_object_index;
	}

	const unsigned int unique_string_index = string_object_inserted.first->second;

	add_string_object(object_id, unique_string_index);
}


GPlatesScribe::Transcription::CompositeObject &
GPlatesScribe::Transcription::get_composite_object(
		object_id_type object_id)
{
	return const_cast<CompositeObject &>(
			static_cast<const Transcription *>(this)->get_composite_object(object_id));
}


const GPlatesScribe::Transcription::CompositeObject &
GPlatesScribe::Transcription::get_composite_object(
		object_id_type object_id) const
{
	const ObjectLocation &object_location = get_object_location(object_id, COMPOSITE);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.index < d_composite_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object index out of bounds.");

	return *d_composite_objects[object_location.index];
}


GPlatesScribe::Transcription::CompositeObject &
GPlatesScribe::Transcription::add_composite_object(
		object_id_type object_id)
{
	ObjectLocation &object_location = add_object_location(object_id, COMPOSITE);

	object_location.index = d_composite_objects.size();

	CompositeObject *composite_object = d_composite_object_pool.construct();
	d_composite_objects.push_back(composite_object);

	return *composite_object;
}


const GPlatesScribe::Transcription::object_tag_name_type &
GPlatesScribe::Transcription::get_object_tag_name(
		object_tag_name_id_type object_tag_name_id) const
{
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_tag_name_id < d_object_tag_names.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object tag name index out of bounds.");

	return d_object_tag_names[object_tag_name_id];
}


GPlatesScribe::Transcription::object_tag_name_id_type
GPlatesScribe::Transcription::add_object_tag_name(
		const object_tag_name_type &object_tag_name)
{
	const object_tag_name_id_type object_tag_name_id = d_object_tag_names.size();

	// Add the object tag name to the list of unique tag names.
	const std::pair<object_tag_name_id_map_type::iterator, bool> object_tag_name_inserted =
			d_object_tag_name_id_map.insert(
					object_tag_name_id_map_type::value_type(
							object_tag_name,
							object_tag_name_id));

	// Object tag must be unique (must not already exist).
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_tag_name_inserted.second,
			GPLATES_ASSERTION_SOURCE,
			"Added object tag names must be unique.");

	d_object_tag_names.push_back(object_tag_name);

	return object_tag_name_id;
}


const std::string &
GPlatesScribe::Transcription::get_unique_string_object(
		unsigned int unique_string_index) const
{
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			unique_string_index < d_unique_string_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"Unique string object index out of bounds.");

	return d_unique_string_objects[unique_string_index];
}


unsigned int
GPlatesScribe::Transcription::add_unique_string_object(
		const std::string &unique_string_object)
{
	const unsigned int unique_string_index = d_unique_string_objects.size();

	// Add the unique string object to the list of unique string objects.
	const std::pair<object_tag_name_id_map_type::iterator, bool> object_tag_inserted =
			d_string_object_index_map.insert(
					object_tag_name_id_map_type::value_type(
							unique_string_object,
							unique_string_index));

	// String object must be unique (must not already exist).
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_tag_inserted.second,
			GPLATES_ASSERTION_SOURCE,
			"Added string objects must be unique.");

	d_unique_string_objects.push_back(unique_string_object);

	return unique_string_index;
}


unsigned int
GPlatesScribe::Transcription::get_string_object(
		object_id_type object_id) const
{
	const ObjectLocation &object_location = get_object_location(object_id, STRING);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.index < d_string_objects.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object index out of bounds.");

	// Get the index to the unique string object.
	return d_string_objects[object_location.index];
}


void
GPlatesScribe::Transcription::add_string_object(
		object_id_type object_id,
		unsigned int unique_string_index)
{
	ObjectLocation &object_location = add_object_location(object_id, STRING);

	object_location.index = d_string_objects.size();

	// Add the index to the unique string object.
	d_string_objects.push_back(unique_string_index);
}


boost::optional<GPlatesScribe::Transcription::object_key_type>
GPlatesScribe::Transcription::get_object_key(
		const object_tag_name_type &object_tag_name,
		object_tag_version_type object_tag_version) const
{
	// Find the object tag name in the list of unique tag names encountered so far.
	const object_tag_name_id_map_type::const_iterator object_tag_name_iter =
			d_object_tag_name_id_map.find(object_tag_name);

	if (object_tag_name_iter == d_object_tag_name_id_map.end())
	{
		return boost::none;
	}

	const object_tag_name_id_type object_tag_name_id = object_tag_name_iter->second;

	return object_key_type(object_tag_name_id, object_tag_version);
}


GPlatesScribe::Transcription::object_key_type
GPlatesScribe::Transcription::get_or_create_object_key(
		const object_tag_name_type &object_tag_name,
		object_tag_version_type object_tag_version)
{
	// Find the object tag name in the list of unique tag names encountered so far.
	const std::pair<object_tag_name_id_map_type::iterator, bool> object_tag_name_inserted =
			d_object_tag_name_id_map.insert(
					object_tag_name_id_map_type::value_type(
							object_tag_name,
							0/*dummy*/));

	if (object_tag_name_inserted.second)
	{
		// Insertion was successful so create a new object tag name entry.

		const object_tag_name_id_type object_tag_name_id = d_object_tag_names.size();

		d_object_tag_names.push_back(object_tag_name);

		// Assign new object tag id to the inserted map entry.
		object_tag_name_inserted.first->second = object_tag_name_id;
	}

	const object_tag_name_id_type object_tag_name_id = object_tag_name_inserted.first->second;

	return object_key_type(object_tag_name_id, object_tag_version);
}


bool
GPlatesScribe::Transcription::is_complete(
		object_id_type null_pointer_object_id,
		bool emit_warnings) const
{
	bool transcription_complete = true;

	const object_id_type num_object_ids = get_num_object_ids();
	for (object_id_type object_id = 0; object_id < num_object_ids; ++object_id)
	{
		const ObjectType object_type = get_object_type(object_id);

		// Ignore object slots that were never used.
		//
		// It should pretty much just be the null-pointer object id (of zero)
		// reserved in class Scribe that is unused.
		if (object_type == UNUSED)
		{
			continue;
		}

		// Check the child object id references in composite objects.
		if (object_type == COMPOSITE)
		{
			const CompositeObject &composite_object = get_composite_object(object_id);

			// Iterate over child object keys.
			const unsigned int num_keys = composite_object.get_num_keys();
			for (unsigned int key_index = 0; key_index < num_keys; ++key_index)
			{
				const object_key_type object_key = composite_object.get_key(key_index);

				// All children with the same object key must have the same object type.
				// This is because the object key refers to a homogenous sequence/array.
				boost::optional<ObjectType> child_object_type;

				// Iterate over children associated with current child object key.
				const unsigned int num_children_with_key = composite_object.get_num_children_with_key(object_key);
				for (unsigned int child_index = 0; child_index < num_children_with_key; ++child_index)
				{
					// See if referencing a valid child object ID.
					// This means the child object ID is something other than UNUSED_OBJECT_ID.
					boost::optional<object_id_type> child_object_id_opt =
							composite_object.has_valid_child(object_key, child_index);
					if (!child_object_id_opt)
					{
						transcription_complete = false;

						if (emit_warnings)
						{
							qWarning() << "Transcription parent object id" << object_id
									<< "has unused reference for child object number "
									<< child_index + 1 << " under "
									<< QString::fromLatin1(get_object_tag_name(object_key.first).c_str())
									<< ".";
						}

						continue;
					}
					const object_id_type child_object_id = child_object_id_opt.get();

					// Ignore NULL pointers - they don't reference an object.
					if (child_object_id == null_pointer_object_id)
					{
						continue;
					}

					// Ignore NULL pointers - they don't reference an object.
					if (child_object_id == null_pointer_object_id)
					{
						continue;
					}

					// Get the current child object type unless child object ID is out-of-bounds.
					boost::optional<ObjectType> current_child_object_type;
					if (child_object_id < num_object_ids)
					{
						current_child_object_type = get_object_type(child_object_id);
					}

					if (current_child_object_type)
					{
						// Initialise child object type if first child so far.
						// Otherwise make sure all children have same object type because the
						// object key refers to a homogenous sequence/array.
						if (!child_object_type)
						{
							child_object_type = current_child_object_type.get();
						}
						else if (child_object_type.get() != current_child_object_type.get())
						{
							transcription_complete = false;

							if (emit_warnings)
							{
								qWarning() << "Transcription parent object id" << object_id
										<< "referencing child object number " << child_index + 1 << " under "
										<< QString::fromLatin1(get_object_tag_name(object_key.first).c_str())
										<< " as a different object type than the first child object.";
							}
						}
					}

					// Make sure current child exists in transcription.
					if (!current_child_object_type ||
						current_child_object_type.get() == UNUSED)
					{
						transcription_complete = false;

						if (emit_warnings)
						{
							qWarning() << "Transcription object id" << child_object_id
									<< "referenced as"
									<< QString::fromLatin1(get_object_tag_name(object_key.first).c_str())
									<< "from parent object id" << object_id
									<< "was not found in the transcription.";
						}
					}
				}
			}
		}

		// Continue on to the next object so that we log warnings for all incomplete objects.
	}

	return transcription_complete;
}


bool
GPlatesScribe::Transcription::operator==(
		const Transcription &other) const
{
	const unsigned int num_object_tags = get_num_object_tags();
	if (num_object_tags != other.get_num_object_tags())
	{
		return false;
	}
	for (unsigned int object_tag_name_id = 0; object_tag_name_id < num_object_tags; ++object_tag_name_id)
	{
		const object_tag_name_type &object_tag_name = get_object_tag_name(object_tag_name_id);
		if (object_tag_name != other.get_object_tag_name(object_tag_name_id))
		{
			return false;
		}
	}

	const unsigned int num_unique_strings = get_num_unique_string_objects();
	if (num_unique_strings != other.get_num_unique_string_objects())
	{
		return false;
	}
	for (unsigned int unique_string_index = 0; unique_string_index < num_unique_strings; ++unique_string_index)
	{
		const std::string &unique_string = get_unique_string_object(unique_string_index);
		if (unique_string != other.get_unique_string_object(unique_string_index))
		{
			return false;
		}
	}

	const Transcription::object_id_type num_object_ids = get_num_object_ids();
	if (num_object_ids != other.get_num_object_ids())
	{
		return false;
	}

	for (object_id_type object_id = 0; object_id < num_object_ids; ++object_id)
	{
		const ObjectType object_type = get_object_type(object_id);
		if (object_type != other.get_object_type(object_id))
		{
			return false;
		}

		// Ignore object slots that were never used.
		//
		// It should pretty much just be the null-pointer object id (of zero)
		// reserved in class Scribe that is unused.
		if (object_type == UNUSED)
		{
			continue;
		}

		switch (object_type)
		{
		case SIGNED_INTEGER:
			if (get_signed_integer(object_id) != other.get_signed_integer(object_id))
			{
				return false;
			}
			break;

		case UNSIGNED_INTEGER:
			if (get_unsigned_integer(object_id) != other.get_unsigned_integer(object_id))
			{
				return false;
			}
			break;

		case FLOAT:
			if (!are_almost_equal(get_float(object_id), other.get_float(object_id), 1e-5))
			{
				return false;
			}
			break;

		case DOUBLE:
			if (!are_almost_equal(get_double(object_id), other.get_double(object_id), 1e-12))
			{
				return false;
			}
			break;

		case STRING:
			if (get_string_object(object_id) != other.get_string_object(object_id))
			{
				return false;
			}
			break;

		case COMPOSITE:
			{
				const CompositeObject &composite_object = get_composite_object(object_id);
				const CompositeObject &other_composite_object = other.get_composite_object(object_id);

				const unsigned int num_keys = composite_object.get_num_keys();
				if (num_keys != other_composite_object.get_num_keys())
				{
					return false;
				}

				// Iterate over child object keys.
				for (unsigned int key_index = 0; key_index < num_keys; ++key_index)
				{
					const object_key_type object_key = composite_object.get_key(key_index);
					if (object_key != other_composite_object.get_key(key_index))
					{
						return false;
					}

					const unsigned int num_children_with_key = composite_object.get_num_children_with_key(object_key);
					if (num_children_with_key != other_composite_object.get_num_children_with_key(object_key))
					{
						return false;
					}

					// Iterate over children associated with current child object key.
					for (unsigned int child_index = 0; child_index < num_children_with_key; ++child_index)
					{
						// See if referencing a valid child object ID.
						// This means the child object ID is something other than UNUSED_OBJECT_ID.
						boost::optional<object_id_type> child_object_id =
								composite_object.has_valid_child(object_key, child_index);
						if (child_object_id != other_composite_object.has_valid_child(object_key, child_index))
						{
							return false;
						}
					}
				}
			}
			break;

		default:
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			break;
		}
	}

	return true;
}


const GPlatesScribe::Transcription::ObjectLocation &
GPlatesScribe::Transcription::get_object_location(
		object_id_type object_id,
		ObjectType object_type) const
{
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_id < d_object_locations.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object id is out of bounds.");

	const ObjectLocation &object_location = d_object_locations[object_id];

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.type == object_type,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to retrieve object of incorrect type.");

	return object_location;
}


GPlatesScribe::Transcription::ObjectLocation &
GPlatesScribe::Transcription::get_object_location(
		object_id_type object_id,
		ObjectType object_type)
{
	return const_cast<ObjectLocation &>(
			static_cast<const Transcription *>(this)->get_object_location(object_id, object_type));
}


GPlatesScribe::Transcription::ObjectLocation &
GPlatesScribe::Transcription::add_object_location(
		object_id_type object_id,
		ObjectType object_type)
{
	if (object_id >= d_object_locations.size())
	{
		d_object_locations.resize(object_id + 1);
	}

	ObjectLocation &object_location = d_object_locations[object_id];

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_location.type == UNUSED,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to add the same object twice.");

	object_location.type = object_type;

	return object_location;
}


unsigned int
GPlatesScribe::Transcription::CompositeObject::get_num_keys() const
{
	const unsigned int encoding_size = d_encoding.size();

	unsigned int num_keys = 0;

	for (unsigned int encoding_index = 0; encoding_index != encoding_size; ++num_keys)
	{
		// Should have at least one child object id along with key info.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				encoding_index + OBJECT_KEY_INFO_SIZE < encoding_size,
				GPLATES_ASSERTION_SOURCE,
				"");

		const unsigned int num_children_with_key = d_encoding[encoding_index + NUM_CHILDREN_OFFSET];

		// Skip to the next key, if any.
		encoding_index += OBJECT_KEY_INFO_SIZE + num_children_with_key;

		// Number of child object ids should not exceed encoded information size.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				encoding_index <= encoding_size,
				GPLATES_ASSERTION_SOURCE,
				"");
	}

	return num_keys;
}


GPlatesScribe::Transcription::object_key_type
GPlatesScribe::Transcription::CompositeObject::get_key(
		unsigned int key_index) const
{
	const unsigned int encoding_size = d_encoding.size();

	unsigned int key_count = 0;

	for (unsigned int encoding_index = 0; encoding_index != encoding_size; ++key_count)
	{
		// Should have at least one child object id along with key info.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				encoding_index + OBJECT_KEY_INFO_SIZE < encoding_size,
				GPLATES_ASSERTION_SOURCE,
				"");

		if (key_index == key_count)
		{
			// Read the child object key.
			const object_tag_name_id_type child_object_tag_name_id = d_encoding[encoding_index + OBJECT_TAG_ID_OFFSET];
			const object_tag_version_type child_object_tag_version = d_encoding[encoding_index + OBJECT_TAG_VERSION_OFFSET];

			return std::make_pair(child_object_tag_name_id, child_object_tag_version);
		}

		const unsigned int num_children_with_key = d_encoding[encoding_index + NUM_CHILDREN_OFFSET];

		// Skip to the next key, if any.
		encoding_index += OBJECT_KEY_INFO_SIZE + num_children_with_key;

		// Number of child object ids should not exceed encoded information size.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				encoding_index <= encoding_size,
				GPLATES_ASSERTION_SOURCE,
				"");
	}

	// If we get here then either the requested 'key_index' was exceeded the number of keys or
	// there's a programming error somewhere.
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			false,
			GPLATES_ASSERTION_SOURCE,
			"");

	// Return dummy value to keep compiler happy.
	return object_key_type(0,0);
}


unsigned int
GPlatesScribe::Transcription::CompositeObject::get_num_children_with_key(
		const object_key_type &object_key) const
{
	const boost::optional<unsigned int> encoding_index = find_key(object_key);
	if (!encoding_index)
	{
		return 0;
	}

	return d_encoding[encoding_index.get() + NUM_CHILDREN_OFFSET];
}


boost::optional<GPlatesScribe::Transcription::object_id_type>
GPlatesScribe::Transcription::CompositeObject::has_valid_child(
		const object_key_type &object_key,
		unsigned int index) const
{
	// See if object key exists.
	const boost::optional<unsigned int> encoding_index = find_key(object_key);
	if (!encoding_index)
	{
		return boost::none;
	}

	// See if 'index' is out-of-bounds.
	const unsigned int num_children_with_key = d_encoding[encoding_index.get() + NUM_CHILDREN_OFFSET];
	if (index >= num_children_with_key)
	{
		return boost::none;
	}

	// See if child object ID is unused.
	const object_id_type object_id = d_encoding[encoding_index.get() + OBJECT_KEY_INFO_SIZE + index];
	if (object_id == UNUSED_OBJECT_ID)
	{
		return boost::none;
	}

	return object_id;
}


GPlatesScribe::Transcription::object_id_type
GPlatesScribe::Transcription::CompositeObject::get_child(
		const object_key_type &object_key,
		unsigned int index) const
{
	boost::optional<object_id_type> object_id = has_valid_child(object_key, index);

	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_id,
			GPLATES_ASSERTION_SOURCE,
			"Cannot load object - requested object tag does not exist.");

	return object_id.get();
}


void
GPlatesScribe::Transcription::CompositeObject::set_child(
		const object_key_type &object_key,
		object_id_type object_id,
		unsigned int index)
{
	const boost::optional<unsigned int> encoding_index_opt = find_key(object_key);
	if (!encoding_index_opt)
	{
		const unsigned int num_children_with_key = index + 1;

		// The object key (and associated children) does not yet exist so append it.
		const unsigned int encoding_index = add_key(object_key, num_children_with_key);

		// Add one child with that object key at the requested index.
		d_encoding[encoding_index + OBJECT_KEY_INFO_SIZE + index] = object_id;

		return;
	}
	const unsigned int encoding_index = encoding_index_opt.get();

	unsigned int num_children_with_key = d_encoding[encoding_index + NUM_CHILDREN_OFFSET];

	// Number of child object ids should not exceed encoded information size.
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			encoding_index + OBJECT_KEY_INFO_SIZE + num_children_with_key <= d_encoding.size(),
			GPLATES_ASSERTION_SOURCE,
			"");

	if (index >= num_children_with_key)
	{
		// Make room for the child object and any holes (holes happen if 'index > num_children_with_key')
		// by extending the end of the encoding array associated with the object key.
		d_encoding.insert(
				d_encoding.begin() + encoding_index + OBJECT_KEY_INFO_SIZE + num_children_with_key,
				index - num_children_with_key + 1,
				UNUSED_OBJECT_ID);

		// Set the child object ID.
		d_encoding[encoding_index + OBJECT_KEY_INFO_SIZE + index] = object_id;

		// Set the new number of children with that object key.
		num_children_with_key = index + 1;
		d_encoding[encoding_index + NUM_CHILDREN_OFFSET] = num_children_with_key;
	}
	else // a slot already exists for child at 'index' ...
	{
		// Make sure the slot used for child at 'index' is unused.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				d_encoding[encoding_index + OBJECT_KEY_INFO_SIZE + index] == UNUSED_OBJECT_ID,
				GPLATES_ASSERTION_SOURCE,
				"An object has already been saved using the same object tag (and optional array index).");

		// Set the child object ID.
		d_encoding[encoding_index + OBJECT_KEY_INFO_SIZE + index] = object_id;
	}
}


boost::optional<unsigned int>
GPlatesScribe::Transcription::CompositeObject::find_key(
		const object_key_type &object_key) const
{
	const unsigned int encoding_size = d_encoding.size();

	for (unsigned int encoding_index = 0; encoding_index != encoding_size; )
	{
		// Should have at least one child object id along with key info.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				encoding_index + OBJECT_KEY_INFO_SIZE < encoding_size,
				GPLATES_ASSERTION_SOURCE,
				"");

		// Read the child object key.
		const object_tag_name_id_type child_object_tag_name_id = d_encoding[encoding_index + OBJECT_TAG_ID_OFFSET];
		const object_tag_version_type child_object_tag_version = d_encoding[encoding_index + OBJECT_TAG_VERSION_OFFSET];
		const object_key_type child_object_key = std::make_pair(child_object_tag_name_id, child_object_tag_version);

		const unsigned int num_children_with_key = d_encoding[encoding_index + NUM_CHILDREN_OFFSET];

		// Number of child object ids should not exceed encoded information size.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				encoding_index + OBJECT_KEY_INFO_SIZE + num_children_with_key <= encoding_size,
				GPLATES_ASSERTION_SOURCE,
				"");

		// If object key matches then return.
		// Note that we return after checking that all the key's children fit within the encoding array.
		if (object_key == child_object_key)
		{
			return encoding_index;
		}

		// Skip to the next object key, if any.
		encoding_index += OBJECT_KEY_INFO_SIZE + num_children_with_key;
	}

	return boost::none;
}


unsigned int
GPlatesScribe::Transcription::CompositeObject::add_key(
		const object_key_type &object_key,
		unsigned int num_children_with_key)
{
	// The object key (and any associated children) does not yet exist so append it.
	const unsigned int encoding_index = d_encoding.size();
	d_encoding.resize(
			d_encoding.size() + OBJECT_KEY_INFO_SIZE + num_children_with_key,
			UNUSED_OBJECT_ID);

	// Initialise the object key info fields.
	d_encoding[encoding_index + OBJECT_TAG_ID_OFFSET] = object_key.first;
	d_encoding[encoding_index + OBJECT_TAG_VERSION_OFFSET] = object_key.second;
	d_encoding[encoding_index + NUM_CHILDREN_OFFSET] = num_children_with_key;

	return encoding_index;
}
