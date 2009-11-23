/* $Id$ */

/**
 * \file 
 * 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_QTUNDOREDO_H
#define GPLATES_VIEWOPERATIONS_QTUNDOREDO_H

#include <vector>
#include <stack>
#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <QUndoGroup>
#include <QUndoStack>
#include <QUndoCommand>
#include <QString>

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "utils/Singleton.h"


namespace GPlatesViewOperations
{
	namespace UndoRedoInternal
	{
		class CommandIdFactory;
		class CommandIdImpl;
	}

	class UndoRedo :
		public GPlatesUtils::Singleton<UndoRedo>
	{
		GPLATES_SINGLETON_CONSTRUCTOR_DEF(UndoRedo)

	public:
		~UndoRedo();

		/**
		 * Typedef for a handle to a @a QUndoStack.
		 */
		typedef unsigned int UndoStackHandle;

		/**
		 * Handle of default @a QUndoStack.
		 */
		static const UndoStackHandle DEFAULT_UNDO_STACK_HANDLE = 0;

		/**
		 * Returns sole instance of @a QUndoGroup.
		 */
		QUndoGroup&
		get_undo_group();

		/**
		 * Creates a @a QUndoStack and adds it to sole @a QUndoGroup instance.
		 *
		 * Only needed if default undo stack is not sufficient.
		 */
		UndoStackHandle
		create_undo_stack();

		/**
		 * Returns current active @a QUndoStack.
		 *
		 * If no undo stacks have been created with @a create_undo_stack() then
		 * returns default undo stack created in constructor.
		 */
		QUndoStack&
		get_active_undo_stack();

		/*
		 * Sets the currently active @a QUndoStack.
		 *
		 * Before any calls to @a set_active_undo_stack() the default undo stack is active.
		 */
		void
		set_active_undo_stack(
				UndoStackHandle undo_stack_handle);

		/**
		 * Wrapper around a unique integer id to be used by QUndoCommand derivations.
		 */
		class CommandId
		{
		public:
			typedef boost::shared_ptr<UndoRedoInternal::CommandIdImpl> impl_ptr_type;

			/**
			 * Creates command with id of -1.
			 * This will prevent Qt from merging two adjacent commands.
			 */
			CommandId();

			CommandId(
					impl_ptr_type);

			//! Returns integer id.
			int
			get_id() const;

		private:
			impl_ptr_type d_command_id_impl;
		};

		/**
		 * Returns a unique command id.
		 *
		 * Returned @a CommandId retains unique id until last copy of it
		 * is destroyed at which point the id is released for reuse.
		 */
		CommandId
		get_unique_command_id();

		/**
		 * Generates a unique command id and stores internally.
		 * Starts a new scope in which id is alive.
		 * Scopes can be nested.
		 */
		void
		begin_unique_command_id_scope();

		/**
		 * Releases unique command id generated in matching @a begin_unique_command_id_scope()
		 * provided no copies of command id still exist.
		 * Ends scopes in which id is alive.
		 * Scopes can be nested.
		 */
		void
		end_unique_command_id_scope();

		/**
		 * Returns unique command id generated in current scope.
		 * If not currently in a scope then returns default unique id.
		 *
		 * Returned @a CommandId retains unique id even if the scope in which
		 * it was created has been exited. Only when all copies of returned
		 * command id are destroyed and the scope in which it was created is exited
		 * will the id be released for reuse.
		 */
		CommandId
		get_unique_command_id_scope() const;

		/**
		 * A convenience structure for automating calls to
		 * @a begin_unique_command_id_scope() and @a end_unique_command_id_scope()
		 * in a scope block.
		 */
		struct UniqueCommandIdScopeGuard :
			public boost::noncopyable
		{
			UniqueCommandIdScopeGuard();

			~UniqueCommandIdScopeGuard();
		};

		/**
		 * General way to merge unrelated undo commands (that don't know about each other).
		 * This converts an existing undo command into one that can be merged
		 * with commands adjacent to it in the undo stack.
		 *
		 * Only commands returned from @a make_mergable_undo_command() and that have the
		 * same id will be merged together when pushed onto undo stack next to
		 * each other (ie, in sequential @a QUndoStack push operations).
		 *
		 * @param  std::auto_ptr<QUndoCommand> existing command.
		 * @param  merge_id used to determine which adjacent merge commands to merge with.
		 * @return new command containing existing command.
		 */
		std::auto_ptr<QUndoCommand>
		make_mergable_undo_command(
				std::auto_ptr<QUndoCommand>,
				CommandId merge_id);

		/**
		 * Same as above except uses command id returned from
		 * @a get_unique_command_id_scope.
		 */
		std::auto_ptr<QUndoCommand>
		make_mergable_undo_command_in_current_unique_command_id_scope(
				std::auto_ptr<QUndoCommand>);

		/**
		 * Undo/redo command for grouping child commands into one command.
		 * The base @a QUndoCommand already does this - we just add
		 * @a RenderedGeometryCollection update guards to ensure only one
		 * update signal is generated within a @a redo or @a undo call.
		 */
		class GroupUndoCommand :
			public QUndoCommand
		{
		public:
			GroupUndoCommand(
					const QString &text,
					QUndoCommand *parent = 0);

			virtual
			void
			redo();

			virtual
			void
			undo();
		};

	private:
		//! Typedef for sequence of @a QUndoStack pointers.
		typedef std::vector<QUndoStack *> undo_stack_ptr_seq_type;

		//! Typedef for stack of unique command ids in begin/end scopes.
		typedef std::stack<CommandId> unique_command_id_scope_stack;

		QUndoGroup d_undo_group;
		undo_stack_ptr_seq_type d_undo_stack_seq;
		UndoStackHandle d_active_stack_handle;
		unique_command_id_scope_stack d_unique_command_id_scope_stack;
		boost::shared_ptr<UndoRedoInternal::CommandIdFactory> d_command_id_factory;
	};
}

#endif // GPLATES_VIEWOPERATIONS_QTUNDOREDO_H
