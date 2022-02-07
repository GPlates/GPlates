/* $Id$ */

/**
 * \file Manages hierarchical building of a QTreeWidget.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <algorithm>
#include <iterator>
#include <boost/numeric/conversion/cast.hpp>

#include "TreeWidgetBuilder.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

//#include "utils/Profile.h"


GPlatesGui::TreeWidgetBuilder::TreeWidgetBuilder(
		QTreeWidget *tree_widget) :
d_tree_widget(tree_widget)
{
	reset();
}


void
GPlatesGui::TreeWidgetBuilder::reset()
{
	// Release the item pointers.
	d_items.clear();
	// Release all handles.
	d_item_handle_manager.clear();
	// Clear the current item stack.
	d_current_item_handle_stack = std::stack<item_handle_type>();

	allocate_root_item();
	d_current_item_handle = d_root_handle;

	// The current item stack contains the root handle only.
	d_current_item_handle_stack.push(d_root_handle);
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::TreeWidgetBuilder::get_root_handle() const
{
	return d_root_handle;
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::TreeWidgetBuilder::get_current_item_handle() const
{
	// It's ok to return the root handle to the caller.
	return d_current_item_handle;
}


QTreeWidgetItem *
GPlatesGui::TreeWidgetBuilder::get_qtree_widget_item(
		item_handle_type item_handle)
{
	// If we get an assertion failure here then there is an error in the caller's program logic.
	// The root handle is not something the client should know about - its a fictitious item.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			item_handle != d_root_handle &&
					d_item_handle_manager.is_valid_item_handle(item_handle),
			GPLATES_ASSERTION_SOURCE);

	return d_items[item_handle]->d_item;
}


void
GPlatesGui::TreeWidgetBuilder::push_current_item(
		item_handle_type item_handle)
{
	// If we get an assertion failure here then there is an error in the caller's program logic.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_item_handle_manager.is_valid_item_handle(item_handle),
			GPLATES_ASSERTION_SOURCE);

	d_current_item_handle = item_handle;
	d_current_item_handle_stack.push(item_handle);
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::TreeWidgetBuilder::pop_current_item()
{
	// If we get an assertion failure here then there is an error in the caller's program logic.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_current_item_handle_stack.empty(),
			GPLATES_ASSERTION_SOURCE);

	d_current_item_handle_stack.pop();
	d_current_item_handle = d_current_item_handle_stack.top();

	// NOTE: If current item has no parent then it's a top-level item and
	// 'd_current_item_handle' will now be 'd_root_handle'.
	return d_current_item_handle;
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::TreeWidgetBuilder::create_item(
		const QStringList &fields)
{
	// If QTreeWidgetItem constructor throws exception then compiler will free memory.
	// NOTE: We don't pass the parent into the constructor as this slows things down a lot -
	// but we must then make sure we don't add to a parent that itself has no parent as this
	// seems to cause Qt to crash.
	std::auto_ptr<QTreeWidgetItem> new_tree_widget_item(new QTreeWidgetItem(fields));

	// Create a new Item wrapper.
	managed_item_ptr_type new_item(new Item(new_tree_widget_item));

	// Allocate a handle for the new item and store the new item.
	return allocate_item(new_item);
}


void
GPlatesGui::TreeWidgetBuilder::destroy_item(
		item_handle_type item_handle)
{
	// If we get an assertion failure here then there is an error in the caller's program logic.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			item_handle != d_root_handle,
			GPLATES_ASSERTION_SOURCE);

	Item *item = get_item(item_handle);

	// If the item has a parent then remove item from parent.
	if (item->d_parent_item_handle != Item::INVALID_HANDLE)
	{
		remove_child(item->d_parent_item_handle, item_handle);
	}

	destroy_item_without_removing_from_parent(item_handle);
}


void
GPlatesGui::TreeWidgetBuilder::destroy_item_without_removing_from_parent(
		item_handle_type item_handle)
{
	Item *item = get_item(item_handle);

	// Recursively delete the children.
	item_handle_seq_type::iterator child_item_handle_iter;
	for (child_item_handle_iter = item->d_child_nodes.begin();
		child_item_handle_iter != item->d_child_nodes.end();
		++child_item_handle_iter)
	{
		const item_handle_type child_item_handle = *child_item_handle_iter;
		destroy_item_without_removing_from_parent(child_item_handle);
	}

	// Release the item's handle for re-use.
	deallocate_item(item_handle);
}


unsigned int
GPlatesGui::TreeWidgetBuilder::get_num_children(
		item_handle_type parent_item_handle) const
{
	return get_item(parent_item_handle)->d_child_nodes.size();
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::TreeWidgetBuilder::get_child_item_handle(
		item_handle_type parent_item_handle,
		const unsigned int child_index) const
{
	const Item *parent_item = get_item(parent_item_handle);

	// If we get an assertion failure here then there is an error in the caller's program logic.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_index < parent_item->d_child_nodes.size(),
			GPLATES_ASSERTION_SOURCE);

	return parent_item->d_child_nodes[child_index];
}


void
GPlatesGui::TreeWidgetBuilder::add_child(
		const item_handle_type parent_item_handle,
		const item_handle_type child_item_handle)
{
	// Insert at end of child list.
	insert_child(parent_item_handle, child_item_handle,
			get_item(parent_item_handle)->d_child_nodes.size());
}


void
GPlatesGui::TreeWidgetBuilder::insert_child(
		const item_handle_type parent_item_handle,
		const item_handle_type child_item_handle,
		const unsigned int child_index)
{
	Item *parent_item = get_item(parent_item_handle);

	// If we get an assertion failure here then there is an error in the caller's program logic.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_index <= parent_item->d_child_nodes.size(),
			GPLATES_ASSERTION_SOURCE);

	item_handle_seq_type::iterator child_item_handle_iter = parent_item->d_child_nodes.begin();
	std::advance(child_item_handle_iter, child_index);

	// Link parent to child.
	parent_item->d_child_nodes.insert(child_item_handle_iter, child_item_handle);

	// Link child to parent.
	Item *child_item = get_item(child_item_handle);
	child_item->d_parent_item_handle = parent_item_handle;
}


void
GPlatesGui::TreeWidgetBuilder::remove_child(
		const item_handle_type parent_item_handle,
		const item_handle_type child_item_handle)
{
	Item *parent_item = get_item(parent_item_handle);

	// Look for child handle in parent's list of children.
	item_handle_seq_type::iterator child_item_handle_iter = std::find(
			parent_item->d_child_nodes.begin(), parent_item->d_child_nodes.end(), child_item_handle);

	// If we get an assertion failure here then there is an error in the caller's program logic.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_item_handle_iter != parent_item->d_child_nodes.end(),
			GPLATES_ASSERTION_SOURCE);

	const unsigned int child_index = std::distance(
			parent_item->d_child_nodes.begin(), child_item_handle_iter);

	remove_child(parent_item, child_item_handle, child_index);
}


void
GPlatesGui::TreeWidgetBuilder::remove_child_at_index(
		const item_handle_type parent_item_handle,
		const unsigned int child_index)
{
	Item *parent_item = get_item(parent_item_handle);

	// If we get an assertion failure here then there is an error in the caller's program logic.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_index < parent_item->d_child_nodes.size(),
			GPLATES_ASSERTION_SOURCE);

	const item_handle_type child_item_handle = parent_item->d_child_nodes[child_index];

	remove_child(parent_item, child_item_handle, child_index);
}


void
GPlatesGui::TreeWidgetBuilder::add_function(
		item_handle_type item_handle,
		const qtree_widget_item_function_type &function)
{
	// If we get an assertion failure here then there is an error in the caller's program logic.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			item_handle != d_root_handle &&
					d_item_handle_manager.is_valid_item_handle(item_handle),
			GPLATES_ASSERTION_SOURCE);

	// Add 'function' to list of functions to be called on the item when it's
	// attached to the QTreeWidget later on.
	get_item(item_handle)->d_functions.push_back(function);
}


GPlatesGui::TreeWidgetBuilder::Item *
GPlatesGui::TreeWidgetBuilder::get_current_item()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			have_current_item(),
			GPLATES_ASSERTION_SOURCE);

	return d_items[d_current_item_handle].get();
}


GPlatesGui::TreeWidgetBuilder::Item *
GPlatesGui::TreeWidgetBuilder::get_item(
		item_handle_type item_handle)
{
	return const_cast<Item *>(
			static_cast<const TreeWidgetBuilder *>(this)->get_item(item_handle));
}


const GPlatesGui::TreeWidgetBuilder::Item *
GPlatesGui::TreeWidgetBuilder::get_item(
		item_handle_type item_handle) const
{
 	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_item_handle_manager.is_valid_item_handle(item_handle),
 			GPLATES_ASSERTION_SOURCE);

	return d_items[item_handle].get();
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::TreeWidgetBuilder::allocate_item(
		managed_item_ptr_type new_item)
{
	// Allocate a handle - it could be a reused handle or a new one.
	const item_handle_type new_item_handle = d_item_handle_manager.allocate_item_handle();

	// Store created Item in the appropriate handle slot.
	if (new_item_handle == boost::numeric_cast<item_handle_type>(d_items.size()))
	{
		d_items.push_back(new_item);
	}
	else
	{
		// If it was previously released then it should now be NULL.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_items[new_item_handle].get() == NULL,
				GPLATES_ASSERTION_SOURCE);

		// We're reusing a previously released slot.
		d_items[new_item_handle] = new_item;
	}

	return new_item_handle;
}


void
GPlatesGui::TreeWidgetBuilder::deallocate_item(
		item_handle_type item_handle)
{
	// Free the item.
	d_items[item_handle].reset();

	// Deallocate the handle.
	d_item_handle_manager.deallocate_item_handle(item_handle);
}


void
GPlatesGui::TreeWidgetBuilder::allocate_root_item()
{
	// The root QTreeWidgetItem is only used so we can use the QTreeWidgetItem API
	// instead of the QTreeWidget API for top-level items.
	QTreeWidgetItem *root_qtree_widget_item = d_tree_widget->invisibleRootItem();

	// Create a Item wrapper around a null root QTreeWidgetItem.
	managed_item_ptr_type root_item(new Item(root_qtree_widget_item));

	// Allocate a handle and store the dummy root item.
	d_root_handle = allocate_item(root_item);
}


void
GPlatesGui::TreeWidgetBuilder::remove_child(
		Item *parent_item,
		item_handle_type child_item_handle,
		unsigned int child_index)
{
	Item *child_item = get_item(child_item_handle);

	// Check the managed resource to see if our local tree owns the QTreeWidgetItem.
	// This will happen if we're removing a child that we've already added and
	// committed to the QTreeWidget.
	if (child_item->d_managed_item.get() == NULL)
	{
		// The child item has already been transferred to the QTreeWidget so transfer it back.
		// We don't need to delay this operation since the QTreeWidgetItem is already attached to
		// the QTreeWidget.
		QTreeWidgetItem *parent_qtree_widget_item = parent_item->d_item;
		QTreeWidgetItem *child_qtree_widget_item = parent_qtree_widget_item->takeChild(child_index);

		child_item->d_managed_item.reset(child_qtree_widget_item);
	}

	// Remove from our local list of nodes to match what's in the QTreeWidget.
	// Also this makes the child indices match what the caller expects - if they
	// remove a child at the same child index again they are expecting to remove
	// the child after the one just removed.
	item_handle_seq_type::iterator child_item_handle_iter = parent_item->d_child_nodes.begin();
	std::advance(child_item_handle_iter, child_index);
	parent_item->d_child_nodes.erase(child_item_handle_iter);

	// The local child item owns the QTreeWidgetItem and its subtree but it is no longer
	// connected to anything - it's just floating waiting for caller to destroy it or
	// add it back to the tree.
	child_item->d_parent_item_handle = Item::INVALID_HANDLE;
}


void
GPlatesGui::TreeWidgetBuilder::update_qtree_widget_with_added_or_inserted_items()
{
	// Visit the hierarchy recursively starting at the root.
	visit_item_recursively(get_item(d_root_handle));
}


void
GPlatesGui::TreeWidgetBuilder::visit_item_recursively(
		Item *item)
{
	// Now that this QTreeWidgetItem has been inserted into a QTreeWidget
	// we can call functions on it that wouldn't normally work such as 'setExpanded()'.
	call_item_functions(item);

	if (item->d_child_nodes.empty())
	{
		return;
	}

	QList<QTreeWidgetItem *> transfer_list;
	int insert_child_index = 0;

	// Transfer any children that we've recently added.
	// Find contiguous adds and group them together.
	while (true)
	{
		if (!transfer_managed_tree_widget_items_to_qlist(
				item->d_child_nodes, transfer_list, &insert_child_index))
		{
			break;
		}

		QTreeWidgetItem *qtree_widget_item = item->d_item;
		qtree_widget_item->insertChildren(insert_child_index, transfer_list);

		insert_child_index += transfer_list.size();
		transfer_list.clear();
	}

	// Visit the children of 'item' recursively.
	item_handle_seq_type::const_iterator child_item_handle_iter;
	for (child_item_handle_iter = item->d_child_nodes.begin();
		child_item_handle_iter != item->d_child_nodes.end();
		++child_item_handle_iter)
	{
		const item_handle_type child_item_handle = *child_item_handle_iter;
		Item* child_item = d_items[child_item_handle].get();

		// Visit the children recursively.
		visit_item_recursively(child_item);
	}
}


bool
GPlatesGui::TreeWidgetBuilder::transfer_managed_tree_widget_items_to_qlist(
		const item_handle_seq_type &item_seq,
		QList<QTreeWidgetItem *> &transfer_list,
		int *insert_child_index)
{
	item_handle_seq_type::const_iterator item_iter = item_seq.begin();
	std::advance(item_iter, *insert_child_index);

	// Skip a contiguous sequence of non-managed items first.
	for ( ; item_iter != item_seq.end(); ++item_iter, ++*insert_child_index)
	{
		const item_handle_type item_handle = *item_iter;
		Item* item = d_items[item_handle].get();

		if (item->d_managed_item.get())
		{
			// End of contiguous sequence of non-managed items.
			break;
		}
	}

	bool have_transfered_items = false;

	// Collect a contiguous sequence of managed items next.
	// Note: we don't increment '*insert_child_index' in this loop because
	// we want it to point to the beginning of this contiguous sequence.
	for ( ; item_iter != item_seq.end(); ++item_iter)
	{
		const item_handle_type item_handle = *item_iter;
		Item* item = d_items[item_handle].get();

		if (item->d_managed_item.get() == NULL)
		{
			// End of contiguous sequence of managed items.
			break;
		}

		transfer_list.push_back(item->d_managed_item.release());
		have_transfered_items = true;
	}

	return have_transfered_items;
}


void
GPlatesGui::TreeWidgetBuilder::call_item_functions(
		Item *item)
{
	//PROFILE_FUNC();

	item_function_seq_type::iterator function_iter;
	for (function_iter = item->d_functions.begin();
		function_iter != item->d_functions.end();
		++function_iter)
	{
		(*function_iter)(item->d_item, d_tree_widget);
	}

	// Clear the list of functions - we only want to call them once.
	item->d_functions.clear();
}


////////////////////////////////////////////////
// Class TreeWidgetBuilder::ItemHandleManager //
////////////////////////////////////////////////


void
GPlatesGui::TreeWidgetBuilder::ItemHandleManager::clear()
{
	d_current_max_handle = 0;
	d_handles_available_for_reuse.clear();
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::TreeWidgetBuilder::ItemHandleManager::allocate_item_handle()
{
	item_handle_type item_handle;

	if (!d_handles_available_for_reuse.empty())
	{
		//
		// We re-use an item handle that's been deallocated.
		//

		// Pop item handle off the stack.
		item_handle = d_handles_available_for_reuse.back();

		d_handles_available_for_reuse.pop_back();
	}
	else
	{
		//
		// No available slots so create a new one.
		//

		item_handle = d_current_max_handle;
		++d_current_max_handle;
	}

	return item_handle;
}


void
GPlatesGui::TreeWidgetBuilder::ItemHandleManager::deallocate_item_handle(
		item_handle_type item_handle)
{
	// Make sure item handle was is currently being used.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			is_valid_item_handle(item_handle),
			GPLATES_ASSERTION_SOURCE);

	// Make item handle available for re-use.
	d_handles_available_for_reuse.push_back(item_handle);
}


bool
GPlatesGui::TreeWidgetBuilder::ItemHandleManager::is_valid_item_handle(
		item_handle_type item_handle) const
{
	return item_handle < d_current_max_handle &&
			std::find(d_handles_available_for_reuse.begin(),
					d_handles_available_for_reuse.end(), item_handle) ==
					d_handles_available_for_reuse.end();
}


//////////////////////
// Global functions //
//////////////////////


QTreeWidgetItem *
GPlatesGui::get_current_qtree_widget_item(
		TreeWidgetBuilder &tree_widget_builder)
{
	return tree_widget_builder.get_qtree_widget_item(
			tree_widget_builder.get_current_item_handle());
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::add_child(
		TreeWidgetBuilder &tree_widget_builder,
		const TreeWidgetBuilder::item_handle_type parent_item_handle,
		const QStringList &fields)
{
	// Create child item.
	const TreeWidgetBuilder::item_handle_type child_item_handle =
			tree_widget_builder.create_item(fields);

	tree_widget_builder.add_child(parent_item_handle, child_item_handle);

	return child_item_handle;
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::add_child(
		TreeWidgetBuilder &tree_widget_builder,
		const TreeWidgetBuilder::item_handle_type parent_item_handle,
		const QString &name,
		const QString &value)
{
	QStringList fields;
	fields.push_back(name);
	fields.push_back(value);

	return add_child(tree_widget_builder, parent_item_handle, fields);
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::add_top_level_item(
		TreeWidgetBuilder &tree_widget_builder,
		const QStringList &fields)
{
	// Use the root handle to add a top-level item.
	const TreeWidgetBuilder::item_handle_type parent_item_handle =
			tree_widget_builder.get_root_handle();

	return add_child(tree_widget_builder, parent_item_handle, fields);
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::add_top_level_item(
		TreeWidgetBuilder &tree_widget_builder,
		const QString &name,
		const QString &value)
{
	QStringList fields;
	fields.push_back(name);
	fields.push_back(value);

	return add_top_level_item(tree_widget_builder, fields);
}


void
GPlatesGui::add_top_level_item(
		TreeWidgetBuilder &tree_widget_builder,
		TreeWidgetBuilder::item_handle_type top_level_item_handle)
{
	// Use the root handle to add a top-level item.
	const TreeWidgetBuilder::item_handle_type parent_item_handle =
			tree_widget_builder.get_root_handle();

	return tree_widget_builder.add_child(parent_item_handle, top_level_item_handle);
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::add_child_to_current_item(
		TreeWidgetBuilder &tree_widget_builder,
		const QStringList &fields)
{
	// This might be the root handle in which case the child
	// will be added as a top-level item.
	const TreeWidgetBuilder::item_handle_type parent_item_handle =
			tree_widget_builder.get_current_item_handle();

	return add_child(tree_widget_builder, parent_item_handle, fields);
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::add_child_to_current_item(
		TreeWidgetBuilder &tree_widget_builder,
		const QString &name,
		const QString &value)
{
	QStringList fields;
	fields.push_back(name);
	fields.push_back(value);

	return add_child_to_current_item(tree_widget_builder, fields);
}


void
GPlatesGui::insert_top_level_item(
		TreeWidgetBuilder &tree_widget_builder,
		TreeWidgetBuilder::item_handle_type top_level_item_handle,
		const unsigned int top_level_item_index)
{
	tree_widget_builder.insert_child(
			tree_widget_builder.get_root_handle(),
			top_level_item_handle,
			top_level_item_index);
}


void
GPlatesGui::destroy_children(
		TreeWidgetBuilder &tree_widget_builder,
		const TreeWidgetBuilder::item_handle_type parent_item_handle)
{
	const unsigned int num_children = tree_widget_builder.get_num_children(parent_item_handle);

	for (unsigned int child_index = 0; child_index < num_children; ++child_index)
	{
		tree_widget_builder.destroy_item(
				tree_widget_builder.get_child_item_handle(parent_item_handle, child_index));
	}
}


void
GPlatesGui::destroy_top_level_items(
		TreeWidgetBuilder &tree_widget_builder)
{
	const TreeWidgetBuilder::item_handle_type parent_item_handle =
			tree_widget_builder.get_root_handle();

	destroy_children(tree_widget_builder, parent_item_handle);
}


unsigned int
GPlatesGui::get_num_top_level_items(
		TreeWidgetBuilder &tree_widget_builder)
{
	return tree_widget_builder.get_num_children(
			tree_widget_builder.get_root_handle());
}


GPlatesGui::TreeWidgetBuilder::item_handle_type
GPlatesGui::get_top_level_item_handle(
		TreeWidgetBuilder &tree_widget_builder,
		const unsigned int top_level_item_index)
{
	return tree_widget_builder.get_child_item_handle(
			tree_widget_builder.get_root_handle(), top_level_item_index);
}


QTreeWidgetItem *
GPlatesGui::get_child_qtree_widget_item(
		TreeWidgetBuilder &tree_widget_builder,
		TreeWidgetBuilder::item_handle_type parent_item_handle,
		const unsigned int child_index)
{
	return tree_widget_builder.get_qtree_widget_item(
			tree_widget_builder.get_child_item_handle(parent_item_handle, child_index));
}


void
GPlatesGui::add_function_to_current_item(
		TreeWidgetBuilder &tree_widget_builder,
		const TreeWidgetBuilder::qtree_widget_item_function_type &function)
{
	tree_widget_builder.add_function(
			tree_widget_builder.get_current_item_handle(),
			function);
}
