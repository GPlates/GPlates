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

#include "PythonConsoleHistory.h"


GPlatesGui::PythonConsoleHistory::PythonConsoleHistory()
{
	reset_modifiable_history();
}


void
GPlatesGui::PythonConsoleHistory::reset_modifiable_history()
{
	std::vector<QString> new_modifiable_history(
			d_unmodifiable_history.begin(),
			d_unmodifiable_history.end());
	new_modifiable_history.push_back(QString());

	d_modifiable_history.swap(new_modifiable_history);
	d_modifiable_history_iter = (d_modifiable_history.end() - 1);
}


boost::optional<QString>
GPlatesGui::PythonConsoleHistory::get_previous_command(
		const QString &current_command)
{
	if (d_modifiable_history_iter == d_modifiable_history.begin())
	{
		return boost::none;
	}

	*d_modifiable_history_iter = current_command;
	--d_modifiable_history_iter;
	return *d_modifiable_history_iter;
}


boost::optional<QString>
GPlatesGui::PythonConsoleHistory::get_next_command(
		const QString &current_command)
{
	if (d_modifiable_history_iter + 1 == d_modifiable_history.end())
	{
		return boost::none;
	}

	*d_modifiable_history_iter = current_command;
	++d_modifiable_history_iter;
	return *d_modifiable_history_iter;
}


void
GPlatesGui::PythonConsoleHistory::commit_command(
		const QString &command)
{
	// Add the command to the unmodifiable history if it is not the same as
	// the last command, and if the command is not the empty string.
	if (!command.isEmpty() && (d_unmodifiable_history.size() == 0 ||
			d_unmodifiable_history.back() != command))
	{
		d_unmodifiable_history.push_back(command);

		// Pop off the front if too full.
		if (d_unmodifiable_history.size() > MAX_HISTORY_SIZE)
		{
			d_unmodifiable_history.pop_front();
		}
	}

	reset_modifiable_history();
}

