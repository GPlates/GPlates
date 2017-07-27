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

#include "ScribeObjectTag.h"

#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


const std::string GPlatesScribe::ObjectTag::SEQUENCE_PROTOCOL_ITEM_TAG_NAME("item");
const unsigned int GPlatesScribe::ObjectTag::SEQUENCE_PROTOCOL_ITEM_TAG_VERSION = 0;

const std::string GPlatesScribe::ObjectTag::SEQUENCE_PROTOCOL_SIZE_TAG_NAME("size");
const unsigned int GPlatesScribe::ObjectTag::SEQUENCE_PROTOCOL_SIZE_TAG_VERSION = 0;

const std::string GPlatesScribe::ObjectTag::MAPPING_PROTOCOL_ITEM_KEY_TAG_NAME("item_key");
const unsigned int GPlatesScribe::ObjectTag::MAPPING_PROTOCOL_ITEM_KEY_TAG_VERSION = 0;

const std::string GPlatesScribe::ObjectTag::MAPPING_PROTOCOL_ITEM_VALUE_TAG_NAME("item_value");
const unsigned int GPlatesScribe::ObjectTag::MAPPING_PROTOCOL_ITEM_VALUE_TAG_VERSION = 0;

const std::string GPlatesScribe::ObjectTag::MAPPING_PROTOCOL_SIZE_TAG_NAME("size");
const unsigned int GPlatesScribe::ObjectTag::MAPPING_PROTOCOL_SIZE_TAG_VERSION = 0;


const std::vector<GPlatesScribe::ObjectTag::Section> &
GPlatesScribe::ObjectTag::get_sections() const
{
	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			!d_sections.empty(),
			GPLATES_ASSERTION_SOURCE,
			"Object tag must not be empty.");

	return d_sections;
}


GPlatesScribe::ObjectTag
GPlatesScribe::ObjectTag::add_tag_section(
		const std::string &tag_name,
		unsigned int tag_version) const
{
	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			!tag_name.empty(),
			GPLATES_ASSERTION_SOURCE,
			"Attempted to use an empty tag string in an object tag.");

	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			d_sections.empty() ||
				d_sections.back().get_type() != ARRAY_SIZE_SECTION,
			GPLATES_ASSERTION_SOURCE,
			"Cannot append to an object tag that was returned by 'sequence_size()', 'map_size' or 'array_size()'.");

	ObjectTag new_object_tag(*this, d_sections.size() + 1);

	new_object_tag.d_sections.push_back(
			Section(TAG_SECTION, tag_name, tag_version));

	return new_object_tag;
}


GPlatesScribe::ObjectTag
GPlatesScribe::ObjectTag::add_array_index_section(
		unsigned int array_index,
		const std::string &array_item_tag_name,
		unsigned int array_item_tag_version) const
{
	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			!array_item_tag_name.empty(),
			GPLATES_ASSERTION_SOURCE,
			"Attempted to use an empty array item string in an object tag.");

	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			d_sections.empty() ||
				d_sections.back().get_type() != ARRAY_SIZE_SECTION,
			GPLATES_ASSERTION_SOURCE,
			"Cannot append to an object tag that was returned by 'sequence_size()', 'map_size' or 'array_size()'.");

	ObjectTag new_object_tag(*this, d_sections.size() + 1);

	new_object_tag.d_sections.push_back(
			Section(ARRAY_INDEX_SECTION, array_item_tag_name, array_item_tag_version, array_index));

	return new_object_tag;
}


GPlatesScribe::ObjectTag
GPlatesScribe::ObjectTag::add_array_size_section(
		const std::string &array_size_tag_name,
		unsigned int array_size_tag_version) const
{
	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			!array_size_tag_name.empty(),
			GPLATES_ASSERTION_SOURCE,
			"Attempted to use an empty array size string in an object tag.");

	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			d_sections.empty() ||
				d_sections.back().get_type() != ARRAY_SIZE_SECTION,
			GPLATES_ASSERTION_SOURCE,
			"Cannot append to an object tag that was returned by 'sequence_size()', 'map_size' or 'array_size()'.");

	ObjectTag new_object_tag(*this, d_sections.size() + 1);

	new_object_tag.d_sections.push_back(
			Section(ARRAY_SIZE_SECTION, array_size_tag_name, array_size_tag_version));

	return new_object_tag;
}
