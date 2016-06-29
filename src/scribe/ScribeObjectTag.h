/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_SCRIBEOBJECTTAG_H
#define GPLATES_SCRIBE_SCRIBEOBJECTTAG_H

#include <string>
#include <vector>


namespace GPlatesScribe
{
	/**
	 * An object tag is used to identify a transcribed object within the transcription.
	 */
	class ObjectTag
	{
	public:

		/**
		 * Each section in an object tag can be:
		 * - a tag (name/version), or
		 * - an array index, or
		 * - an array size.
		 */
		enum SectionType
		{
			TAG_SECTION,
			ARRAY_INDEX_SECTION,
			ARRAY_SIZE_SECTION
		};


		/**
		 * An object tag is divided into one or more sections.
		 *
		 * Each section can either be a tag (name/version) or an array (index or size).
		 */
		class Section
		{
		public:

			SectionType
			get_type() const
			{
				return d_section_type;
			}


			//! The tag name.
			const std::string &
			get_tag_name() const
			{
				return d_tag_name;
			}

			//! The tag version.
			unsigned int
			get_tag_version() const
			{
				return d_tag_version;
			}


			//! The array index - only use if @a get_type returns @a ARRAY_INDEX_SECTION.
			unsigned int
			get_array_index() const
			{
				return d_array_index;
			}

		private:

			SectionType d_section_type;

			std::string d_tag_name;
			unsigned int d_tag_version;

			// Only used if section is an array type...
			unsigned int d_array_index;

		public: // Used by ObjectTag...

			Section(
					SectionType section_type,
					const std::string &tag_name,
					unsigned int tag_version,
					unsigned int array_index = 0) :
				d_section_type(section_type),
				d_tag_name(tag_name),
				d_tag_version(tag_version),
				d_array_index(array_index)
			{  }
		};


		/**
		 * An empty object tag.
		 *
		 * Note: An empty object tag should not be used to transcribe an object, otherwise
		 * @a ScribeUserError will get thrown when transcribing.
		 *
		 * This is only useful when you need an object tag for a direct array (with no tags prefixed)
		 * such as:
		 *
		 *   GPlatesScribe::ObjectTag()[n] // or GPlatesScribe::ObjectTag().array_item(n, ...)
		 *
		 * ...or...
		 *
		 *   GPlatesScribe::ObjectTag().sequence_size() // or GPlatesScribe::ObjectTag().array_size(...)
		 *
		 * ...which represents the 'n'th index into an array and the length/size of the array.
		 */
		ObjectTag()
		{  }


		/**
		 * Create a single-entry object tag from the specified tag name and version.
		 *
		 * Note: Not using 'explicit' allows implicit conversion from std::string to an ObjectTag.
		 * For example when passing a std::string to a function that expects an ObjectTag.
		 */
		ObjectTag(
				const std::string &tag_name,
				unsigned int tag_version = 0)
		{
			d_sections.push_back(Section(TAG_SECTION, tag_name, tag_version));
		}

		/**
		 * Same as other constructor but the tag name is specified as a NULL-terminated string.
		 *
		 * Note: Not using 'explicit' allows implicit conversion from a NULL-terminated 'const char *'
		 * to an ObjectTag. For example when passing a 'const char *' to a function that expects an ObjectTag.
		 */
		ObjectTag(
				const char *tag_name,
				unsigned int tag_version = 0)
		{
			d_sections.push_back(Section(TAG_SECTION, tag_name, tag_version));
		}


		/**
		 * Returns a copy of this object tag, but with a suffix tag appended.
		 *
		 * For example:
		 *
		 *   if (!scribe.transcribe(
		 *           TRANSCRIBE_SOURCE,
		 *           objectA,
		 *           GPlatesScribe::ObjectTag("objects")("objectA")))
		 *   {
		 *       return scribe.get_transcribe_result();
		 *   }
		 *
		 * Throws @a ScribeUserError if @a sequence_size, @a map_size or @a array_size has already
		 * been called on this object.
		 */
		ObjectTag
		operator()(
				const std::string &suffix_tag_name,
				unsigned int suffix_tag_version = 0) const
		{
			return add_tag_section(suffix_tag_name, suffix_tag_version);
		}


