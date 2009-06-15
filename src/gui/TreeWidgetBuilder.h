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

#ifndef GPLATES_GUI_TREEWIDGETBUILDER_H
#define GPLATES_GUI_TREEWIDGETBUILDER_H

#include <algorithm>
#include <memory> // std::auto_ptr
#include <vector>
#include <list>
#include <stack>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/integer_traits.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <QTreeWidget>

//#include "utils/Profile.h"


namespace GPlatesGui
{
	/**
	 * Manages hierarchical building of a QTreeWidget.
	 *
	 * Its main purpose is to delay attaching tree widget items to a tree widget
	 * until the hierarchy is assembled.
	 */
	class TreeWidgetBuilder :
			public boost::noncopyable
	{
	public:
		/**
		 * A generalised function to call that takes a 'QTreeWidgetItem *' argument
		 * followed by a 'QTreeWidget *' argument.
		 * If using boost::bind then you don't even need all or any arguments.
		 * For example,
		 *    void do_something_without_any_args();
		 *
		 *    d_tree_widget_builder->add_function_to_current_item(
		 *        boost::bind(&do_something_without_any_args));
		 *
		 *    void do_something_without_qtree_widget_item(QTreeWidget *);
		 *
		 *    d_tree_widget_builder->add_function_to_current_item(
		 *        boost::bind(&do_something_without_qtree_widget_item, _2));
		 *
		 * ...the '_2' means the argument to 'do_something_without_qtree_widget_item'
		 * comes from the second argument of the function
		 * 'qtree_widget_item_function_type' below.
		 */
		typedef boost::function<void (QTreeWidgetItem *, QTreeWidget *)> qtree_widget_item_function_type;

		/**
		 * Item handles are used to identify QTreeWidgetItem's.
		 */
		typedef unsigned int item_handle_type;

		/**
		 * Constructor.
		 * Calls to @a update_qtree_widget_with_added_or_inserted_items will use @a tree_widget.
		 */
		TreeWidgetBuilder(
				QTreeWidget *tree_widget);

		/**
		 * Resets internal state so can be used again from scratch.
		 * Any item handles become invalid.
		 */
		void
		reset();

		/**
		 * Returns the handle to the root of the widget tree.
		 * This handle can be used to add items as top-level items.
		 * The root handle does not correspond to a QTreeWidgetItem because
		 * the top-level QTreeWidgetItems attach directly to a QTreeWidget not
		 * a root QTreeWidgetItem.
		 * It is merely a convenient way to add top-level items.
		 */
		item_handle_type
		get_root_handle() const;

		/**
		 * Returns current item's handle.
		 * If the returned handle is the root handle (eg, if @a push_current_item
		 * has never been called) then you cannot call @a add_function or
		 * @a get_qtree_widget_item with it.
		 */
		item_handle_type
		get_current_item_handle() const;

		/**
		 * Returns the QTreeWidgetItem associated with @a item_handle.
		 * Throws exception if @a item_handle is the root handle.
		 */
		QTreeWidgetItem *
		get_qtree_widget_item(
				item_handle_type item_handle);

		/**
		 * Creates a tree widget item and returns handle identifying it.
		 */
		item_handle_type
		create_item(
				const QStringList &fields = QStringList());

		/**
		 * Destroys a tree widget item (and items in its child subtrees).
		 * If the item is a child of another item then it is removed
		 * from the parent's list of children before being destroyed.
		 * If attempt to destroy the root item then an exception is thrown.2
		 * The item handles of the destroyed item and items in its
		 * child subtrees are now invalid.
		 */
		void
		destroy_item(
				item_handle_type);

		/**
		 * Returns the number of children of the item @a parent_item_handle.
		 * @a parent_item_handle can be the root handle in which case it
		 * returns the number of top-level items.
		 */
		unsigned int
		get_num_children(
				item_handle_type parent_item_handle) const;

		/**
		 * Returns the handle of child item of @a parent_item_handle
		 * at child index @a child_index.
		 */
		item_handle_type
		get_child_item_handle(
				item_handle_type parent_item_handle,
				const unsigned int child_index) const;

		/**
		 * Adds a previously created child item to a previously created parent item.
		 * Note: @a child_item_handle must not currently be added or inserted.
		 * If @a parent_item_handle is the root handle then @a child_item_handle
		 * becomes a top-level item.
		 */
		void
		add_child(
				const item_handle_type parent_item_handle,
				const item_handle_type child_item_handle);

		/**
		 * Inserts a previously created child item into the list of children of a
		 * previously created parent item at the child index @a child_index.
		 * Note: @a child_item_handle must not currently be added or inserted.
		 * If @a parent_item_handle is the root handle then @a child_item_handle
		 * becomes a top-level item.
		 */
		void
		insert_child(
				const item_handle_type parent_item_handle,
				const item_handle_type child_item_handle,
				const unsigned int child_index);

		/**
		 * Removes a previously created child item from a previously created parent item.
		 * Throws exception if @a child_item_handle is not in the parent's children.
		 * Note: this does not destroy the removed child or any of its child subtrees.
		 * Removing without destroying makes it possible to add or insert item again.
		 * If @a parent_item_handle is the root handle then @a child_item_handle
		 * is a top-level item that's being removed.
		 */
		void
		remove_child(
				const item_handle_type parent_item_handle,
				const item_handle_type child_item_handle);

		/**
		 * Removes a previously created child item from a previously created parent item.
		 * Throws exception if @a child_index greater or equal to the number children in
		 * @a parent_item_handle.
		 * Note: this does not destroy the removed child or any of its child subtrees.
		 * Removing without destroying makes it possible to add or insert item again.
		 * If @a parent_item_handle is the root handle then @a child_item_handle
		 * is a top-level item that's being removed.
		 */
		void
		remove_child_at_index(
				const item_handle_type parent_item_handle,
				const unsigned int child_index);

		/**
		 * Adds @a function to the item identified by @a item_handle.
		 * If @a item_handle is the root handle then an exception is thrown.
		 * Adds a function to the list of functions to be called when
		 * the specified QTreeWidgetItem is attached to the QTreeWidget
		 * in @a update_qtree_widget_with_added_or_inserted_items.
		 * This is only needed for functions that don't work unless the
		 * QTreeWidgetItem is attached to a QTreeWidget such as 'setExpanded()'.
		 * NOTE: QTreeWidgetItem's added or inserted since last call to
		 * @a update_qtree_widget_with_added_or_inserted_items are not yet attached to the QTreeWidget.
		 * QTreeWidgetItem's that are currently attached do not need this method -
		 * they can use @a get_qtree_widget_item and call function directly on that.
		 */
		void
		add_function(
				item_handle_type item_handle,
				const qtree_widget_item_function_type &function);

		/**
		 * Changes the current item to refer to @a item_handle.
		 * This new current item is pushed onto a stack.
		 * Subsequent calls to @a add_child_to_current_item and
		 * @a add_function_to_current_item will add to this new current item.
		 */
		void
		push_current_item(
				item_handle_type item_handle);

		/**
		 * Pops the current item off the stack and restores previous current item.
		 * Returns the new current item.
		 * Requires matching @a push_current_item.
		 */
		item_handle_type
		pop_current_item();

		/**
		 * Transfers all QTreeWidgetItems added or inserted since last call to
		 * @a update_qtree_widget_with_added_or_inserted_items to the QTreeWidget passed in the constructor.
		 * Also calls any functions attached to those QTreeWidgetItems.
		 * Any QTreeWidgetItems that are not linked directly or indirectly to
		 * top-level QTreeWidgetItems are not updated to @a tree_widget.
		 * So if you've added an item to a subtree but the root of that tree
		 * is not attached to the QTreeWidget then none of those items will be transferred.
		 * If items have only been removed or deleted since last call to
		 * @a update_qtree_widget_with_added_or_inserted_items then don't need to call this method.
		 * Only need to call if have added or inserted something.
		 */
		void
		update_qtree_widget_with_added_or_inserted_items();

	private:
		struct Item;

		//! A sequence of item handles.
		typedef std::vector<item_handle_type> item_handle_seq_type;

		//! A sequence of item pointers (not memory-managed).
		typedef std::vector<Item *> item_ptr_seq_type;

		//! A memory-managed pointer to Item.
		typedef boost::shared_ptr<Item> managed_item_ptr_type;

		//! A sequence of memory-managed item pointers.
		typedef std::vector<managed_item_ptr_type> managed_item_ptr_seq_type;

		//! A sequence of functions to call on a QTreeWidgetItem.
		typedef std::list<qtree_widget_item_function_type> item_function_seq_type;

		/**
		 * Manages allocating/deallocating item handles.
		 */
		class ItemHandleManager
		{
		public:
			//! Used to identify a handle that will never be used.
			static const item_handle_type INVALID_HANDLE =
					boost::integer_traits<item_handle_type>::const_max;

			ItemHandleManager() : d_current_max_handle(0) {  }

			void
			clear();

			item_handle_type
			allocate_item_handle();

			void
			deallocate_item_handle(
					item_handle_type);

			bool
			is_valid_item_handle(
					item_handle_type) const;

		private:
			typedef std::vector<item_handle_type> available_handles_type;

			item_handle_type d_current_max_handle;
			available_handles_type d_handles_available_for_reuse;
		};

		/**
		 * Keeps track a tree widget item, its children, its functions and its parent.
		 */
		struct Item :
				public boost::noncopyable
		{
			//! Used to identify a handle that will never be used.
			static const item_handle_type INVALID_HANDLE = ItemHandleManager::INVALID_HANDLE;

			//! Constructor for managed QTreeWidgetItem.
			Item(
					std::auto_ptr<QTreeWidgetItem> tree_widget_item,
					item_handle_type parent_item_handle = INVALID_HANDLE) :
				d_parent_item_handle(parent_item_handle),
				d_managed_item(tree_widget_item),
				d_item(d_managed_item.get())
			{  }

			//! Constructor for non-managed QTreeWidgetItem.
			Item(
					QTreeWidgetItem *tree_widget_item,
					item_handle_type parent_item_handle = INVALID_HANDLE) :
				d_parent_item_handle(parent_item_handle),
				d_managed_item(NULL),
				d_item(tree_widget_item)
			{  }

			item_handle_type d_parent_item_handle;

			// std::auto_ptr is used here because Item is non-copyable and we need to
			// release this resource when transferring ownership to the tree widget later.
			std::auto_ptr<QTreeWidgetItem> d_managed_item;
			QTreeWidgetItem *d_item;

			//! Functions to call on the QTreeWidgetItem when it's attached to QTreeWidget.
			item_function_seq_type d_functions;

			item_handle_seq_type d_child_nodes;
		};

		QTreeWidget *d_tree_widget;

		ItemHandleManager d_item_handle_manager;

		item_handle_type d_current_item_handle;

		/**
		 * There is no actual root QTreeWidgetItem - this just helps identify top-level items.
		 */
		item_handle_type d_root_handle;

		//! Keeps track of @a push_current_item and @a pop_current_item calls.
		std::stack<item_handle_type> d_current_item_handle_stack;

		//! A sequence of all items created so far (memory-managed).
		managed_item_ptr_seq_type d_items;


		//! Returns true if we have a current item.
		bool
		have_current_item() const
		{
			return d_current_item_handle != d_root_handle;
		}

		//! Returns current item (throws exception if @a have_current_item returns false).
		Item *
		get_current_item();

		/**
		 * Returns item identified by @a item_handle.
		 * Throws exception if @a item_handle is not a valid handle.
		 */
		Item *
		get_item(
				item_handle_type item_handle);

		/**
		 * Returns item identified by @a item_handle.
		 * Throws exception if @a item_handle is not a valid handle.
		 */
		const Item *
		get_item(
				item_handle_type item_handle) const;

		item_handle_type
		allocate_item(
				managed_item_ptr_type new_item);

		void
		deallocate_item(
				item_handle_type item_handle);

		void
		allocate_root_item();

		void
		destroy_item_without_removing_from_parent(
				item_handle_type item_handle);

		void
		remove_child(
				Item *parent_item,
				item_handle_type child_item_handle,
				unsigned int child_index);

		void
		visit_item_recursively(
				Item *item);

		bool
		transfer_managed_tree_widget_items_to_qlist(
				const item_handle_seq_type &item_seq,
				QList<QTreeWidgetItem *> &transfer_list,
				int *insert_child_index);

		void
		call_item_functions(
				Item *item);
	};

