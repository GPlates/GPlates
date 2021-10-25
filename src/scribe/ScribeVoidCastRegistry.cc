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

#include "ScribeVoidCastRegistry.h"

#include "global/GPlatesAssert.h"


boost::optional<void *>
GPlatesScribe::VoidCastRegistry::up_cast(
		const std::type_info &derived_type,
		const std::type_info &base_type,
		void *derived_object_address) const
{
	// Return early if derived and base types are the same.
	if (derived_type == base_type)
	{
		return derived_object_address;
	}

	// Recursively search the derived type's base types list to find a path to the specified base type.
	class_links_list_type derived_to_base_path;
	if (!find_derived_to_base_path(derived_to_base_path, derived_type, base_type))
	{
		return boost::none;
	}

	// Iterate over the derived-to-base path and performing casting.
	void *object_address = derived_object_address;
	class_links_list_type::const_iterator derived_to_base_path_iter = derived_to_base_path.begin();
	class_links_list_type::const_iterator derived_to_base_path_end = derived_to_base_path.end();
	for ( ; derived_to_base_path_iter != derived_to_base_path_end; ++derived_to_base_path_iter)
	{
		const ClassLink &class_link = *derived_to_base_path_iter;

		object_address = class_link.upcast(object_address);
	}

	return object_address;
}


boost::optional< boost::shared_ptr<void> >
GPlatesScribe::VoidCastRegistry::up_cast(
		const std::type_info &derived_type,
		const std::type_info &base_type,
		const boost::shared_ptr<void> &derived_object_address) const
{
	// Return early if derived and base types are the same.
	if (derived_type == base_type)
	{
		return derived_object_address;
	}

	// Recursively search the derived type's base types list to find a path to the specified base type.
	class_links_list_type derived_to_base_path;
	if (!find_derived_to_base_path(derived_to_base_path, derived_type, base_type))
	{
		return boost::none;
	}

	// Iterate over the derived-to-base path and performing casting.
	boost::shared_ptr<void> object_address = derived_object_address;
	class_links_list_type::const_iterator derived_to_base_path_iter = derived_to_base_path.begin();
	class_links_list_type::const_iterator derived_to_base_path_end = derived_to_base_path.end();
	for ( ; derived_to_base_path_iter != derived_to_base_path_end; ++derived_to_base_path_iter)
	{
		const ClassLink &class_link = *derived_to_base_path_iter;

		object_address = class_link.upcast(object_address);
	}

	return object_address;
}


boost::optional<void *>
GPlatesScribe::VoidCastRegistry::down_cast(
		const std::type_info &derived_type,
		const std::type_info &base_type,
		void *base_object_address) const
{
	// Return early if derived and base types are the same.
	if (derived_type == base_type)
	{
		return base_object_address;
	}

	// Recursively search the derived type's base types list to find a path to the specified base type.
	// It is more efficient to search from derived-to-base than base-to-derived due to the
	// branching nature of inheritance.
	class_links_list_type derived_to_base_path;
	if (!find_derived_to_base_path(derived_to_base_path, derived_type, base_type))
	{
		return boost::none;
	}

	// Iterate over the derived-to-base path in reverse order to get the base-to-derived path.
	class_links_list_type base_to_derived_path;
	get_base_to_derived_path(
			base_to_derived_path,
			derived_to_base_path.begin(),
			derived_to_base_path.end());

	// Iterate over the base-to-derived path and performing casting.
	void *object_address = base_object_address;
	class_links_list_type::const_iterator base_to_derived_path_iter = base_to_derived_path.begin();
	class_links_list_type::const_iterator base_to_derived_path_end = base_to_derived_path.end();
	for ( ; base_to_derived_path_iter != base_to_derived_path_end; ++base_to_derived_path_iter)
	{
		const ClassLink &class_link = *base_to_derived_path_iter;

		object_address = class_link.downcast(object_address);
	}

	return object_address;
}


boost::optional< boost::shared_ptr<void> >
GPlatesScribe::VoidCastRegistry::down_cast(
		const std::type_info &derived_type,
		const std::type_info &base_type,
		const boost::shared_ptr<void> &base_object_address) const
{
	// Return early if derived and base types are the same.
	if (derived_type == base_type)
	{
		return base_object_address;
	}

	// Recursively search the base type's derived types list to find a path to the specified derived type.
	// It is more efficient to search from derived-to-base than base-to-derived due to the
	// branching nature of inheritance.
	class_links_list_type derived_to_base_path;
	if (!find_derived_to_base_path(derived_to_base_path, derived_type, base_type))
	{
		return boost::none;
	}

	// Iterate over the derived-to-base path in reverse order to get the base-to-derived path.
	class_links_list_type base_to_derived_path;
	get_base_to_derived_path(
			base_to_derived_path,
			derived_to_base_path.begin(),
			derived_to_base_path.end());

	// Iterate over the base-to-derived path and performing casting.
	boost::shared_ptr<void> object_address = base_object_address;
	class_links_list_type::const_iterator base_to_derived_path_iter = base_to_derived_path.begin();
	class_links_list_type::const_iterator base_to_derived_path_end = base_to_derived_path.end();
	for ( ; base_to_derived_path_iter != base_to_derived_path_end; ++base_to_derived_path_iter)
	{
		const ClassLink &class_link = *base_to_derived_path_iter;

		object_address = class_link.downcast(object_address);
	}

	return object_address;
}


