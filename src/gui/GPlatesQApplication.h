/* $Id$ */

/**
 * \file Used to override methods in QApplication.
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

#ifndef GPLATES_GUI_GPLATESQAPPLICATION_H
#define GPLATES_GUI_GPLATESQAPPLICATION_H

#include <QtGui/QApplication>


namespace GPlatesGui
{
	class GPlatesQApplication :
			public QApplication
	{
	public:
		GPlatesQApplication(
				int &_argc,
				char **_argv):
			QApplication(_argc, _argv)
		{  }

		/**
		 * This Qt method is overridden in order to catch any uncaught exceptions in
		 * the Qt event handling thread.
		 * Pops up a dialog informing user of uncaught exception, records exception's
		 * call stack trace from the location at which it was thrown and logs this
		 * information with the currently installed Qt message handler.
		 */
		virtual
		bool
		notify(
				QObject *,
				QEvent *);

		/**
		 * Calls the main-like function @a main_function and handles any
		 * uncaught exceptions.
		 * Pops up a dialog informing user of uncaught exception, records exception's
		 * call stack trace from the location at which it was thrown and logs this
		 * information with the currently installed Qt message handler.
		 */
		static
		int
		call_main(
				int (*main_function)(int, char* []),
				int argc,
				char* argv[]);
	};
}

#endif // GPLATES_GUI_GPLATESQAPPLICATION_H