	/**
	 * Returns the QTreeWidgetItem associated with the current item.
	 * Throws exception if there's no current item.
	 * An example of no current is when only top-level children
	 * have been added so far (there's no root QTreeWidgetItem).
	 */
	QTreeWidgetItem *
	get_current_qtree_widget_item(
			TreeWidgetBuilder &tree_widget_builder);

	/**
	 * Creates top-level tree widget item.
	 * Returns item handle of top-level item.
	 */
	TreeWidgetBuilder::item_handle_type
	add_top_level_item(
			TreeWidgetBuilder &tree_widget_builder,
			const QString &name,
			const QString &value = QString());

	/**
	 * Creates top-level tree widget item.
	 * Returns item handle of top-level item.
	 */
	TreeWidgetBuilder::item_handle_type
	add_top_level_item(
			TreeWidgetBuilder &tree_widget_builder,
			const QStringList &fields = QStringList());

	/**
	 * Adds @a top_level_item_handle as top-level tree widget item.
	 * Note: @a top_level_item_handle must not have been added before.
	 */
	void
	add_top_level_item(
			TreeWidgetBuilder &tree_widget_builder,
			TreeWidgetBuilder::item_handle_type top_level_item_handle);

	/**
	 * Creates and adds a child tree widget item to the current item.
	 * Returns item handle of child item.
	 * If there's no current item then the tree widget item added is a
	 * top-level item.
	 */
	TreeWidgetBuilder::item_handle_type
	add_child_to_current_item(
			TreeWidgetBuilder &tree_widget_builder,
			const QString &name,
			const QString &value = QString());

