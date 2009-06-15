/* $Id$ */

/**
 * \file A @a QtMsgHandler used to output debug, warning, critical and fatal Qt messages.
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

#ifndef GPLATES_GUI_GPLATESQTMSGHANDLER_H
#define GPLATES_GUI_GPLATESQTMSGHANDLER_H

#include <boost/scoped_ptr.hpp>
#include <QFile>
#include <QString>
#include <QTextStream>
#include <QtGlobal>


namespace GPlatesGui
{
	/**
	 * A Qt message handler to log qDebug, qWarning, qFatal, etc messages to a file.
	 */
	class GPlatesQtMsgHandler
	{
	public:
		~GPlatesQtMsgHandler();

		//! Default filename to log Qt messages to.
		static const QString DEFAULT_LOG_FILENAME;

		/**
		 * Uses @a qInstallMsgHandler to install_qt_message_handler this class as a Qt message handler.
		 * WARNING: installing this handler removes the default Qt handler and since this
		 * handler only outputs to a log file the qDebug(), etc messages will not get output
		 * to their default locations (eg, the console window). As a result only install this
		 * handler when releasing GPlates to the public (ie, non-developers).
		 * This handler is uninstalled when its singleton instance is destroyed
		 * at application exit (and the previous handler is reinstalled).
		 */
		static
		void
		install_qt_message_handler(
				const QString &log_filename = DEFAULT_LOG_FILENAME);

		/**
		 * The message handler function called by Qt.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 */
		static
		void
		qt_message_handler(
				QtMsgType msg_type,
				const char * msg);

	private:
		//
		// Static data members
		//

		//! Next Qt message handler in the chain of message handlers.
		static QtMsgHandler s_prev_msg_handler;

		//! Name of the log file to write to.
		static QString s_log_filename;

		//
		// Instance data members
		//

		QFile d_log_file;
		boost::scoped_ptr<QTextStream> d_log_stream;

		//
		// Static methods
		//

		//! Singleton instance.
		static
		GPlatesQtMsgHandler &
		instance()
		{
			// Singleton instance.
			static GPlatesQtMsgHandler s_msg_handler;

			return s_msg_handler;
		}

		//
		// Instance methods
		//

		//! Singleton constructor is private.
		GPlatesQtMsgHandler();

		/**
		 * Handler method for Qt messages.
		 */
		void
		handle_qt_message(
				QtMsgType msg_type,
				const char * msg);
	};
}

#endif // GPLATES_GUI_GPLATESQTMSGHANDLER_H
