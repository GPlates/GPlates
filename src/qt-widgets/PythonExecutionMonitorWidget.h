/* $Id$ */

/**
 * \file 
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
 
#ifndef GPLATES_QTWIDGETS_PYTHONEXECUTIONMONITORWIDGET_H
#define GPLATES_QTWIDGETS_PYTHONEXECUTIONMONITORWIDGET_H

#include <boost/optional.hpp>
#include <QWidget>
#include <QBasicTimer>
#include <QMessageBox>
#include <QString>

#include "PythonExecutionMonitorWidgetUi.h"


namespace GPlatesGui
{
	class ModalPythonExecutionMonitor;
}

namespace GPlatesQtWidgets
{
	/**
	 * PythonExecutionMonitorWidget is a widget that provides an alternative to
	 * pressing Ctrl+C to stop the Python execution thread.
	 */
	class PythonExecutionMonitorWidget :
			public QWidget,
			protected Ui_PythonExecutionMonitorWidget
	{
		Q_OBJECT

	public:

		/**
		 * Constructs a @a PythonExecutionMonitorWidget with a non-NULL @a parent_.
		 * The given @a monitor is used to interrupt the Python thread when requested.
		 */
		explicit
		PythonExecutionMonitorWidget(
				GPlatesGui::ModalPythonExecutionMonitor *monitor,
				QWidget *parent_);

		~PythonExecutionMonitorWidget();

	signals:

		void
		python_thread_interruption_requested();

	protected:

		virtual
		void
		timerEvent(
				QTimerEvent *ev);

		virtual
		bool
		eventFilter(
				QObject *watched,
				QEvent *ev);

	private slots:

		void
		handle_cancel_button_clicked();

		void
		handle_python_thread_interrupted();

		void
		handle_execution_finished();

		void
		handle_system_exit_exception_raised(
				int exit_status,
				const boost::optional<QString> &error_message);

	private:

		void
		reposition();

		static const int APPEARANCE_TIME = 500 /* milliseconds */;

		GPlatesGui::ModalPythonExecutionMonitor *d_monitor;
		QMessageBox *d_system_exit_messagebox;
		QBasicTimer d_timer;
	};
}

#endif  // GPLATES_QTWIDGETS_PYTHONEXECUTIONMONITORWIDGET_H
