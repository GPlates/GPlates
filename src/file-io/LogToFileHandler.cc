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

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>
#include <QtGlobal>

#include "LogToFileHandler.h"

#include "file-io/ErrorOpeningFileForWritingException.h"

#include "global/Version.h"

#include "utils/Environment.h"


const QString GPlatesFileIO::LogToFileHandler::DEFAULT_LOG_FILENAME = "GPlates_log.txt";


namespace
{
	/**
	 * Specify logic for choosing the default loglevel here.
	 */
	int
	adjust_default_log_level(
			int log_level = QtDebugMsg)
	{
		// User can tweak log level via an environment variable, GPLATES_LOGLEVEL.
		QString log_level_qstr = GPlatesUtils::getenv("GPLATES_LOGLEVEL").toLower();
		if (log_level_qstr == "debug")	// All messages, including Debug messages, will be output.
		{
			log_level = QtDebugMsg;
		}
		else if (log_level_qstr == "warning")	// Warnings and up (Critical, Fatal) will be output.
		{
			log_level = QtWarningMsg;
		}
		else if (log_level_qstr == "critical")	// Only Critical and Fatal messages will be output.
		{
			log_level = QtCriticalMsg;
		}
		// else: keep log_level at the same level supplied to this fn, or the default of "Everything (QtDebugMsg and up)"
		return log_level;
	}
}



GPlatesFileIO::LogToFileHandler::LogToFileHandler(
		const QString &log_filename) :
	d_log_file(log_filename),
	// For File logs, default to "Warning or above", no Debug messages.
	// Rationale: File-IO for lots of debug messages may suck.
	//
	// UPDATE: We now log debug messages so when users send us their log files they
	// contain more info - at least for internal (non-public) builds...
	d_log_level(adjust_default_log_level(QtDebugMsg))
{
	// Open the QFile QIODevice that we'll use to write to a file on disk.
	if (d_log_file.fileName().isEmpty())
	{
		d_log_file.setFileName(DEFAULT_LOG_FILENAME);
	}
	if ( ! d_log_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate) )
	{
		//
		// We couldn't open the log file for writing. This can happen on Windows when GPlates has
		// been installed to "C:\Program Files" and hence cannot write to that location.
		// Instead we'll attempt to write to the local writable app data location.
		// For example:
		//
		// Windows - "C:/Users/<USER>/AppData/Local/GPlates/GPlates/"
		// macOS   - "~/Library/Application Support/GPlates/GPlates/"
		// Linux   - "~/.local/share/GPlates/GPlates/".
		//
		const QDir app_data_dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
		const QString log_basename = QFileInfo(d_log_file.fileName()).fileName();
		const QString app_data_log_filename = app_data_dir.absolutePath() + "/" + log_basename;

		d_log_file.setFileName(app_data_log_filename);
		if ( ! d_log_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate) )
		{
			throw GPlatesFileIO::ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
					QFileInfo(d_log_file).absoluteFilePath());
		}
	}

	// Wrap that up in a QTextStream to make writing to it nicer.
	d_log_stream.reset(new QTextStream(&d_log_file));

	// Print message with timestamp and version so we know what made this log.
	*d_log_stream << "Log file created on " << QDateTime::currentDateTime().toString() << " by GPlates "
			<< GPlatesGlobal::Version::get_GPlates_version()
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			<< Qt::endl;
#else
			<< endl;
#endif
}


GPlatesFileIO::LogToFileHandler::LogToFileHandler(
		FILE *output_file_ptr):
	d_log_file(),
	// For IO-stream logs, default to "Everything", since this output will be going to the terminal.
	d_log_level(adjust_default_log_level())
{
	// We don't need to open an actual QFile in this case, just create the QTextStream wrapper around
	// the FILE pointer.
	d_log_stream.reset(new QTextStream(output_file_ptr, QIODevice::WriteOnly | QIODevice::Text));	
	
	// Logging to stderr doesn't really need to care about what GPLATES_LOGLEVEL is set; that's really just for the log file.

	*d_log_stream << "Logging to console started at " << QDateTime::currentDateTime().toString() << " by GPlates "
			<< GPlatesGlobal::Version::get_GPlates_version()
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			<< Qt::endl;
#else
			<< endl;
#endif
}



GPlatesFileIO::LogToFileHandler::~LogToFileHandler()
{  }


void
GPlatesFileIO::LogToFileHandler::handle_qt_message(
		QtMsgType msg_type,
		const QString &msg)
{
	// Only output log messages of the configured severity and up.
	// The default, QtWarningMsg, will exclude Debug messages but let everything else through.
	if (msg_type < d_log_level)
			return;

	switch (msg_type)
	{
	case QtDebugMsg:
		*d_log_stream << "[Debug] " << msg
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			<< Qt::endl;
#else
			<< endl;
#endif
		break;

	case QtWarningMsg:
		*d_log_stream << "[Warning] " << msg
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			<< Qt::endl;
#else
			<< endl;
#endif
		break;

	case QtCriticalMsg:
		// Note: system and critical messages have the same enumeration value.
		*d_log_stream << "[Critical] " << msg
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			<< Qt::endl;
#else
			<< endl;
#endif
		break;

	case QtFatalMsg:
		*d_log_stream << "[Fatal] " << msg
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			<< Qt::endl;
#else
			<< endl;
#endif
		break;

	default:
		break;
	}
}

