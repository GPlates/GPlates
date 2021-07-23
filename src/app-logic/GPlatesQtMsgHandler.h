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

#include <list>
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

		//! Typedef for a message handler identifier (so it can be removed after adding).
		typedef unsigned int message_handler_id_type;

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
					const QMessageLogContext &context,
					const QString &msg) = 0;
		};

		/**
		 * Uses @a qInstallMessageHandler to install the GPlates Qt message handler.
		 *
		 * NOTE: Does not install handler if GPLATES_OVERRIDE_QT_MESSAGE_HANDLER environment variable
		 *       is set to case-insensitive "0", "false", "off", "disabled", or "no".
		 *
		 * If handler is successfully installed then it handles message first followed by the
		 * previously installed Qt message handler (which we call directly, Qt doesn't call it).
		 * And when this singleton instance is destroyed at application exit the handler is uninstalled
		 * and the previously installed Qt message handler is reinstalled.
		 *
		 * If handler is not installed then only the previously installed Qt message handler is used.
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
				const QMessageLogContext &context,
				const QString &msg);

		/**
		 * Add one of our own MessageHandler derivatives to the list of handlers that
		 * can process messages.
		 */
		message_handler_id_type
		add_handler(
				boost::shared_ptr<MessageHandler> handler);

		/**
		 * Remove a message handler added with @a add_handler.
		 */
		void
		remove_handler(
				message_handler_id_type handler_id);


	private:
		//
		// Static data members
		//

		//! Next Qt message handler in the chain of message handlers.
		static QtMessageHandler s_prev_msg_handler;
		
		//
		// Instance member data
		//

		typedef std::list<boost::shared_ptr<MessageHandler> > message_handle_list_type;

		/**
		 * Store all MessageHandler derivations registered with this class, so we can pass
		 * the messages to them all.
		 */
		message_handle_list_type d_message_handler_list;

		/**
		 * Index by @a message_handler_id_type to find the message handler in @a d_message_handler_list.
		 */
		std::vector<message_handle_list_type::iterator> d_message_handler_iterators;

		//
		// Instance methods
		//

		/**
		 * This delegates the message to our various MessageHandler derivations.
		 */
		void
		handle_qt_message(
				QtMsgType msg_type,
				const QMessageLogContext &context,
				const QString &msg);

		/**
		 * Returns true if should install message handler.
		 */
		static
		bool
		should_install_message_handler();
	};
}

#endif // GPLATES_APP_LOGIC_GPLATESQTMSGHANDLER_H