bool
GPlatesScribe::VoidCastRegistry::find_derived_to_base_path(
		class_links_list_type &derived_to_base_path,
		const std::type_info &derived_type,
		const std::type_info &base_type) const
{
	// Find the derived type class node.
	class_type_info_to_node_map_type::const_iterator derived_node_iter =
			d_class_type_info_to_node_map.find(&derived_type);

	// If unable to start the path from the derived type...
	if (derived_node_iter == d_class_type_info_to_node_map.end())
	{
		return false;
	}

	const ClassNode &derived_class_node = *derived_node_iter->second;

	// Recursively search the derived type's base types list to find a path to the specified base type.
	return find_derived_to_base_path(derived_to_base_path, derived_class_node, derived_type, base_type);
}


bool
GPlatesScribe::VoidCastRegistry::find_derived_to_base_path(
		class_links_list_type &derived_to_base_path,
		const ClassNode &current_class_node,
		const std::type_info &derived_type,
		const std::type_info &base_type) const
{
	bool found_path = false;

	class_type_info_to_link_map_type::const_iterator current_base_class_links_iter =
			current_class_node.base_class_links.begin();
	class_type_info_to_link_map_type::const_iterator current_base_class_links_end =
			current_class_node.base_class_links.end();
	for ( ; current_base_class_links_iter != current_base_class_links_end; ++current_base_class_links_iter)
	{
		const std::type_info &current_base_type = *current_base_class_links_iter->first;
		const ClassLink &current_base_class_link = *current_base_class_links_iter->second;

		// See if we found the base type.
		if (current_base_type == base_type)
		{
			// Throw AmbiguousCast exception if we have already found a path between derived and base types.
			// If this is the first time 'base_type' has been found then the list will be empty.
			GPlatesGlobal::Assert<Exceptions::AmbiguousCast>(
					derived_to_base_path.empty(),
					GPLATES_ASSERTION_SOURCE,
					derived_type,
					base_type);

			// Push the current base class link onto the list.
			derived_to_base_path.push_front(&current_base_class_link);
			found_path = true;

			// We still need to check the remaining base classes of current class node in case there's
			// another path to 'base_type' as shown in the following example:
			//
			//      A
			//      |
			//  A   C
			//   \ /
			//    D
			//
			// ...where the first 'A' is the base type (of the current node 'D") that we just found,
			// but we still need to look at 'C' since it inherits from another 'A' in which case
			// we'll need to throw the AmbiguousCast exception.
		}

		// If we are on the right path to the base type then add the current base class node to the list.
		//
		// Note: We don't also immediately return because we need to check the remaining 
		if (find_derived_to_base_path(
				derived_to_base_path,
				*current_base_class_link.base_class_node,
				derived_type,
				base_type))
		{
			derived_to_base_path.push_front(&current_base_class_link);
			found_path = true;
		}
	}

	return found_path;
}


void
GPlatesScribe::VoidCastRegistry::get_base_to_derived_path(
		class_links_list_type &base_to_derived_path,
		class_links_list_type::const_iterator derived_to_base_path_iter,
		class_links_list_type::const_iterator derived_to_base_path_end) const
{
	if (derived_to_base_path_iter == derived_to_base_path_end)
	{
		return;
	}

	const ClassLink &class_link = *derived_to_base_path_iter;

	// NOTE: We need to increment the iterator *before* we add the element to another list
	// because adding to a list overwrite the next-link in the list.
	++derived_to_base_path_iter;
	base_to_derived_path.push_front(&class_link);

	get_base_to_derived_path(base_to_derived_path, derived_to_base_path_iter, derived_to_base_path_end);
}


GPlatesScribe::VoidCastRegistry::ClassNode *
GPlatesScribe::VoidCastRegistry::get_or_create_class_node(
		const std::type_info &class_type_info)
{
	// Attempt to insert the class node.
	std::pair<class_type_info_to_node_map_type::iterator, bool> class_node_inserted =
			d_class_type_info_to_node_map.insert(
					class_type_info_to_node_map_type::value_type(
							&class_type_info,
							static_cast<ClassNode *>(NULL)/*dummy*/));

	// If class node was inserted (ie, didn't previously exist in the map)...
	if (class_node_inserted.second)
	{
		// Create a ClassNode for the class and point the map entry to it.
		class_node_inserted.first->second =
				d_class_node_storage.construct(class_type_info);
	}

	return class_node_inserted.first->second;
}
