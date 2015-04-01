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

#ifndef GPLATES_SCRIBE_TRANSCRIPTION_H
#define GPLATES_SCRIBE_TRANSCRIPTION_H

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>

#include "utils/ReferenceCount.h"


namespace GPlatesScribe
{
	/**
	 * The transcribed state of the object network in its most essential and accessible form.
	 *
	 * This state can be written by an archive reader (when transferring state *from* an archive) or
	 * read by an archive writer (when transferring state *to* an archive).
	 *
	 * This state can also be written and read by class TranscriptionScribeContext
	 * (which is, in turn, used by class Scribe) when the object network is transcribed.
	 *
	 * The archives typically store the state in a serialised manner (as a stream) and usually make
	 * some effort to compress the state. The transcription, on the other hand, is accessible
	 * in a random (versus sequential) manner (while still keeping the memory usage down, but not
	 * to the same extent as the archives).
	 */
	class Transcription :
			public GPlatesUtils::ReferenceCount<Transcription>
	{
	public:

		// Convenience typedefs for a shared pointer to a @a Transcription.
		typedef GPlatesUtils::non_null_intrusive_ptr<Transcription> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Transcription> non_null_ptr_to_const_type;


		/**
		 * Typedef for an integer identifier for a transcribed object.
		 */
		typedef unsigned int object_id_type;

		/**
		 * Typedef for an integer object tag version.
		 */
		typedef unsigned int object_tag_version_type;

		/**
		 * Typedef for integer object tag identifier that indexes into the sequence returned
		 * by @a get_object_tag.
		 */
		typedef unsigned int object_tag_id_type;

		/**
		 * Typedef for an object key used to lookup a child object id in @a CompositeObject.
		 */
		typedef std::pair<object_tag_id_type, object_tag_version_type> object_key_type;


		/**
		 * A composite object contains child object ids indexed by @a object_key_type (object tag/version).
		 *
		 * Each child object id, in turn, can be used to identify a primitive or composite object.
		 */
		class CompositeObject
		{
		public:

			/**
			 * Returns the total number of child object keys.
			 *
			 * Returns zero if there are no keys.
			 */
			unsigned int
			get_num_keys() const;

			//! Returns the object key at the specified key index.
			object_key_type
			get_key(
					unsigned int key_index) const;

			/**
			 * Returns the number of child object ids associated with @a object_key.
			 *
			 * Returns zero if there are no keys equal to @a object_key.
			 */
			unsigned int
			get_num_children_with_key(
					const object_key_type &object_key) const;

			/**
			 * Returns the child object id associated with @a object_key and at the specified index.
			 *
			 * Note that there can be more than one child object id associated with a single object key.
			 */
			object_id_type
			get_child(
					const object_key_type &object_key,
					unsigned int index) const;

			//! Adds the specified child object id to be associated with @a object_key.
			void
			add_child(
					const object_key_type &object_key,
					object_id_type object_id);

		private:

			/**
			 * Returns the index into the encoding array at the start of the found key (if found).
			 *
			 * Note that the children of @a object_key are checked to ensure they fit within the encoding array.
			 *
			 * Returns none if object key not found.
			 */
			boost::optional<unsigned int>
			find_key(
					const object_key_type &object_key) const;


			// Offsets from beginning of each object key sub-array in encoding array to the
			// two parts of the object key, the number of children and the first child object id.
			static const unsigned int OBJECT_TAG_ID_OFFSET = 0;
			static const unsigned int OBJECT_TAG_VERSION_OFFSET = 1;
			static const unsigned int NUM_CHILDREN_OFFSET = 2;

			//! Size (in number of integers) of the object key information before the child object ids.
			static const unsigned int OBJECT_KEY_INFO_SIZE = 3;


			/**
			 * The keys and one or more child object ids associated with each key.
			 *
			 * This information is tightly packed into a single array to reduce memory usage.
			 */
			std::vector<unsigned int> d_encoding;
		};


		/**
		 * The types of transcribed objects.
		 */
		enum ObjectType
		{
			SIGNED_INTEGER,
			UNSIGNED_INTEGER,
			FLOAT,
			DOUBLE,
			STRING,
			COMPOSITE,

			UNUSED // Associated with an unused object id.
		};


		//
		// Typedefs for signed/unsigned 32-bit integer types.
		//
		typedef boost::int32_t int32_type;
		typedef boost::uint32_t uint32_type;


		/**
		 * Creates an empty transcription.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new Transcription());
		}


		/**
		 * Returns the number of object ids (including unused ids).
		 */
		object_id_type
		get_num_object_ids() const;

		/**
		 * Returns the type of the transcribed object with the specified object id.
		 *
		 * @a object_id must be less than @a get_num_object_ids.
		 */
		ObjectType
		get_object_type(
				object_id_type object_id) const;


		int32_type
		get_signed_integer(
				object_id_type object_id) const;

		void
		add_signed_integer(
				object_id_type object_id,
				int32_type value);


		uint32_type
		get_unsigned_integer(
				object_id_type object_id) const;

		void
		add_unsigned_integer(
				object_id_type object_id,
				uint32_type value);


		float
		get_float(
				object_id_type object_id) const;

		void
		add_float(
				object_id_type object_id,
				float value);


		double
		get_double(
				object_id_type object_id) const;

		void
		add_double(
				object_id_type object_id,
				const double &value);


		std::string
		get_string(
				object_id_type object_id) const;

		void
		add_string(
				object_id_type object_id,
				const std::string &value);


		CompositeObject &
		get_composite_object(
				object_id_type object_id);

		const CompositeObject &
		get_composite_object(
				object_id_type object_id) const;

		void
		add_composite_object(
				object_id_type object_id);


		//! Typedef for a unique object tag (string).
		typedef std::string object_tag_type;


		/**
		 * Returns the number of unique object tags.
		 *
		 * Used by archive writers.
		 */
		unsigned int
		get_num_object_tags() const
		{
			return d_object_tags.size();
		}

		/**
		 * Returns the unique object tag identified by @a object_tag_id.
		 *
		 * Used by archive writers.
		 */
		const object_tag_type &
		get_object_tag(
				object_tag_id_type object_tag_id) const;


		/**
		 * Adds a unique object tag.
		 *
		 * Used by archive readers.
		 *
		 * Note that all added object tags must be unique and they must be added
		 * in the same order they were read/obtained.
		 */
		object_tag_id_type
		add_object_tag(
				const object_tag_type &object_tag);


		/**
		 * Returns the number of unique string objects.
		 *
		 * Used by archive writers.
		 */
		unsigned int
		get_num_unique_string_objects() const
		{
			return d_unique_string_objects.size();
		}

		/**
		 * Returns the unique string object identified by a unique string index.
		 *
		 * Used by archive writers.
		 */
		const std::string &
		get_unique_string_object(
				unsigned int unique_string_index) const;


		/**
		 * Adds a unique string object.
		 *
		 * Used by archive readers.
		 *
		 * Note that all added string object must be unique and they must be added
		 * in the same order they were read/obtained.
		 */
		unsigned int
		add_unique_string_object(
				const std::string &unique_string_object);


		/**
		 * Returns the string object identified by @a object_tag_id.
		 *
		 * The string object is actually an index into the unique strings.
		 *
		 * Used by archive writers.
		 */
		unsigned int
		get_string_object(
				object_id_type object_id) const;


		/**
		 * Adds a string object.
		 *
		 * A string object is actually an index into the unique strings.
		 *
		 * Used by archive readers.
		 */
		void
		add_string_object(
				object_id_type object_id,
				unsigned int unique_string_index);


		/**
		 * Returns the specified object tag/version as an object key.
		 *
		 * Used by class TranscriptionScribeContext.
		 *
		 * Returns none if the object tag/version are not used by any (composite) objects.
		 */
		boost::optional<object_key_type>
		get_object_key(
				const char *object_tag,
				object_tag_version_type object_tag_version) const;

		/**
		 * Returns the specified object tag/version as an object key if it already exists,
		 * or creates a new object key if needed.
		 *
		 * Used by class TranscriptionScribeContext.
		 */
		object_key_type
		get_or_create_object_key(
				const char *object_tag,
				object_tag_version_type object_tag_version);


		/**
		 * Returns true if the transcription is complete.
		 *
		 * This should typically be called after having transcribed all objects to/from the archive
		 * (using Scribe via TranscriptionScribeContext).
		 *
		 * @a null_pointer_object_id is the object id used for NULL pointers (by Scribe).
		 *
		 * If @a emit_warnings is true then a warning is emitted for referenced object that was not found.
		 */
		bool
		is_complete(
				object_id_type null_pointer_object_id,
				bool emit_warnings = true) const;

	private:

		//! Typedef for mapping unique object tags to tag ids (indices into @a object_tag_seq_type).
		typedef std::map<object_tag_type, object_tag_id_type> object_tag_id_map_type;


		//! Typedef for mapping unique string objects to indices (into @a string_object_seq_type).
		typedef std::map<std::string, unsigned int> string_object_index_map_type;


		//! Info on where to find a primitive/composite object.
		struct ObjectLocation
		{
			ObjectLocation() :
				type(UNUSED),
				index(0)
			{  }

			ObjectType type;    // Which std::vector to lookup.
			unsigned int index; // Index into std::vector.
		};

		typedef std::vector<ObjectLocation> object_location_seq_type;

		/**
		 * Typedef for a pool allocator of @a CompositeObject.
		 *
		 * This reduces the cost of std::vector re-allocations since they involve copying
		 * @a CompositeObject which, in turn, involves copying std::vector.
		 */
		typedef boost::object_pool<CompositeObject> composite_object_pool_type;


		// Keep track of unique object tags (strings) and map them to integer tag ids.
		std::vector<object_tag_type> d_object_tags;
		object_tag_id_map_type d_object_tag_id_map;

		// Info on where to find the primitive/composite objects.
		object_location_seq_type d_object_locations;

		// Primitive integral/float objects.
		std::vector<int32_type> d_signed_integer_objects;
		std::vector<uint32_type> d_unsigned_integer_objects;
		std::vector<float> d_float_objects;
		std::vector<double> d_double_objects;

		// Primitive string objects.
		std::vector<std::string> d_unique_string_objects;
		std::vector<unsigned int> d_string_objects; // Indices into 'd_unique_string_objects'
		string_object_index_map_type d_string_object_index_map;

		// Composite objects.
		composite_object_pool_type d_composite_object_pool;
		std::vector<CompositeObject *> d_composite_objects;


		Transcription()
		{  }

		const ObjectLocation &
		get_object_location(
				object_id_type object_id,
				ObjectType object_type) const;

		ObjectLocation &
		add_object_location(
				object_id_type object_id,
				ObjectType object_type);
	};
}


#endif // GPLATES_SCRIBE_TRANSCRIPTION_H