		/**
		 * Returns a copy of this object tag, but with an additional array indexation using
		 * the sequence protocol (ie, 'item' for array items).
		 *
		 * Using the sequence protocol means a sequence (eg, std::vector) could also be used
		 * when loading transcription (as an alternative to array indexing in object tag) because
		 * internally a sequence uses the sequence protocol.
		 *
		 * For example:
		 *
		 *   if (!scribe.transcribe(
		 *           TRANSCRIBE_SOURCE,
		 *           objects[n],
		 *           GPlatesScribe::ObjectTag("objects")[n]))
		 *   {
		 *       return scribe.get_transcribe_result();
		 *   }
		 *
		 * Throws @a ScribeUserError if @a sequence_size, @a map_size or @a array_size has already
		 * been called on this object.
		 */
		ObjectTag
		operator[](
				unsigned int array_index) const
		{
			return sequence_item(array_index);
		}

		/**
		 * Same as operator[].
		 */
		ObjectTag
		sequence_item(
				unsigned int sequence_index) const
		{
			return add_array_index_section(sequence_index, SEQUENCE_PROTOCOL_ITEM_TAG_NAME, SEQUENCE_PROTOCOL_ITEM_TAG_VERSION);
		}

		/**
		 * Returns a copy of this object tag, but with an additional array indexation using
		 * the mapping protocol (ie, 'item_key' for map keys).
		 *
		 * Using the mapping protocol means a map (eg, std::map) could also be used when loading
		 * transcription (as an alternative to array indexing in object tag) because internally
		 * a map uses the mapping protocol.
		 *
		 * Same as 'array_item(map_index, "item_key")'.
		 */
		ObjectTag
		map_item_key(
				unsigned int map_index) const
		{
			return add_array_index_section(map_index, MAPPING_PROTOCOL_ITEM_KEY_TAG_NAME, MAPPING_PROTOCOL_ITEM_KEY_TAG_VERSION);
		}

		/**
		 * Returns a copy of this object tag, but with an additional array indexation using
		 * the mapping protocol (ie, 'item_value' for map values).
		 *
		 * Using the mapping protocol means a map (eg, std::map) could also be used when loading
		 * transcription (as an alternative to array indexing in object tag) because internally
		 * a map uses the mapping protocol.
		 *
		 * Same as 'array_item(map_index, "item_value")'.
		 */
		ObjectTag
		map_item_value(
				unsigned int map_index) const
		{
			return add_array_index_section(map_index, MAPPING_PROTOCOL_ITEM_VALUE_TAG_NAME, MAPPING_PROTOCOL_ITEM_VALUE_TAG_VERSION);
		}

		/**
		 * Same as @a sequence_item, @a map_item_key and @a map_item_value except can specify the
		 * array indexing tag name/version instead of relying on the sequence protocol (which uses
		 * 'item' for sequence items) or the mapping protocol (which uses 'item_key' and 'item_value'
		 * for map key/value items).
		 *
		 * For example:
		 *
		 *   if (!scribe.transcribe(
		 *           TRANSCRIBE_SOURCE,
		 *           container.get_element(n),
		 *           GPlatesScribe::ObjectTag("container").array_item(n, "element")))
		 *   {
		 *       return scribe.get_transcribe_result();
		 *   }
		 *
		 * Throws @a ScribeUserError if @a sequence_size, @a map_size or @a array_size has already
		 * been called on this object.
		 */
		ObjectTag
		array_item(
				unsigned int array_index,
				const std::string &array_item_tag_name,
				unsigned int array_item_tag_version = 0) const
		{
			return add_array_index_section(array_index, array_item_tag_name, array_item_tag_version);
		}


		/**
		 * Returns a copy of this object tag that will be used to query the size of an array using
		 * the sequence protocol (ie, 'size' for sequence size).
		 *
		 * Using the sequence protocol means a sequence (eg, std::vector) could also be used
		 * when loading transcription (as an alternative to array indexing in object tag) because
		 * internally a sequence uses the sequence protocol.
		 *
		 * For example:
		 *
		 *   unsigned int num_objects;
		 *   
		 *   if (scribe.is_saving())
		 *   {
		 *       num_objects = objects.size();
		 *   }
		 *   
		 *   if (!scribe.transcribe(
		 *           TRANSCRIBE_SOURCE,
		 *           num_objects,
		 *           GPlatesScribe::ObjectTag("objects").sequence_size()))
		 *   {
		 *       return scribe.get_transcribe_result();
		 *   }
		 *
		 * Throws @a ScribeUserError if @a sequence_size, @a map_size or @a array_size has already
		 * been called on this object.
		 */
		ObjectTag
		sequence_size() const
		{
			return add_array_size_section(SEQUENCE_PROTOCOL_SIZE_TAG_NAME, SEQUENCE_PROTOCOL_SIZE_TAG_VERSION);
		}


