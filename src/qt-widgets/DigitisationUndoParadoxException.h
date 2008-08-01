/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

// FIXME: This exception should travel to whatever namespace/directory that the
// DigitisationWidgetUndoCommands.h does.
#ifndef GPLATES_QTWIDGETS_DIGITISATIONUNDOPARADOXEXCEPTION_H
#define GPLATES_QTWIDGETS_DIGITISATIONUNDOPARADOXEXCEPTION_H

#include "global/AssertionFailureException.h"

namespace GPlatesQtWidgets
{
	/**
	 * An AssertionFailureException that indicates a paradox has occurred in the
	 * DigitisationWidget's QUndoStack - an undo command previously pushed
	 * onto the stack has been undone, but encountered a situation which should
	 * not exist e.g:-
	 *  1. DigitisationAddPoint is created and pushed onto the stack,
	 *  2. DigitisationAddPoint::redo() is called, modifying the state of
	 *     the DigitisationWidget.
	 *  3. The user requests an undo,
	 *  4. DigitisationAddPoint::undo() is called, but the state of 
	 *     the DigitisationWidget does not match the state it should be in
	 *     after the call to ::redo() - the coordinate we are supposed to
	 *     remove does not exist, or the table is completely empty.
	 *
	 * These kinds of undo paradoxes can occur if the undo() and redo() methods
	 * of an undo command were not set up to properly cancel each other out,
	 * or if the state has been manipulated by a force not managed by undo/redo
	 * commands. Either way, it is a programming error and an exception should
	 * be raised.
	 */
	class DigitisationUndoParadoxException : 
			public GPlatesGlobal::AssertionFailureException
	{
	public:
		/**
		 * @param filename_ should be supplied using the @c __FILE__ macro,
		 * @param line_num_ should be supplied using the @c __LINE__ macro.
		 *
		 * FIXME: Ideally, we'd be tracking the call stack etc, and also supplying
		 * some sort of function object that might be used to do damage control
		 * for the program should such an exception be thrown. For example, the
		 * DigitisationWidgetUndoCommands have a few exceptional 'should never
		 * ever reach here' states. The 'recovery' function of those exceptions
		 * might be to clear the digitisation widget and wipe the undo stack clean,
		 * restoring the widget to a known sane state, and then alerting the user.
		 */
		explicit
		DigitisationUndoParadoxException(
				const char *filename_,
				int line_num_):
			AssertionFailureException(filename_, line_num_)
		{  }

		virtual
		~DigitisationUndoParadoxException()
		{  }

	protected:
	
		virtual
		const char *
		ExceptionName() const
		{
			return "DigitisationUndoParadoxException";
		}

	private:

	};
}

#endif  // GPLATES_QTWIDGETS_DIGITISATIONUNDOPARADOXEXCEPTION_H
