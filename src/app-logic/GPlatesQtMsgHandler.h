/* $Id$ */

/**
 * \file A @a QtMsgHandler that delegates messages to other logging classes.
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

#ifndef GPLATES_APP_LOGIC_GPLATESQTMSGHANDLER_H
#define GPLATES_APP_LOGIC_GPLATESQTMSGHANDLER_H

#include "utils/Singleton.h"

#include <vector>
#include <boost/shared_ptr.hpp>
#include <QtGlobal>
#include <QString>


namespace GPlatesAppLogic
{
	/**
	 * A Qt message handler to log qDebug, qWarning, qFatal, etc messages to file and to a log dialog.
	 * It delegates responsibility to GPlatesFileIO::LogToFileHandler and GPlatesAppLogic::LogToModelHandler.
	 */
	class GPlatesQtMsgHandler :
			public GPlatesUtils::Singleton<GPlatesQtMsgHandler>
	{
		GPLATES_SINGLETON_CONSTRUCTOR_DECL(GPlatesQtMsgHandler)

	public:
		~GPlatesQtMsgHandler();

		/**
		 * Abstract base for a simple handler class that we can use to delegate
		 * message handling to a variety of different destinations.
		 */
		class MessageHandler
		{
		public:
			virtual
			~MessageHandler()
			{ }
			
			virtual
			void
			handle_qt_message(
					QtMsgType msg_type,
					const char * msg) = 0;
		};

		/**
		 * Uses @a qInstallMsgHandler to @a install_qt_message_handler as the sole Qt message handler.
		 * NOTE: only installs handler if any of the following conditions are satisfied:
		 *   1) GPLATES_PUBLIC_RELEASE is defined (automatically handled by CMake build system), or
		 *   2) GPLATES_OVERRIDE_QT_MESSAGE_HANDLER environment variable is set to case-insensitive
		 *      "true", "1", "yes" or "on".
		 * If handler is not installed then default Qt handler applies.
		 * This handler is uninstalled when its singleton instance is destroyed
		 * at application exit (and the previous handler is reinstalled).
		 *
		 * @param log_filename - an optional override to the default LogToFileHandler's filename.
		 */
		static
		void
		install_qt_message_handler(
				const QString &log_filename = QString());


		/**
		 * The message handler function called by Qt.
		 */
		static
		void
		qt_message_handler(
				QtMsgType msg_type,
				const char *msg);

		/**
		 * Add one of our own MessageHandler derivatives to the list of handlers that
		 * can process messages.
		 */
		void
		add_handler(
				boost::shared_ptr<MessageHandler> handler);



	private:
		//
		// Static data members
		//

		//! Next Qt message handler in the chain of message handlers.
		static QtMsgHandler s_prev_msg_handler;
		
		//
		// Instance member data
		//

		/**
		 * Store all MessageHandler derivations registered with this class, so we can pass
		 * the messages to them all.
		 */
		std::vector<boost::shared_ptr<MessageHandler> > d_message_handlers;

		//
		// Instance methods
		//

		/**
		 * This delegates the message to our various MessageHandler derivations.
		 */
		void
		handle_qt_message(
				QtMsgType msg_type,
				const char *msg);

		/**
		 * Returns true if should install message handler.
		 */
		static
		bool
		should_install_message_handler();
	};
}

#endif // GPLATES_APP_LOGIC_GPLATESQTMSGHANDLER_H
