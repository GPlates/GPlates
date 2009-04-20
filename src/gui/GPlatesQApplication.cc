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

#include <string>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <QDebug>
#include <QMessageBox>
#include <QStringList>

#include "GPlatesQApplication.h"

#include "global/GPlatesException.h"


namespace
{
	/**
	 * Call function @a func and process any uncaught exceptions.
	 */
	template <typename ReturnType>
	ReturnType
	call_function(
			boost::function<ReturnType ()> func,
			QObject *qreceiver,
			QEvent *qevent)
	{
		std::string error_message_std;
		std::string call_stack_trace_std;

		try
		{
			return func();
		}
		catch (GPlatesGlobal::Exception &exc)
		{
			// Get exception to write its message.
			std::ostringstream ostr_stream;
			ostr_stream << exc;
			error_message_std = ostr_stream.str();

			// Extract the call stack trace to the location where the exception was thrown.
			exc.get_call_stack_trace_string(call_stack_trace_std);
		}
		catch (...)
		{
			error_message_std = "unknown exception";
		}

		//
		// If we get here then we caught an exception
		//

		QStringList error_message_stream;
		if (qreceiver && qevent)
		{
			error_message_stream
					<< QObject::tr("Error: GPlates has caught an unhandled exception from '")
					<< qreceiver->objectName()
					<< QObject::tr("' from event type ")
					<< QString::number(qevent->type())
					<< ": "
					<< QString::fromStdString(error_message_std);
		}
		else
		{
			error_message_stream
					<< QObject::tr("Error: GPlates has caught an unhandled exception: ")
					<< QString::fromStdString(error_message_std);
		}

		QString error_message = error_message_stream.join("");

		if (qreceiver && qevent)
		{
			// Pop up a dialog letting the user know what happened.
			// Only do this if we're in the Qt event thread otherwise
			// it seems to crash (if an exception is thrown in 'main()' before
			// QApplication::exec() is called).
			QMessageBox::critical(NULL,
					QObject::tr("Error: unhandled GPlates exception"),
					error_message,
					QMessageBox::Ok,
					QMessageBox::Ok);
		}

		// If we have an installed message handler then this will also output to a log file.
		qWarning() << error_message;

		// Output the call stack trace if we have one.
		if (!call_stack_trace_std.empty())
		{
			// If we have an installed message handler then this will also output to a log file.
			// Also write out the SVN revision number so we know which source code to look
			// at when users send us back a log file.
			qWarning()
					<< QString::fromStdString(call_stack_trace_std)
					<< endl
					<< "SVN $Revision$";
		}

		// If we have an installed message handler then this will also output to a log file.
		qFatal("Exiting due to exception caught");

		// Shouldn't get past qFatal - this just keeps compiler happy.
		return false;
	}


	/**
	 * Convenience function to call base class @a QApplication::notify method.
	 */
	bool
	qapplication_notify(
			QApplication *qapplication,
			QObject *qreceiver,
			QEvent *qevent)
	{
		return qapplication->QApplication::notify(qreceiver, qevent);
	}
}


bool
GPlatesGui::GPlatesQApplication::notify(
		QObject *qreceiver,
		QEvent *qevent)
{
	return call_function<bool>(
		boost::bind(&qapplication_notify, this, qreceiver, qevent),
		qreceiver,
		qevent);
}


int
GPlatesGui::GPlatesQApplication::call_main(
		int (*main_function)(int, char* []),
		int argc,
		char* argv[])
{
	return call_function<int>(
		boost::bind(main_function, argc, argv),
		NULL/*qreceiver*/,
		NULL/*qevent*/);
}
