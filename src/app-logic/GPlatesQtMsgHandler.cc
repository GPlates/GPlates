/* $Id$ */

/**
 * \file 
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

#include <QtGlobal>  // For Q_OS_WIN, etc.

// C headers for low-level functions (setvbuf, pipe, fileno, dup2, read).
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#if defined(Q_OS_WIN)
#	include <io.h>
#else
#	include <unistd.h>
#endif

#include <memory>
#include <string>
#include <iostream>
#include <ostream>
#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <Qt>

#include "GPlatesQtMsgHandler.h"

#include "file-io/LogToFileHandler.h"
#include "file-io/ErrorOpeningFileForWritingException.h"

#include "global/AssertionFailureException.h"
#include "global/config.h"  // GPLATES_PUBLIC_RELEASE
#include "global/GPlatesAssert.h"
#include "global/Version.h"

#include "utils/Environment.h"


namespace GPlatesAppLogic
{
	/**
	 * Class to capture stdout or stderr (each stream run in a separate QThread) and send captured
	 * output back to GPlatesQtMsgHandler so it can pass onto to any registered message handlers.
	 */
	class StdOutErrCapture :
			public QObject
	{
		Q_OBJECT

	public:

		StdOutErrCapture() :
			d_is_capturing(false)
		{  }

		~StdOutErrCapture()
		{
			if (d_is_capturing)
			{
				stop_capturing();
			}
		}

		bool
		start_capturing(
				FILE *stream)
		{
			if (d_is_capturing)
			{
				return true;
			}

			// Flush any output buffered in stream before we redirect it for capture.
			fflush(stream);

			// Use *line* buffering in FILE stream.
			if (0 != setvbuf(stream, NULL, _IOLBF, 4096))
			{
				return false;
			}

			// Create a pipe so we can duplicate the output stream onto its write end.
			// Use text mode to convert \r\n to \n on Windows.
			if (0 !=
#if defined(Q_OS_WIN)
                _pipe(d_pipe_read_write_descriptors, 16*1024, O_TEXT)
#else
                pipe(d_pipe_read_write_descriptors)
#endif
                )
			{
				return false;
			}

			d_stream_file_descriptor = fileno(stream);
			if (d_stream_file_descriptor < 0)
			{
				// This includes the case of a Windows application without a console window
				// (which returns -2 to indicate stdout/stderr not associated without a stream).
				close(d_pipe_read_write_descriptors[0]);
				close(d_pipe_read_write_descriptors[1]);
				return false;
			}

			// Keep a copy so we can restore later.
			d_original_stream_file_descriptor = dup(d_stream_file_descriptor);
			if (d_original_stream_file_descriptor < 0)
			{
				close(d_pipe_read_write_descriptors[0]);
				close(d_pipe_read_write_descriptors[1]);
				return false;
			}

			// Make the stdout/stderr output stream refer to the write end of the pipe.
			//
			// Note: Unix platforms (macOS/Linux) return the second file descriptor on success.
			//       However the Windows version of 'dup' (renamed to '_dup') returns zero on success.
			//       So we cannot compare with zero for success (instead checking for non-negative).
			if (dup2(d_pipe_read_write_descriptors[1], d_stream_file_descriptor) < 0)
			{
				close(d_pipe_read_write_descriptors[0]);
				close(d_pipe_read_write_descriptors[1]);
				// Restore original stream.
				dup2(d_original_stream_file_descriptor, d_stream_file_descriptor);
				close(d_original_stream_file_descriptor);
				return false;
			}

			d_is_capturing = true;

			return true;
		}

	public Q_SLOTS:

		void
		capture_messages()
		{
			// Read from the read end of the pipe.
			const int bytes_read = read(d_pipe_read_write_descriptors[0], d_pipe_buffer, sizeof(d_pipe_buffer) - 1);
			if (bytes_read < 0 || bytes_read >= boost::numeric_cast<int>(sizeof(d_pipe_buffer)))
			{
				// Emit error and return early to event loop.
				// The calling thread will then ask us to stop capturing
				// (and then will quit the thread that this object is in).
				Q_EMIT error_reading();
				return;
			}

			// Write a null terminator.
			d_pipe_buffer[bytes_read] = 0;

			// Split read buffer at newlines (into multiple strings).
			QStringList messages = QString(d_pipe_buffer).split("\n",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
				Qt::KeepEmptyParts
#else
				QString::KeepEmptyParts
#endif
			);

			// If last message is empty then remove it because it means there's no string after the last newline.
			if (messages.size() > 1 &&
				messages.last().isEmpty())
			{
				messages.removeLast();
			}

			Q_EMIT output_messages(messages);
		}

		void
		stop_capturing()
		{
			close(d_pipe_read_write_descriptors[0]);
			close(d_pipe_read_write_descriptors[1]);
			// Restore original stream.
			dup2(d_original_stream_file_descriptor, d_stream_file_descriptor);
			close(d_original_stream_file_descriptor);

			d_is_capturing = false;
		}

	Q_SIGNALS:

		void
		error_reading();

		void
		output_messages(
				QStringList);

	private:

		bool d_is_capturing;
		int d_stream_file_descriptor;
		int d_original_stream_file_descriptor;
		int d_pipe_read_write_descriptors[2];
		char d_pipe_buffer[16*1024];
	};
}
// CMake documentation on Qt5 AUTOMOC states that if Q_OBJECT is found in an implementation file (ie, not a header)
// then the user must include "<basename>.moc" (at least for CMake > 3.7). This is not needed for header files.
//
// Note: We include this after declaring our QObject-derived class StdOutErrCapture above otherwise
//       it gives a compile error that GPlatesAppLogic::StdOutErrCapture is undeclared.
#include "GPlatesQtMsgHandler.moc"


