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

#include "utils/Singleton.h"

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
	class GPlatesQtMsgHandler :
			public GPlatesUtils::Singleton<GPlatesQtMsgHandler>
	{

		GPLATES_SINGLETON_CONSTRUCTOR_DECL(GPlatesQtMsgHandler)

	public:
		~GPlatesQtMsgHandler();

		//! Default filename to log Qt messages to.
		static const QString DEFAULT_LOG_FILENAME;

		/**
		 * Uses @a qInstallMsgHandler to @a install_qt_message_handler as the sole Qt message handler.
		 * NOTE: only installs handler if any of the following conditions are satisfied:
		 *   1) GPLATES_PUBLIC_RELEASE is defined (automatically handled by CMake build system), or
		 *   2) GPLATES_OVERRIDE_QT_MESSAGE_HANDLER environment variable is set to case-insensitive
		 *      "true", "1", "yes" or "on".
		 * If handler is not installed then default Qt handler applies.
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

		int d_log_level;

		//
		// Instance methods
		//

		/**
		 * Handler method for Qt messages.
		 */
		void
		handle_qt_message(
				QtMsgType msg_type,
				const char * msg);

		/**
		 * Returns true if should install message handler.
		 */
		static
		bool
		should_install_message_handler();
	};
}

#endif // GPLATES_GUI_GPLATESQTMSGHANDLER_H