		/**
		 * Returns a copy of this object tag that will be used to query the size of a map using
		 * the mapping protocol (ie, 'size' for map size).
		 *
		 * Using the mapping protocol means a map (eg, std::map) could also be used when loading
		 * transcription (as an alternative to array indexing in object tag) because internally
		 * a map uses the mapping protocol.
		 *
		 * For example:
		 *
		 *   unsigned int num_objects_in_map;
		 *   
		 *   if (scribe.is_saving())
		 *   {
		 *       num_objects_in_map = map.size();
		 *   }
		 *   
		 *   if (!scribe.transcribe(
		 *           TRANSCRIBE_SOURCE,
		 *           num_objects_in_map,
		 *           GPlatesScribe::ObjectTag("map").map_size()))
		 *   {
		 *       return scribe.get_transcribe_result();
		 *   }
		 *
		 * Throws @a ScribeUserError if @a sequence_size, @a map_size or @a array_size has already
		 * been called on this object.
		 */
		ObjectTag
		map_size() const
		{
			return add_array_size_section(MAPPING_PROTOCOL_SIZE_TAG_NAME, MAPPING_PROTOCOL_SIZE_TAG_VERSION);
		}


		/**
		 * Same as @a sequence_size and @a map_size except can specify the array size tag name/version
		 * (instead of relying on sequence or mapping protocol - both of which use 'size' for array size).
		 *
		 * For example:
		 *
		 *   if (!scribe.transcribe(
		 *           TRANSCRIBE_SOURCE,
		 *           num_elements,
		 *           GPlatesScribe::ObjectTag("container").array_size("num_elements")))
		 *   {
		 *       return scribe.get_transcribe_result();
		 *   }
		 *
		 * Throws @a ScribeUserError if @a sequence_size, @a map_size or @a array_size has already
		 * been called on this object.
		 */
		ObjectTag
		array_size(
				const std::string &array_size_tag_name,
				unsigned int array_size_tag_version = 0) const
		{
			return add_array_size_section(array_size_tag_name, array_size_tag_version);
		}


		/**
		 * Returns the sections of this object tag.
		 *
		 * Throws @a ScribeUserError if there are no sections.
		 */
		const std::vector<Section> &
		get_sections() const;

	private:

		//
		// The object tag name/version used by the sequence protocol for the sequence items.
		//
		static const std::string SEQUENCE_PROTOCOL_ITEM_TAG_NAME;
		static const unsigned int SEQUENCE_PROTOCOL_ITEM_TAG_VERSION;

		//
		// The object tag name/version used by the sequence protocol for the sequence size.
		//
		static const std::string SEQUENCE_PROTOCOL_SIZE_TAG_NAME;
		static const unsigned int SEQUENCE_PROTOCOL_SIZE_TAG_VERSION;


		//
		// The object tag name/version used by the mapping protocol for the map keys.
		//
		static const std::string MAPPING_PROTOCOL_ITEM_KEY_TAG_NAME;
		static const unsigned int MAPPING_PROTOCOL_ITEM_KEY_TAG_VERSION;

		//
		// The object tag name/version used by the mapping protocol for the map values.
		//
		static const std::string MAPPING_PROTOCOL_ITEM_VALUE_TAG_NAME;
		static const unsigned int MAPPING_PROTOCOL_ITEM_VALUE_TAG_VERSION;

		//
		// The object tag name/version used by the mapping protocol for the map size.
		//
		static const std::string MAPPING_PROTOCOL_SIZE_TAG_NAME;
		static const unsigned int MAPPING_PROTOCOL_SIZE_TAG_VERSION;


		std::vector<Section> d_sections;


		/**
		 * A small optimisation to reduce std::vector reallocations by reserving enough space
		 * for a subsequent 'push_back()'.
		 */
		ObjectTag(
				const ObjectTag &rhs,
				unsigned int num_sections_to_reserve)
		{
			d_sections.reserve(num_sections_to_reserve);
			// Copy the sections.
			d_sections.insert(
					d_sections.begin(),
					rhs.d_sections.begin(),
					rhs.d_sections.end());
		}

		ObjectTag
		add_tag_section(
				const std::string &tag_name,
				unsigned int tag_version) const;

		ObjectTag
		add_array_index_section(
				unsigned int array_index,
				const std::string &array_item_tag_name,
				unsigned int array_item_tag_version) const;

		ObjectTag
		add_array_size_section(
				const std::string &array_size_tag_name,
				unsigned int array_size_tag_version) const;
	};
}

#endif // GPLATES_SCRIBE_SCRIBEOBJECTTAG_H
