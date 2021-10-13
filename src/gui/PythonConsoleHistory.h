/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_PYTHONCONSOLEHISTORY_H
#define GPLATES_GUI_PYTHONCONSOLEHISTORY_H

#include <list>
#include <vector>
#include <boost/optional.hpp>
#include <QString>


namespace GPlatesGui
{
	/**
	 * This class encapsulates the logic behind the history functionality in the
	 * Python console dialog. The behaviour is designed to mimic that of Bash and
	 * the interactive Python interpreter.
	 */
	class PythonConsoleHistory
	{
	public:

		static const std::size_t MAX_HISTORY_SIZE = 80;

		explicit
		PythonConsoleHistory();

		/**
		 * Handles the case when the user pressses "up" to get the previous
		 * command in the history stack, as modified. If there is a previous
		 * command in the history stack (i.e. the current command is not the
		 * oldest), the current command is replaced with @a current_command and
		 * the previous command is returned; otherwise, @a boost::none is
		 * returned and the current command is unmodified.
		 */
		boost::optional<QString>
		get_previous_command(
				const QString &current_command);

		/**
		 * Handles the case when the user presses "down" to get the next
		 * command in the history stack, as modified. If there is a next
		 * command in the history stack (i.e. the current command is not the
		 * newest), the current command is replaced with @a current_command and
		 * the next command is returneed; otherwise @a boost::none is
		 * returned and the current command is unmodified.
		 */
		boost::optional<QString>
		get_next_command(
				const QString &current_command);

		/**
		 * Handles the case when the user presses "enter" and commits the given
		 * @a command as the newest command.
		 */
		void
		commit_command(
				const QString &command);

		/**
		 * Discards any changes made to the modifiable history by the user and
		 * starts afresh with a new copy of the history; this handles the case
		 * when the user presses Ctrl+C while entering a command.
		 */
		void
		reset_modifiable_history();

	private:

		/**
		 * A list of commands in the order that the user entered them. If the
		 * size of this list exceeds @a MAX_HISTORY_SIZE, items are popped off,
		 * oldest first. New items are added to the end of the list. The list is
		 * unmodifiable in the sense that even if the user edits old history
		 * items, this is not propagated back to this list.
		 */
		std::list<QString> d_unmodifiable_history;

		/**
		 * A copy of @a d_unmodifiable_history plus an extra entry at the back
		 * for a new command. This is modifiable in the sense that if the
		 * user edits old history items, the changes are reflected back into
		 * here. However, this modifiable history is cleaned out and recreated
		 * from the unmodifiable history when the user presses enter and commits
		 * their command.
		 */
		std::vector<QString> d_modifiable_history;

		/**
		 * An iterator into @a d_modifiable_history pointing at which item the
		 * user is currently modifying.
		 */
		std::vector<QString>::iterator d_modifiable_history_iter;
	};
}


#endif	// GPLATES_GUI_PYTHONCONSOLEHISTORY_H
