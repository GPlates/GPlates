/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <vector>
#include <boost/cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include "UndoRedo.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesViewOperations
{
	namespace UndoRedoInternal
	{
		/**
		 * Manages allocation/deallocation of unique integer IDs.
		 */
		class CommandIdFactory
		{
		public:
			CommandIdFactory() :
			d_next_id(0)
			{  }

			int
			allocate_id()
			{
				if (!d_free_id_seq.empty())
				{
					const int free_id = d_free_id_seq.back();
					d_free_id_seq.pop_back();
					return free_id;
				}

				return d_next_id++;
			}

			void
			deallocate_id(
					int command_id)
			{
				d_free_id_seq.push_back(command_id);
			}

		private:
			typedef std::vector<int> free_id_seq_type;
			free_id_seq_type d_free_id_seq;
			int d_next_id;
		};

		/**
		 * Interface of @a CommandId pimpl.
		 */
		class CommandIdImpl
		{
		public:
			virtual
			~CommandIdImpl()
			{  }

			virtual
			int
			get_id() const = 0;
		};

		/**
		 * Non-null implementation of @a CommandId pimpl.
		 */
		class NonNullCommandIdImpl :
			public CommandIdImpl,
			private boost::noncopyable
		{
		public:
			explicit NonNullCommandIdImpl(CommandIdFactory *command_id_factory) :
				d_command_id_factory(command_id_factory)
			{
				d_command_id = d_command_id_factory->allocate_id();
			}

			~NonNullCommandIdImpl()
			{
				// Since this is a destructor we cannot let any exceptions escape.
				// If one is thrown we just have to lump it and continue on.
				try
				{
					d_command_id_factory->deallocate_id(d_command_id);
				}
				catch (...)
				{
				}
			}

			int
			get_id() const
			{
				return d_command_id;
			}

		private:
			CommandIdFactory *d_command_id_factory;
			int d_command_id;
		};

		/**
		 * Null implementation of @a CommandId pimpl.
		 */
		class NullCommandIdImpl :
			public CommandIdImpl,
			private boost::noncopyable
		{
		public:
			static
			NullCommandIdImpl &
			instance()
			{
				// Defining locally avoids problem with undefined initialisation
				// order of non-local static variables.
				static NullCommandIdImpl s_instance;
				return s_instance;
			}

			static
			void
			null_destroy(
					void *)
			{  }

			virtual
			int
			get_id() const
			{
				// -1 is used to indicate to Qt that commands won't have their ids compared.
				return -1;
			}

		private:
			NullCommandIdImpl()
			{  }
		};

		/**
		 * A decorator command that makes an existing undo command mergeable.
		 */
		class MergeUndoCommand :
			public QUndoCommand
		{
		public:
			MergeUndoCommand(
					std::auto_ptr<QUndoCommand> command,
					UndoRedo::CommandId command_id,
					QUndoCommand *parent = 0) :
			QUndoCommand(parent),
			d_command_id(command_id)
			{
				setText(command->text());

				d_command_seq.push_back(
						undo_command_ptr_type(command.release()));
			}

			virtual
			void
			redo()
			{
				// Delay any notification of changes to the rendered geometry collection
				// until end of current scope block.
				GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

				// Execute commands in normal order.
				std::for_each(
					d_command_seq.begin(),
					d_command_seq.end(),
					boost::bind(&QUndoCommand::redo, _1));
			}

			virtual
			void
			undo()
			{
				// Delay any notification of changes to the rendered geometry collection
				// until end of current scope block.
				GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

				// Execute commands in reverse order.
				std::for_each(
					d_command_seq.rbegin(),
					d_command_seq.rend(),
					boost::bind(&QUndoCommand::undo, _1));
			}

			virtual
			int
			id() const
			{
				return d_command_id.get_id();
			}

			/**
			 * Merge this command with another command.
			 * Returns @a true if merged in which case other command will be
			 * deleted by Qt and this command will perform both commands in future.
			 */
			virtual
			bool
			mergeWith(
					const QUndoCommand *other_command)
			{
				const MergeUndoCommand *other_merge_command =
					dynamic_cast<const MergeUndoCommand *>(other_command);

				// If other command is same type as us and has same id
				// then move its internal command to our list.
				if (other_merge_command != NULL &&
					other_merge_command->id() == id())
				{
					// Copy or merge the other list of commands to the end of our list.
					// The other merge command is about to get deleted by our caller because
					// we are going to return true.

					// See if commands at beginning of the other merge command's list
					// merge with the command at the end of our list.
					command_seq_type::const_iterator other_child_iter;
					for (other_child_iter = other_merge_command->d_command_seq.begin();
						other_child_iter != other_merge_command->d_command_seq.end();
						++other_child_iter)
					{
						const QUndoCommand *other_child_command = other_child_iter->get();

						// A command id of -1 means don't merge.
						if (other_child_command->id() == -1 ||
							other_child_command->id() != d_command_seq.back()->id() ||
							!d_command_seq.back()->mergeWith(other_child_command))
						{
							break;
						}

						// We only get here if the command merged - in which case we will
						// not be copying it to our command list.
					}

					// Copy to the end of our list any commands that didn't merge.
					d_command_seq.insert(
						d_command_seq.end(),
						other_child_iter,
						other_merge_command->d_command_seq.end());

					// Use the text of first command since that is probably most representative
					// of the group of merged undo commands. Nothing to do - that is already the case.

					return true;
				}

				return false;
			}

		private:
			typedef boost::shared_ptr<QUndoCommand> undo_command_ptr_type;
			typedef std::vector<undo_command_ptr_type> command_seq_type;
			command_seq_type d_command_seq;

			UndoRedo::CommandId d_command_id;
		};
	}
}