	/**
	 * Creates and adds a child tree widget item to the current item.
	 * Returns item handle of child item.
	 * If there's no current item then the tree widget item added is a
	 * top-level item.
	 */
	TreeWidgetBuilder::item_handle_type
	add_child_to_current_item(
			TreeWidgetBuilder &tree_widget_builder,
			const QStringList &fields = QStringList());

	/**
	 * Creates and adds a child tree widget item to @a parent_item_handle.
	 * Returns item handle of child item.
	 */
	TreeWidgetBuilder::item_handle_type
	add_child(
			TreeWidgetBuilder &tree_widget_builder,
			const TreeWidgetBuilder::item_handle_type parent_item_handle,
			const QString &name,
			const QString &value = QString());

	/**
	 * Creates and adds a child tree widget item to @a parent_item_handle.
	 * Returns item handle of child item.
	 */
	TreeWidgetBuilder::item_handle_type
	add_child(
			TreeWidgetBuilder &tree_widget_builder,
			const TreeWidgetBuilder::item_handle_type parent_item_handle,
			const QStringList &fields = QStringList());

	/**
	 * Adds sequence of previously created children item handles to
	 * a previously created parent item.
	 */
	template <typename ItemHandleForwardIter>
	void
	add_children(
			TreeWidgetBuilder &tree_widget_builder,
			const TreeWidgetBuilder::item_handle_type parent_item_handle,
			ItemHandleForwardIter begin_child_item_handles,
			ItemHandleForwardIter end_child_item_handles)
	{
		std::for_each(begin_child_item_handles, end_child_item_handles,
				boost::bind(&TreeWidgetBuilder::add_child,
						boost::ref(tree_widget_builder), parent_item_handle, _1));
	}