QtMessageHandler GPlatesAppLogic::GPlatesQtMsgHandler::s_prev_msg_handler = NULL;


GPlatesAppLogic::GPlatesQtMsgHandler::GPlatesQtMsgHandler()
{
	// Determine if we should even install the message handler.
	if (!should_install_message_handler())
	{
		return;
	}
	
	// Print the last message to the console before it gets redirected to log window and log file.
#if defined(GPLATES_PUBLIC_RELEASE)  // Flag defined by CMake build system (in "global/config.h").
	const QString console_message = QObject::tr("GPlates %1 started at %2")
			.arg(GPlatesGlobal::Version::get_GPlates_version())
			.arg(QDateTime::currentDateTime().toString());
#else
	const QString console_message = QObject::tr("GPlates %1 (build:%3 %4) started at %2")
			.arg(GPlatesGlobal::Version::get_GPlates_version())
			.arg(QDateTime::currentDateTime().toString())
			.arg(GPlatesGlobal::Version::get_working_copy_version_number())
			.arg(GPlatesGlobal::Version::get_working_copy_branch_name());
#endif
	qDebug() << console_message;

	// Install our message handler and keep track of the previous message handler.
	s_prev_msg_handler = qInstallMessageHandler(qt_message_handler);

	// Capture low-level stdout and stderr (eg, from our dependency libraries)
	// and log those messages too.
	start_capturing_stdout_and_stderr();
}


