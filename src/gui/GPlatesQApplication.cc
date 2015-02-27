/* $Id$ */

/**
 * \file Used to override methods in QApplication.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2011 The University of Sydney, Australia
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

#include "global/Constants.h"
#include "global/GPlatesException.h"
#include "global/SubversionInfo.h"

#include "presentation/Application.h"

#include "qt-widgets/ViewportWindow.h"

#include "utils/DeferredCallEvent.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace
{
	/**
	 * Call function @a func and process any uncaught exceptions.
	 */
	template <typename ReturnType>
	ReturnType
	try_catch(
			boost::function<ReturnType ()> func,
			QObject *qreceiver,
			QEvent *qevent)
	{
#if !defined(GPLATES_DEBUG)
		std::string error_message_std;
		std::string call_stack_trace_std;
#endif

		try
		{
			return func();
		}
		catch (GPlatesGlobal::NeedExitException &ex)
		{
			std::ostringstream os;
			os << ex;
			qDebug() << os.str().c_str();
			//use exception to exit gplates is better than call exit(0) directly.
			return true;
		}
		// For debug builds we don't want to catch exceptions (except 'NeedExitException')
		// because if we do then we lose the debugger call stack trace which is
		// much more detailed than our own stack trace implementation that
		// currently requires placing TRACK_CALL_STACK macros around the code.
		// And, of course, debugging relies on the native debugger stack trace.
#if !defined(GPLATES_DEBUG)
		catch (GPlatesGlobal::Exception &exc)
		{
			// Get exception to write its message.
			std::ostringstream ostr_stream;
			ostr_stream << exc;
			error_message_std = ostr_stream.str();

			// Extract the call stack trace to the location where the exception was thrown.
			exc.get_call_stack_trace_string(call_stack_trace_std);
		}
		catch(std::exception& exc)
		{
			error_message_std = exc.what();
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
			// This also applies when GPlates is used for command-line processing
			// (ie, when it's not used as a GUI).
			QMessageBox::critical(NULL,
					QObject::tr("Error: unhandled GPlates exception"),
					error_message,
					QMessageBox::Ok,
					QMessageBox::Ok);
		}

		// If we have an installed message handler then this will output to a log file.
		qWarning() << error_message;

		// Output the call stack trace if we have one.
		if (!call_stack_trace_std.empty())
		{
			// If we have an installed message handler then this will output to a log file.
			// Also write out the SVN revision number so we know which source code to look
			// at when users send us back a log file.
			qWarning()
					<< QString::fromStdString(call_stack_trace_std)
					<< endl
					<< GPlatesGlobal::SubversionInfo::get_working_copy_version_number();
		}

		// If we have an installed message handler then this will output to a log file.
		// This is where the core dump or debugger trigger happens on debug builds.
		// On release builds this exits the application with a return value of 1.
		qFatal("Exiting due to exception caught");

		// Shouldn't get past qFatal - this just keeps compiler happy.
		return false;
#endif // if !defined(GPLATES_DEBUG)
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
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This is located here because this is the highest level of the GUI event
	// that captures a single user interaction - the user performs an action and
	// we update canvas once.
	// But since these guards can be nested it's ok to have them further down
	// the call stack if needed for some reason.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	return try_catch<bool>(
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
	return try_catch<int>(
		boost::bind(main_function, argc, argv),
		NULL/*qreceiver*/,
		NULL/*qevent*/);
}


bool
GPlatesGui::GPlatesQApplication::event(
		QEvent *ev)
{
	if (ev->type() == GPlatesUtils::AbstractDeferredCallEvent::TYPE)
	{
		// Because this object lives on the main GUI thread, we process all
		// DeferredCallEvents destined to be executed on the main thread here, to save
		// every class that uses DeferredCallEvents from having to handle this
		// themselves.
		GPlatesUtils::AbstractDeferredCallEvent *deferred_call_event =
			static_cast<GPlatesUtils::AbstractDeferredCallEvent *>(ev);
		deferred_call_event->execute();

		return true;
	}
	else if (ev->type() == QEvent::FileOpen)
	{
		// If the filename looks like a project file then load it.
		//
		// NOTE: QFileOpenEvent is MacOS specific.
		// See http://doc.qt.digia.com/qq/qq18-macfeatures.html#newevents
		// This event is triggered when a file is double-clicked in Finder (and the user has
		// associated the file type with GPlates).
		//
		// For now we only support project files (not feature collection files) since it makes
		// more sense to open a single project file. Also there's the issue of whether to open
		// multiple feature collection files in a single GPlates instance or one per GPlates instance.
		// I think the latter happens by default. For project files this is fine since a single
		// GPlates instance should only open a single project file.
		// In any case we can add the ability to load feature collection files if it's requested.
		const QString project_filename = static_cast<QFileOpenEvent *>(ev)->file();
		if (project_filename.endsWith(QString(".gproj"), Qt::CaseInsensitive))
		{
			GPlatesPresentation::Application::instance().get_main_window().load_project(project_filename);
			return true;
		}
	}

	return QApplication::event(ev);
}

