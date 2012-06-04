/* $Id$ */

/**
 * \file Qt Model/View class for a list of log entries. Does not depend on any QtGui modules.
 *
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
 
#ifndef GPLATES_APP_LOGIC_LOGMODEL_H
#define GPLATES_APP_LOGIC_LOGMODEL_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QtGlobal>
#include <QPointer>
#include <QTimer>

#include "GPlatesQtMsgHandler.h"


namespace GPlatesAppLogic
{
	/**
	 * Qt Model/View class for a list of log entries.
	 * See http://doc.trolltech.com/4.5/model-view-model-subclassing.html for a primer.
	 */
	class LogModel :
			public QAbstractListModel,
			private boost::noncopyable
	{
		Q_OBJECT
	public:
		
		/**
		 * Inner class to hold details about each entry of the log.
		 * TODO: Maybe extract into its own file if it gets too big
		 *       - possibly moving this and LogModel into Utils.
		 */
		class LogEntry
		{
		public:
			enum Severity
			{
				DEBUG = QtDebugMsg,
				WARNING = QtWarningMsg,
				CRITICAL = QtCriticalMsg,
				FATAL = QtFatalMsg,
				
				OTHER = 100
			};
			enum Type
			{
				NORMAL,
				META
			};
		
			explicit
			LogEntry(
					const QString &_text,
					Severity _severity = OTHER,
					Type _type = NORMAL):
				d_text(_text),
				d_severity(_severity),
				d_type(_type)
			{  }
			
			virtual
			~LogEntry() {  }
			
			QString
			text() const
			{
				return d_text;
			}
			
			Severity
			severity() const
			{
				return d_severity;
			}
			
			Type
			type() const
			{
				return d_type;
			}
			
			bool
			operator==(
					const LogEntry &other) const
			{
				return (d_text == other.d_text
						&& d_severity == other.d_severity
						&& d_type == other.d_type);
			}
		
		private:
			QString d_text;
			Severity d_severity;
			Type d_type;
		};
		
		/**
		 * Roles for @a data() to use to supply info about the severity and type of log message
		 * to a higher-up Qt Model that can filter and colour accordingly.
		 */
		static const int SeverityRole;
		static const int TypeRole;
		
		explicit
		LogModel(
				QObject *_parent);
		
		virtual
		~LogModel();
		
		/**
		 * Our accessor for appending new log entries.
		 */
		void
		append(
				const LogEntry &entry);
		
		
		/**
		 * Qt Model/View accessor for data of a LogEntry for assorted roles.
		 */
		QVariant
		data(
				const QModelIndex &idx,
				int role = Qt::DisplayRole) const;
		
		/**
		 * Qt Model/View accessor for item flags of a LogEntry to see how it should behave.
		 */
		Qt::ItemFlags
		flags(
				const QModelIndex &idx) const;
		
		/**
		 * Qt Model/View accessor to see how many LogEntries we have.
		 */
		int
		rowCount(
				const QModelIndex &parent_idx) const;
		
	private slots:
		
		/**
		 * Called after a short period of no further incoming messages, to
		 * ensure that large floods of messages get processed as a batch
		 * rather than continuous small updates (that can create GUI resize
		 * events that slow everything down.
		 */
		void
		flush_buffer();
		
	private:
		/**
		 * The backend to the model, the log of actual messages.
		 */
		QList<LogEntry> d_log;

		/**
		 * Temporary holding area for inbound messages to protect against flooding.
		 */
		QList<LogEntry> d_buffer;

		/**
		 * Timer used to prevent flooding.
		 */
		QPointer<QTimer> d_buffer_timeout;

		/**
		 * Handle to the installed message handler so can remove it in destructor.
		 */
		boost::optional<GPlatesQtMsgHandler::message_handler_id_type> d_message_handler_id;
	};
			
}


#endif // GPLATES_APP_LOGIC_LOGMODEL_H