GPlatesAppLogic::GPlatesQtMsgHandler::~GPlatesQtMsgHandler()
{
	stop_capturing_stdout_and_stderr();

	// Reinstall the previous message handler.
	qInstallMessageHandler(s_prev_msg_handler);
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::add_log_file_handler(
		const QString &log_filename)
{
	try
	{
		// Set up a LogToFile handler for our log file.
		add_handler(boost::shared_ptr<MessageHandler>(
				new GPlatesFileIO::LogToFileHandler(log_filename)));
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &e)
	{
		// We couldn't open a log file for writing (not even in the local writable app data location).
		// Emit a warning rather than aborting so that other clients can still add handlers
		// (via 'add_handler()' such as the LogModel) and have them function.
		qWarning() << "Failed to install message handler because" << e.filename() << "cannot be opened for writing.";
	}
}


GPlatesAppLogic::GPlatesQtMsgHandler::message_handler_id_type
GPlatesAppLogic::GPlatesQtMsgHandler::add_handler(
		boost::shared_ptr<GPlatesAppLogic::GPlatesQtMsgHandler::MessageHandler> handler)
{
	// Add the message handler to the list.
	d_message_handler_list.push_back(handler);

	// Get iterator to list element just added.
	message_handle_list_type::iterator handler_iter = d_message_handler_list.end();
	--handler_iter;

	// Reference to message handler just added.
	const message_handler_id_type handler_id = d_message_handler_iterators.size();
	d_message_handler_iterators.push_back(handler_iter);

	return handler_id;
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::remove_handler(
		message_handler_id_type handler_id)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			handler_id < d_message_handler_iterators.size(),
			GPLATES_ASSERTION_SOURCE);

	message_handle_list_type::iterator handler_iter = d_message_handler_iterators[handler_id];
	d_message_handler_list.erase(handler_iter);
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::qt_message_handler(
		QtMsgType msg_type,
		const QMessageLogContext& context,
		const QString &msg)
{
	// Delegate message handling to our MessageHandlers.
	instance().handle_qt_message(msg_type, msg);
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::handle_qt_message(
		QtMsgType msg_type,
		const QString &msg)
{
	BOOST_FOREACH(boost::shared_ptr<MessageHandler> handler, d_message_handler_list)
	{
		if (handler)
		{
			handler->handle_qt_message(msg_type, msg);
		}
	}
}


bool
GPlatesAppLogic::GPlatesQtMsgHandler::should_install_message_handler()
{
	/*
	 * Overrides default Qt message handler unless the GPLATES_OVERRIDE_QT_MESSAGE_HANDLER
	 * environment variable is set to 'no' - in case developers want to use the built-in
	 * Qt message handler only. The message handler determines what happens when qDebug(),
	 * qWarning(), qCritical() and qFatal() are called.
	 */

	// We should override Qt's message handler by default,
	// unless GPLATES_OVERRIDE_QT_MESSAGE_HANDLER is defined and false. ("false", "0", "no" etc)
	bool default_should_install = true;

	return GPlatesUtils::getenv_as_bool("GPLATES_OVERRIDE_QT_MESSAGE_HANDLER", default_should_install);
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::start_capturing_stdout_and_stderr()
{
	// Our capture objects that run in separate threads (each thread blocks on reading stdout or stderr).
	std::unique_ptr<StdOutErrCapture> stdout_capture(new StdOutErrCapture());
	std::unique_ptr<StdOutErrCapture> stderr_capture(new StdOutErrCapture());

	if (!stdout_capture->start_capturing(stdout))
	{
		// Failed to redirect stdout so return without starting thread to capture stdout/stderr.
		// Use sterr (instead of qWarning()) since we've not added any Qt message handlers yet (eg, log window/file).
		std::cerr << "Unable to redirect stdout/stderr from console to log window/file." << std::endl;
		return;
	}
	if (!stderr_capture->start_capturing(stderr))
	{
		// Failed to redirect stderr so return without starting thread to capture stdout/stderr.
		// Use sterr (instead of qWarning()) since we've not added any Qt message handlers yet (eg, log window/file).
		std::cerr << "Unable to redirect stdout/stderr from console to log window/file." << std::endl;
		// Also stop capturing stdout (which successfully started capturing for some reason).
		// This leaves both stdout and stderr going to console.
		stdout_capture->stop_capturing();
		return;
	}

	// Move to respective threads.
	stdout_capture->moveToThread(&d_stdout_capture_thread);
	stderr_capture->moveToThread(&d_stderr_capture_thread);

	// Kickstart the whole process as soon as the thread starts.
	QObject::connect(&d_stdout_capture_thread, SIGNAL(started()), stdout_capture.get(), SLOT(capture_messages()));
	QObject::connect(&d_stderr_capture_thread, SIGNAL(started()), stderr_capture.get(), SLOT(capture_messages()));

	// Ensure our capture objects get deleted when threads finish.
	QObject::connect(&d_stdout_capture_thread, SIGNAL(finished()), stdout_capture.get(), SLOT(deleteLater()));
	QObject::connect(&d_stderr_capture_thread, SIGNAL(finished()), stderr_capture.get(), SLOT(deleteLater()));

	// Handle read errors from the threads.
	QObject::connect(stdout_capture.get(), SIGNAL(error_reading()), this, SLOT(handle_stdout_error()));
	QObject::connect(stderr_capture.get(), SIGNAL(error_reading()), this, SLOT(handle_stderr_error()));

	// Restore original streams after a read error.
	QObject::connect(this, SIGNAL(stop_capturing_stdout()), stdout_capture.get(), SLOT(stop_capturing()));
	QObject::connect(this, SIGNAL(stop_capturing_stderr()), stderr_capture.get(), SLOT(stop_capturing()));

	// Capture stdout/stderr messages from the threads.
	QObject::connect(stdout_capture.get(), SIGNAL(output_messages(QStringList)), this, SLOT(handle_stdout_messages(QStringList)));
	QObject::connect(stderr_capture.get(), SIGNAL(output_messages(QStringList)), this, SLOT(handle_stderr_messages(QStringList)));

	// Ask thread to process next stdout/stderr message.
	QObject::connect(this, SIGNAL(capture_stdout_messages()), stdout_capture.get(), SLOT(capture_messages()));
	QObject::connect(this, SIGNAL(capture_stderr_messages()), stderr_capture.get(), SLOT(capture_messages()));

	// When thread finishes it will delete these.
	stdout_capture.release();
	stderr_capture.release();

	// These threads will block on reading the pipe attached to stdout/stderr and so should just be sleeping
	// most of the time. So they probably don't need priorities, but we'll make them low priority anyway.
	d_stdout_capture_thread.start(QThread::LowestPriority);
	d_stderr_capture_thread.start(QThread::LowestPriority);
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::stop_capturing_stdout_and_stderr()
{
	//
	// For each stdout and stderr thread, send quit event so thread returns from its event loop.
	// However each thread is blocking in a read, so to unblock it we'll explicitly write something
	// to stdout/stderr so it returns from the read and can then exit its event loop.
	// If something goes wrong and we wait more than 1 second then just terminate the thread.
	//

	if (d_stdout_capture_thread.isRunning())
	{
		d_stdout_capture_thread.quit();
		std::cout << std::endl << std::flush;
		if (!d_stdout_capture_thread.wait(1000))
		{
			d_stdout_capture_thread.terminate();
			d_stdout_capture_thread.wait();
		}
	}

	if (d_stderr_capture_thread.isRunning())
	{
		d_stderr_capture_thread.quit();
		std::cerr << std::endl << std::flush;
		if (!d_stderr_capture_thread.wait(1000))
		{
			d_stderr_capture_thread.terminate();
			d_stderr_capture_thread.wait();
		}
	}
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::handle_stdout_messages(
		QStringList messages)
{
	for (int i = 0; i < messages.size(); ++i)
	{
		// Pass the message to our handlers.
		// This is essentially the same as "qDebug() << messages[i]" but more direct
		// (ie, without spaces inserted, etc).
		handle_qt_message(QtDebugMsg, messages[i]);
	}

	// Capture more messages.
	Q_EMIT capture_stdout_messages();
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::handle_stderr_messages(
		QStringList messages)
{
	for (int i = 0; i < messages.size(); ++i)
	{
		// Pass the message to our handlers.
		// This is essentially the same as "qWarning() << messages[i]" but more direct
		// (ie, without spaces inserted, etc).
		handle_qt_message(QtWarningMsg, messages[i]);
	}

	// Capture more messages.
	Q_EMIT capture_stderr_messages();
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::handle_stdout_error()
{
	Q_EMIT stop_capturing_stdout();

	// Quit the stdout thread (so thread returns from its event loop).
	// The thread is not blocking in a read so it should finish quickly.
	// If something goes wrong and we wait more than 1 second then just terminate the thread.
	d_stdout_capture_thread.quit();
	if (!d_stdout_capture_thread.wait(1000))
	{
		d_stdout_capture_thread.terminate();
		d_stdout_capture_thread.wait();
	}
}


void
GPlatesAppLogic::GPlatesQtMsgHandler::handle_stderr_error()
{
	Q_EMIT stop_capturing_stderr();

	// Quit the stderr thread (so thread returns from its event loop).
	// The thread is not blocking in a read so it should finish quickly.
	// If something goes wrong and we wait more than 1 second then just terminate the thread.
	d_stderr_capture_thread.quit();
	if (!d_stderr_capture_thread.wait(1000))
	{
		d_stderr_capture_thread.terminate();
		d_stderr_capture_thread.wait();
	}
}
