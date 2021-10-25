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

#include <QDateTime>
#include <boost/shared_ptr.hpp>

#include "LogToModelHandler.h"
#include "global/SubversionInfo.h"

#include "LogModel.h"


const int GPlatesAppLogic::LogModel::SeverityRole = Qt::UserRole + 1;
const int GPlatesAppLogic::LogModel::TypeRole = Qt::UserRole + 2;


namespace
{
	/**
	 * Replace heavily duplicated messages with a single message that indicates
	 * how many times the repeat occurred.
	 */
	QList<GPlatesAppLogic::LogModel::LogEntry>
	compress_buffer(
			QList<GPlatesAppLogic::LogModel::LogEntry> &buffer)
	{
		using namespace GPlatesAppLogic;
		
		if (buffer.size() < 2) {
			return buffer;
		}
		QList<LogModel::LogEntry> compressed;

		// Process the first entry specially.
		LogModel::LogEntry *last_entry_ptr = &buffer[0];
		compressed.append(buffer.at(0));
		
		// From then on, look for duplicates.
		int dup_count = 0;
		for (int i = 1; i < buffer.size(); ++i) {
			if (buffer.at(i) == *last_entry_ptr) {
				// Aha, a duplicate. Remember it, but don't push any messages to
				// the "compressed" list just yet.
				dup_count++;
				
			} else {
				if (dup_count == 0) {
					// Not a duplicate of the previous message,
					// and there's no running count of duplicates.
					// Add normally.
					compressed.append(buffer.at(i));
					
				} else if (dup_count <= 1) {
					// Not a duplicate of the previous message,
					// but there's been one duplicate up until now.
					// Not really enough to justify a placeholder message,
					// so just add the duped message and the non-dup trigger.
					compressed.append(*last_entry_ptr);
					compressed.append(buffer.at(i));
					dup_count = 0;

				} else {
					// Not a duplicate of the previous message,
					// but there's been lots of duplicates up until now.
					// Add a placeholder message instead of the duplicates,
					// and add the non-dup message that triggered this.
					LogModel::LogEntry placeholder(
							QObject::tr("Last message repeated %1 times").arg(dup_count),
							last_entry_ptr->severity(),
							LogModel::LogEntry::META);
					compressed.append(placeholder);
					compressed.append(buffer.at(i));
					dup_count = 0;
					
				}
			}
			// Remember last message we saw.
			last_entry_ptr = &buffer[i];
		}
		// Process any remaining duplicates that were at the end of the buffer.
		if (dup_count == 0) {
			// No trailing duplicate messages.
		} else if (dup_count <= 1) {
			compressed.append(*last_entry_ptr);
		} else {
			LogModel::LogEntry placeholder(
					QObject::tr("Last message repeated %1 times").arg(dup_count),
					last_entry_ptr->severity(),
					LogModel::LogEntry::META);
			compressed.append(placeholder);
		}
		
		return compressed;
	}
}



GPlatesAppLogic::LogModel::LogModel(
		QObject *_parent):
	QAbstractListModel(_parent),
	d_buffer_timeout(new QTimer(this))
{
	// A timeout is used to hold messages briefly before actually appending them.
	connect(d_buffer_timeout, SIGNAL(timeout()), this, SLOT(flush_buffer()));
	d_buffer_timeout->setSingleShot(true);
	
	// Start the log with the date and our version.
	QString logmsg = QObject::tr("Log started at %1 by GPlates %2 %3")
			.arg(QDateTime::currentDateTime().toString())
			.arg(GPlatesGlobal::SubversionInfo::get_working_copy_branch_name())
			.arg(GPlatesGlobal::SubversionInfo::get_working_copy_version_number());
	append(LogEntry(logmsg, LogEntry::OTHER, LogEntry::META));
			
	// As we get created by ApplicationState, we should now be ready to install
	// our LogToModelHandler to the GPlatesQtMsgHandler.
	boost::shared_ptr<GPlatesQtMsgHandler::MessageHandler> handler(new LogToModelHandler(*this));
	d_message_handler_id = GPlatesQtMsgHandler::instance().add_handler(handler);
}


GPlatesAppLogic::LogModel::~LogModel()
{
	if (d_message_handler_id)
	{
		GPlatesQtMsgHandler::instance().remove_handler(d_message_handler_id.get());
	}
}


void
GPlatesAppLogic::LogModel::append(
		const GPlatesAppLogic::LogModel::LogEntry &entry)
{
	// When messages first come in, they have to wait it out in a buffer until things calm down,
	d_buffer.append(entry);
	
	if (d_buffer.size() < 50) {
		// Presuming a sane amount of messages come in, just wait until they stop and then process them.
		d_buffer_timeout->start(100);
	} else {
		// OR, if the buffer is pretty full already and there's no sign of them stopping, don't
		// continually reset the timer; this will mean we start processing chunks of messages
		// EVERY 100ms or so until the flood stops.
	}
}


QVariant
GPlatesAppLogic::LogModel::data(
		const QModelIndex &idx,
		int role) const
{
	if ( ! idx.isValid()) {
		// An invalid index - we cannot report data for this.
		return QVariant();
	} else if (idx.row() < 0 || idx.row() >= d_log.size()) {
		// The index is valid, but refers to an out-of-bounds row - we cannot report data for this.
		return QVariant();
	}
	
	switch (role)
	{
	case Qt::DisplayRole:
			return d_log.at(idx.row()).text();

	case LogModel::SeverityRole:
			return d_log.at(idx.row()).severity();

	case LogModel::TypeRole:
			return d_log.at(idx.row()).type();
	
	default:
			return QVariant();
	}
}


Qt::ItemFlags
GPlatesAppLogic::LogModel::flags(
		const QModelIndex &idx) const
{
	if ( ! idx.isValid()) {
		// An invalid index - we cannot report data for this.
		return Qt::NoItemFlags;
	}
	
	// TODO: Here we might vary the flags a little depending on the type of LogEntry.
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


int
GPlatesAppLogic::LogModel::rowCount(
		const QModelIndex &parent_idx) const
{
	return d_log.size();
}


void
GPlatesAppLogic::LogModel::flush_buffer()
{
	if (d_buffer.size() <= 0) {
		// Shouldn't happen.
		return;
	}
	
	// Squish repeated messages down a bit.
	QList<LogEntry> compressed = compress_buffer(d_buffer);
	d_buffer.clear();
	
	beginInsertRows(QModelIndex(), d_log.size(), d_log.size() + compressed.size() - 1);
	d_log += compressed;
	endInsertRows();
}


