/* $Id$ */

/**
 * \file 
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
#include <iostream>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "GPlatesQtMsgHandler.h"

#include "file-io/LogToFileHandler.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "utils/Environment.h"


QtMsgHandler GPlatesAppLogic::GPlatesQtMsgHandler::s_prev_msg_handler = NULL;


GPlatesAppLogic::GPlatesQtMsgHandler::GPlatesQtMsgHandler()
{  }

	
GPlatesAppLogic::GPlatesQtMsgHandler::~GPlatesQtMsgHandler()
{
	// Reinstall the previous message handler.
	qInstallMsgHandler(s_prev_msg_handler);
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::install_qt_message_handler(
		const QString &log_filename)
{
	// Determine if we should even install the message handler.
	if ( ! should_install_message_handler())
	{
		return;
	}

	// Create the singleton instance now so that the log file gets cleared.
	// This needs to be done in case no Qt messages are output and hence no
	// log file is created - leaving the old log file in place.
	try
	{
		// Set up a LogToFile handler for our log file.
		instance().add_handler(boost::shared_ptr<MessageHandler>(
				new GPlatesFileIO::LogToFileHandler(log_filename)));

		// Set up a LogToFile handler for STDERR to compensate for the default output handler being removed.
		instance().add_handler(boost::shared_ptr<MessageHandler>(
				new GPlatesFileIO::LogToFileHandler(stderr)));
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &e)
	{
		// If we can't open the log file for writing then just return before
		// installing message handler.
		qWarning() << "Failed to install message handler because" << e.filename() << "cannot be opened for writing.";
		return;
	}

	// Install our message handler and keep track of the previous message handler.
	s_prev_msg_handler = qInstallMsgHandler(qt_message_handler);
}


bool
GPlatesAppLogic::GPlatesQtMsgHandler::should_install_message_handler()
{
	/*
	 * Overrides default Qt message handler unless the GPLATES_OVERRIDE_QT_MESSAGE_HANDLER
	 * environment variable is set to 'no' - in case developers want to use the built-in
	 * Qt message handler only. The message handler determines what happens when qDebug(),
	 * qWarning(), qCritical() and qFatal() are called.
	 */

	// We should override Qt's message handler by default,
	// unless GPLATES_OVERRIDE_QT_MESSAGE_HANDLER is defined and false. ("false", "0", "no" etc)
	bool default_should_install = true;

	return GPlatesUtils::getenv_as_bool("GPLATES_OVERRIDE_QT_MESSAGE_HANDLER", default_should_install);
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::qt_message_handler(
		QtMsgType msg_type,
		const char * msg)
{
	// Delegate message handling to our MessageHandlers.
	instance().handle_qt_message(msg_type, msg);

	// Call the original Qt message handler if there is one.
	if (s_prev_msg_handler)	{
		s_prev_msg_handler(msg_type, msg);
	}
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::add_handler(
		boost::shared_ptr<GPlatesAppLogic::GPlatesQtMsgHandler::MessageHandler> handler)
{
	d_message_handlers.push_back(handler);
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::handle_qt_message(
		QtMsgType msg_type,
		const char *msg)
{
	BOOST_FOREACH(boost::shared_ptr<MessageHandler> handler, d_message_handlers)
	{
		if (handler) {
			handler->handle_qt_message(msg_type, msg);
		}
	}
}

