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

#include <QString>
#include <QFileInfo>
#include <QDateTime>

#include "LogToFileHandler.h"

#include "file-io/ErrorOpeningFileForWritingException.h"
#include "utils/Environment.h"
#include "global/SubversionInfo.h"


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
		// Compiled-in default will always exclude "Debug" messages if we are on a release build.
#if defined(GPLATES_PUBLIC_RELEASE)  // Flag defined by CMake build system.
		if (log_level < QtWarningMsg) {
			log_level = QtWarningMsg;
		}
		// Note this can still be overriden if a release build sees the GPLATES_LOGLEVEL env var below.
#endif
		
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
		const QString &log_filename):
	d_log_file(log_filename),
	// For File logs, default to "Warning or above", no Debug messages.
	// Rationale: File-IO for lots of debug messages may suck.
	d_log_level(adjust_default_log_level(QtWarningMsg))
{
	// Open the QFile QIODevice that we'll use to write to a file on disk.
	if (log_filename.isEmpty()) {
		d_log_file.setFileName(DEFAULT_LOG_FILENAME);
	}
	if ( ! d_log_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate) )
	{
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
				QFileInfo(d_log_file).absoluteFilePath());
	}

	// Wrap that up in a QTextStream to make writing to it nicer.
	d_log_stream.reset(new QTextStream(&d_log_file));

	// Print message with timestamp and version so we know what made this log.
	*d_log_stream << "Log file created on " << QDateTime::currentDateTime().toString() << " by GPlates " 
			<< GPlatesGlobal::SubversionInfo::get_working_copy_branch_name() << " "
			<< GPlatesGlobal::SubversionInfo::get_working_copy_version_number() << endl;
}


GPlatesFileIO::LogToFileHandler::LogToFileHandler(
		FILE *output_file_ptr):
	d_log_file(),
	// For IO-stream logs, default to "Everything", since this output will be going to the terminal.
	// Rationale: File-IO for lots of debug messages may suck.
	d_log_level(adjust_default_log_level())
{
	// We don't need to open an actual QFile in this case, just create the QTextStream wrapper around
	// the FILE pointer.
	d_log_stream.reset(new QTextStream(output_file_ptr, QIODevice::WriteOnly | QIODevice::Text));	
	
	// Logging to stderr doesn't really need to care about what GPLATES_LOGLEVEL is set; that's really just for the log file.

	*d_log_stream << "Logging to console started at " << QDateTime::currentDateTime().toString() << " by GPlates " 
			<< GPlatesGlobal::SubversionInfo::get_working_copy_branch_name() << " "
			<< GPlatesGlobal::SubversionInfo::get_working_copy_version_number() << endl;
}



GPlatesFileIO::LogToFileHandler::~LogToFileHandler()
{  }


void
GPlatesFileIO::LogToFileHandler::handle_qt_message(
		QtMsgType msg_type,
		const char *msg)
{
	// Only output log messages of the configured severity and up.
	// The default, QtWarningMsg, will exclude Debug messages but let everything else through.
	if (msg_type < d_log_level)
			return;

	switch (msg_type)
	{
	case QtDebugMsg:
		*d_log_stream << "[Debug] " << msg << endl;
		break;

	case QtWarningMsg:
		*d_log_stream << "[Warning] " << msg << endl;
		break;

	case QtCriticalMsg:
		// Note: system and critical messages have the same enumeration value.
		*d_log_stream << "[Critical] " << msg << endl;
		break;

	case QtFatalMsg:
		*d_log_stream << "[Fatal] " << msg << endl;
		break;

	default:
		break;
	}
}