boost::scoped_ptr<GPlatesViewOperations::UndoRedo>
GPlatesViewOperations::UndoRedo::s_instance;

GPlatesViewOperations::UndoRedo &
GPlatesViewOperations::UndoRedo::instance()
{
	if (!s_instance)
	{
		s_instance.reset(new UndoRedo());
	}

	return *s_instance;
}

GPlatesViewOperations::UndoRedo::UndoRedo() :
d_active_stack_handle(DEFAULT_UNDO_STACK_HANDLE),
d_command_id_factory(new UndoRedoInternal::CommandIdFactory())
{
	// Create default undo stack and make it active.
	const UndoStackHandle default_stack_handle = create_undo_stack();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			default_stack_handle == DEFAULT_UNDO_STACK_HANDLE,
			GPLATES_ASSERTION_SOURCE);

	set_active_undo_stack(default_stack_handle);

	// Generate a unique command id scope that doesn't close until destructor.
	// Scopes can be nested so this won't interfere if a new scope is later created.
	begin_unique_command_id_scope();
}

GPlatesViewOperations::UndoRedo::~UndoRedo()
{
	// End the scope block started in constructor.
	end_unique_command_id_scope();
}

QUndoGroup &
GPlatesViewOperations::UndoRedo::get_undo_group()
{
	return d_undo_group;
}

GPlatesViewOperations::UndoRedo::UndoStackHandle
GPlatesViewOperations::UndoRedo::create_undo_stack()
{
	UndoStackHandle handle = d_undo_stack_seq.size();

	QUndoStack *new_undo_stack = new QUndoStack();
	get_undo_group().addStack(new_undo_stack);
	d_undo_stack_seq.push_back(new_undo_stack);

	return handle;
}

void
GPlatesViewOperations::UndoRedo::set_active_undo_stack(
		UndoStackHandle undo_stack_handle )
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			boost::numeric_cast<undo_stack_ptr_seq_type::size_type>(undo_stack_handle) <
					d_undo_stack_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	d_active_stack_handle = undo_stack_handle;
	d_undo_stack_seq[d_active_stack_handle]->setActive(true);
}

QUndoStack &
GPlatesViewOperations::UndoRedo::get_active_undo_stack()
{
	return *d_undo_stack_seq.back();
}

void
GPlatesViewOperations::UndoRedo::begin_unique_command_id_scope()
{
	d_unique_command_id_scope_stack.push(
		get_unique_command_id());
}

void
GPlatesViewOperations::UndoRedo::end_unique_command_id_scope()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
		!d_unique_command_id_scope_stack.empty(),
		GPLATES_ASSERTION_SOURCE);

	d_unique_command_id_scope_stack.pop();
}

GPlatesViewOperations::UndoRedo::CommandId
GPlatesViewOperations::UndoRedo::get_unique_command_id_scope() const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_unique_command_id_scope_stack.empty(),
			GPLATES_ASSERTION_SOURCE);

	return d_unique_command_id_scope_stack.top();
}

std::auto_ptr<QUndoCommand>
GPlatesViewOperations::UndoRedo::make_mergable_undo_command(
		std::auto_ptr<QUndoCommand> undo_command,
		CommandId merge_id)
{
	return std::auto_ptr<QUndoCommand>(
			new UndoRedoInternal::MergeUndoCommand(
					undo_command,
					merge_id));
}

std::auto_ptr<QUndoCommand>
GPlatesViewOperations::UndoRedo::make_mergable_undo_command_in_current_unique_command_id_scope(
		std::auto_ptr<QUndoCommand> undo_command)
{
	const CommandId merge_id = get_unique_command_id_scope();

	return make_mergable_undo_command(undo_command, merge_id);
}

GPlatesViewOperations::UndoRedo::CommandId
GPlatesViewOperations::UndoRedo::get_unique_command_id()
{
	boost::shared_ptr<UndoRedoInternal::CommandIdImpl> command_id_impl(
		new UndoRedoInternal::NonNullCommandIdImpl(d_command_id_factory.get()));

	return CommandId(command_id_impl);
}

GPlatesViewOperations::UndoRedo::UniqueCommandIdScopeGuard::UniqueCommandIdScopeGuard()
{
	UndoRedo::instance().begin_unique_command_id_scope();
}

GPlatesViewOperations::UndoRedo::UniqueCommandIdScopeGuard::~UniqueCommandIdScopeGuard()
{
	// Since this is a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		UndoRedo::instance().end_unique_command_id_scope();
	}
	catch (...)
	{
	}
}

GPlatesViewOperations::UndoRedo::GroupUndoCommand::GroupUndoCommand(
		const QString &text_,
		QUndoCommand *parent_):
	QUndoCommand(text_, parent_)
{
}

void
GPlatesViewOperations::UndoRedo::GroupUndoCommand::redo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Visit child commands.
	QUndoCommand::redo();
}

void
GPlatesViewOperations::UndoRedo::GroupUndoCommand::undo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Visit child commands.
	QUndoCommand::undo();
}

GPlatesViewOperations::UndoRedo::CommandId::CommandId() :
d_command_id_impl(
		&UndoRedoInternal::NullCommandIdImpl::instance(),
		&UndoRedoInternal::NullCommandIdImpl::null_destroy)
{
}

GPlatesViewOperations::UndoRedo::CommandId::CommandId(
		impl_ptr_type command_id_impl) :
d_command_id_impl(command_id_impl)
{
}

int
GPlatesViewOperations::UndoRedo::CommandId::get_id() const
{
	return d_command_id_impl->get_id();
}
