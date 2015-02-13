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

#include <QDebug>
#include <QString>

#include "Transcription.h"

#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


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


void
GPlatesScribe::Transcription::add_composite_object(
		object_id_type object_id)
{
	ObjectLocation &object_location = add_object_location(object_id, COMPOSITE);

	object_location.index = d_composite_objects.size();

	CompositeObject *composite_object = d_composite_object_pool.construct();
	d_composite_objects.push_back(composite_object);
}


const GPlatesScribe::Transcription::object_tag_type &
GPlatesScribe::Transcription::get_object_tag(
		object_tag_id_type object_tag_id) const
{
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_tag_id < d_object_tags.size(),
			GPLATES_ASSERTION_SOURCE,
			"Object tag index out of bounds.");

	return d_object_tags[object_tag_id];
}


GPlatesScribe::Transcription::object_tag_id_type
GPlatesScribe::Transcription::add_object_tag(
		const object_tag_type &object_tag)
{
	const object_tag_type object_tag_string(object_tag);

	const object_tag_id_type object_tag_id = d_object_tags.size();

	// Add the object tag to the list of unique tags.
	const std::pair<object_tag_id_map_type::iterator, bool> object_tag_inserted =
			d_object_tag_id_map.insert(
					object_tag_id_map_type::value_type(
							object_tag_string,
							object_tag_id));

	// Object tag must be unique (must not already exist).
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			object_tag_inserted.second,
			GPLATES_ASSERTION_SOURCE,
			"Added object tags must be unique.");

	d_object_tags.push_back(object_tag_string);

	return object_tag_id;
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
	const std::pair<object_tag_id_map_type::iterator, bool> object_tag_inserted =
			d_string_object_index_map.insert(
					object_tag_id_map_type::value_type(
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
		const char *object_tag,
		object_tag_version_type object_tag_version) const
{
	const object_tag_type object_tag_string(object_tag);

	// Find the object tag in the list of unique tags encountered so far.
	const object_tag_id_map_type::const_iterator object_tag_iter =
			d_object_tag_id_map.find(object_tag_string);

	if (object_tag_iter == d_object_tag_id_map.end())
	{
		return boost::none;
	}

	const object_tag_id_type object_tag_id = object_tag_iter->second;

	return object_key_type(object_tag_id, object_tag_version);
}


GPlatesScribe::Transcription::object_key_type
GPlatesScribe::Transcription::get_or_create_object_key(
		const char *object_tag,
		object_tag_version_type object_tag_version)
{
	const object_tag_type object_tag_string(object_tag);

	// Find the object tag in the list of unique tags encountered so far.
	const std::pair<object_tag_id_map_type::iterator, bool> object_tag_inserted =
			d_object_tag_id_map.insert(
					object_tag_id_map_type::value_type(
							object_tag_string,
							0/*dummy*/));

	if (object_tag_inserted.second)
	{
		// Insertion was successful so create a new object tag entry.

		const object_tag_id_type object_tag_id = d_object_tags.size();

		d_object_tags.push_back(object_tag_string);

		// Assign new object tag id to the inserted map entry.
		object_tag_inserted.first->second = object_tag_id;
	}

	const object_tag_id_type object_tag_id = object_tag_inserted.first->second;

	return object_key_type(object_tag_id, object_tag_version);
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

				// Iterate over children associated with current child object key.
				const unsigned int num_children_with_key = composite_object.get_num_children_with_key(object_key);
				for (unsigned int child_index = 0; child_index < num_children_with_key; ++child_index)
				{
					const object_id_type child_object_id =
							composite_object.get_child(object_key, child_index);

					// Ignore NULL pointers - they don't reference an object.
					if (child_object_id == null_pointer_object_id)
					{
						continue;
					}

					// Make sure current child exists in transcription.
					if (child_object_id >= num_object_ids ||
						get_object_type(child_object_id) == UNUSED)
					{
						transcription_complete = false;

						if (emit_warnings)
						{
							qWarning() << "Transcription object id" << child_object_id
									<< "referenced as"
									<< QString::fromLatin1(get_object_tag(object_key.first).c_str())
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
			const object_tag_id_type child_object_tag_id = d_encoding[encoding_index + OBJECT_TAG_ID_OFFSET];
			const object_tag_version_type child_object_tag_version = d_encoding[encoding_index + OBJECT_TAG_VERSION_OFFSET];

			return std::make_pair(child_object_tag_id, child_object_tag_version);
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


GPlatesScribe::Transcription::object_id_type
GPlatesScribe::Transcription::CompositeObject::get_child(
		const object_key_type &object_key,
		unsigned int index) const
{
	const boost::optional<unsigned int> encoding_index = find_key(object_key);

	// Caller should have called 'get_num_children_with_key()' to ensure the object key exists.
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			encoding_index,
			GPLATES_ASSERTION_SOURCE,
			"");

	const unsigned int num_children_with_key = d_encoding[encoding_index.get() + NUM_CHILDREN_OFFSET];

	// The index should not exceed the number of children associated with 'object_key'.
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			index < num_children_with_key,
			GPLATES_ASSERTION_SOURCE,
			"");

	return d_encoding[encoding_index.get() + OBJECT_KEY_INFO_SIZE + index];
}


void
GPlatesScribe::Transcription::CompositeObject::add_child(
		const object_key_type &object_key,
		object_id_type object_id)
{
	const boost::optional<unsigned int> encoding_index_opt = find_key(object_key);
	if (!encoding_index_opt)
	{
		// The object key (and associated child) does not yet exist so append it.
		const unsigned int encoding_index = d_encoding.size();
		d_encoding.resize(d_encoding.size() + OBJECT_KEY_INFO_SIZE + 1);

		// Initialise the object key info fields.
		d_encoding[encoding_index + OBJECT_TAG_ID_OFFSET] = object_key.first;
		d_encoding[encoding_index + OBJECT_TAG_VERSION_OFFSET] = object_key.second;
		d_encoding[encoding_index + NUM_CHILDREN_OFFSET] = 1/*num_children_with_key*/;

		// Add one child with that object key.
		d_encoding[encoding_index + OBJECT_KEY_INFO_SIZE] = object_id;

		return;
	}
	const unsigned int encoding_index = encoding_index_opt.get();

	// The object key already exists.

	unsigned int num_children_with_key = d_encoding[encoding_index + NUM_CHILDREN_OFFSET];

	// Number of child object ids should not exceed encoded information size.
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			encoding_index + OBJECT_KEY_INFO_SIZE + num_children_with_key <= d_encoding.size(),
			GPLATES_ASSERTION_SOURCE,
			"");

	// Add the child object id to the end of the list (associated with 'object_key').
	d_encoding.insert(
			d_encoding.begin() + encoding_index + OBJECT_KEY_INFO_SIZE + num_children_with_key,
			object_id);

	// Increment the number of children with that object key.
	++num_children_with_key;
	d_encoding[encoding_index + NUM_CHILDREN_OFFSET] = num_children_with_key;
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
		const object_tag_id_type child_object_tag_id = d_encoding[encoding_index + OBJECT_TAG_ID_OFFSET];
		const object_tag_version_type child_object_tag_version = d_encoding[encoding_index + OBJECT_TAG_VERSION_OFFSET];
		const object_key_type child_object_key = std::make_pair(child_object_tag_id, child_object_tag_version);

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