	/**
	 * Adds sequence of previously created children item handles to
	 * the current item.
	 */
	template <typename ItemHandleForwardIter>
	void
	add_children_to_current_item(
			TreeWidgetBuilder &tree_widget_builder,
			ItemHandleForwardIter begin_child_item_handles,
			ItemHandleForwardIter end_child_item_handles)
	{
		const TreeWidgetBuilder::item_handle_type parent_item_handle =
				tree_widget_builder.get_current_item_handle();

		add_children(tree_widget_builder, parent_item_handle,
				begin_child_item_handles, end_child_item_handles);
	}

	/**
	 * Adds sequence of previously created children item handles
	 * as top-level items.
	 */
	template <typename ItemHandleForwardIter>
	void
	add_top_level_items(
			TreeWidgetBuilder &tree_widget_builder,
			ItemHandleForwardIter begin_child_item_handles,
			ItemHandleForwardIter end_child_item_handles)
	{
		const TreeWidgetBuilder::item_handle_type parent_item_handle =
				tree_widget_builder.get_root_handle();

		add_children(tree_widget_builder, parent_item_handle,
				begin_child_item_handles, end_child_item_handles);
	}

	/**
	 * Inserts @a top_level_item_handle as top-level tree widget item at index
	 * @a top_level_item_index.
	 * Note: @a top_level_item_handle must not have been added before.
	 */
	void
	insert_top_level_item(
			TreeWidgetBuilder &tree_widget_builder,
			TreeWidgetBuilder::item_handle_type top_level_item_handle,
			const unsigned int top_level_item_index);

	/**
	 * Destroys children from @a parent_item_handle.
	 */
	void
	destroy_children(
			TreeWidgetBuilder &tree_widget_builder,
			const TreeWidgetBuilder::item_handle_type parent_item_handle);

	/**
	 * Destroys all top-level items.
	 */
	void
	destroy_top_level_items(
			TreeWidgetBuilder &tree_widget_builder);

	/**
	 * Returns the number of top-level items.
	 */
	unsigned int
	get_num_top_level_items(
			TreeWidgetBuilder &tree_widget_builder);

	/**
	 * Returns item handle of a top-level item.
	 */
	TreeWidgetBuilder::item_handle_type
	get_top_level_item_handle(
			TreeWidgetBuilder &tree_widget_builder,
			const unsigned int top_level_item_index);

	/**
	 * Gets the QTreeWidgetItem of the child of @a parent_item_handle
	 * at index @a child_index.
	 */
	QTreeWidgetItem *
	get_child_qtree_widget_item(
			TreeWidgetBuilder &tree_widget_builder,
			TreeWidgetBuilder::item_handle_type parent_item_handle,
			const unsigned int child_index);

	/**
	 * Adds @a function to the current item.
	 * If there's no current item then an exception is thrown.
	 */
	void
	add_function_to_current_item(
			TreeWidgetBuilder &tree_widget_builder,
			const TreeWidgetBuilder::qtree_widget_item_function_type &function);
}

#endif // GPLATES_GUI_TREEWIDGETBUILDER_H
