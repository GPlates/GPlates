/* $Id$ */

/**
 * \file A @a GPlatesQtMsgHandler::MessageHandler that logs to a file.
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

#ifndef GPLATES_FILEIO_LOGTOFILEHANDLER_H
#define GPLATES_FILEIO_LOGTOFILEHANDLER_H

#include <QString>
#include <QtGlobal>
#include <QFile>
#include <QTextStream>
#include <boost/scoped_ptr.hpp>
#include <iostream>

#include "app-logic/GPlatesQtMsgHandler.h"


namespace GPlatesFileIO
{
	/**
	 * A derivation of GPlatesQtMsgHandler::MessageHandler that logs to a file on disk.
	 * This is enabled for release builds so that devs can do a post-mortem if something
	 * goes wrong. The default filename is GPlates_log.txt.
	 */
	class LogToFileHandler :
			public GPlatesAppLogic::GPlatesQtMsgHandler::MessageHandler
	{
	public:
	
		/**
		 * Default constructor for LogToFileHandler; optionally takes the filename
		 * to log to, which will be dumped into the current working directory unless
		 * a full pathname is specified.
		 */
		explicit
		LogToFileHandler(
				const QString &log_filename = DEFAULT_LOG_FILENAME);

		/**
		 * Special constructor to allow you to log to stderr.
		 */
		explicit
		LogToFileHandler(
				FILE *output_file_ptr);
		
		virtual
		~LogToFileHandler();

		virtual
		void
		handle_qt_message(
				QtMsgType msg_type,
				const char *msg);

	private:

		//! Default filename to log Qt messages to.
		static const QString DEFAULT_LOG_FILENAME;

		/**
		 * The file we log to.
		 */
		QFile d_log_file;
		
		/**
		 * QTextStream to log with.
		 */
		boost::scoped_ptr<QTextStream> d_log_stream;

		/**
		 * QtMsgType log messages of this level and above will be logged
		 * to file; those below it will be ignored.
		 */
		int d_log_level;
	};
	
}

#endif //GPLATES_FILEIO_LOGTOFILEHANDLER_H

