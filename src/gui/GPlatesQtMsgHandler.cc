/* $Id$ */

/**
 * \file 
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

#include "GPlatesQtMsgHandler.h"

#include "file-io/ErrorOpeningFileForWritingException.h"


QtMsgHandler GPlatesGui::GPlatesQtMsgHandler::s_prev_msg_handler = NULL;

const QString GPlatesGui::GPlatesQtMsgHandler::DEFAULT_LOG_FILENAME = "GPlates_log.txt";

QString GPlatesGui::GPlatesQtMsgHandler::s_log_filename(DEFAULT_LOG_FILENAME);


GPlatesGui::GPlatesQtMsgHandler::GPlatesQtMsgHandler() :
	d_log_file(s_log_filename)
{
	if ( ! d_log_file.open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
				s_log_filename);
	}

	d_log_stream.reset( new QTextStream(&d_log_file) );
}

	
GPlatesGui::GPlatesQtMsgHandler::~GPlatesQtMsgHandler()
{
	// Reinstall the previous message handler.
	qInstallMsgHandler(s_prev_msg_handler);
}


void
GPlatesGui::GPlatesQtMsgHandler::install_qt_message_handler(
		const QString &log_filename)
{
	s_log_filename = log_filename;

	// Create the singleton instance now so that the log file gets cleared.
	// This needs to be done in case no Qt messages are output and hence no
	// log file is created - leaving the old log file in place.
	try
	{
		instance();
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &)
	{
		// If we can't open the log file for writing then just return before
		// installing message handler.
		qWarning()
				<< "Failed to install message handler because '"
				<< log_filename
				<< "' cannot be opened for writing";

		return;
	}

	// Install our message handler and keep track of the previous message handler.
	s_prev_msg_handler = qInstallMsgHandler(qt_message_handler);
}


void
GPlatesGui::GPlatesQtMsgHandler::qt_message_handler(
		QtMsgType msg_type,
		const char * msg)
{
	instance().handle_qt_message(msg_type, msg);

	 // Call the next message handler in the chain if there is one.
	 if (s_prev_msg_handler)
	 {
		 s_prev_msg_handler(msg_type, msg);
	 }
}


void
GPlatesGui::GPlatesQtMsgHandler::handle_qt_message(
		QtMsgType msg_type,
		const char * msg)
{
     switch (msg_type)
	 {
#if 0 // Don't print debug messages - there's too many - and they are not useful to the user.
     case QtDebugMsg:
		 d_log_stream << "Debug: " << msg << endl;
         break;
#endif

     case QtWarningMsg:
		 *d_log_stream << "Warning: " << msg << endl;
         break;

     case QtCriticalMsg:
		 // Note: system and critical messages have the same enumeration value.
		 *d_log_stream << "Critical: " << msg << endl;
         break;

     case QtFatalMsg:
		 *d_log_stream << "Fatal: " << msg << endl;
         break;

	 default:
		 break;
     }
}
