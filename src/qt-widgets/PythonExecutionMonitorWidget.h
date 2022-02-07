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
#include <QString>

#include "PythonExecutionMonitorWidgetUi.h"


namespace GPlatesApi
{
	class PythonExecutionThread;
}

namespace GPlatesQtWidgets
{
	/**
	 * PythonExecutionMonitorWidget is a widget that appears on screen to allow the
	 * user to stop Python execution. It also listens to Ctrl+C, which is an
	 * alternative way to stop Python execution.
	 */
	class PythonExecutionMonitorWidget :
			public QWidget,
			protected Ui_PythonExecutionMonitorWidget
	{
		Q_OBJECT

	public:

		/**
		 * Constructs a @a PythonExecutionMonitorWidget with a non-NULL @a parent_.
		 */
		PythonExecutionMonitorWidget(
				GPlatesApi::PythonExecutionThread *python_execution_thread,
				QWidget *parent_);

		virtual
		~PythonExecutionMonitorWidget();

	protected:

		virtual
		void
		timerEvent(
				QTimerEvent *ev);

		virtual
		void
		showEvent(
				QShowEvent *ev);

		virtual
		void
		hideEvent(
				QHideEvent *ev);

		virtual
		bool
		eventFilter(
				QObject *watched,
				QEvent *ev);

	private Q_SLOTS:

		void
		handle_cancel_button_clicked();

	private:

		void
		reposition();

		static const int APPEARANCE_TIME = 500 /* milliseconds */;

		GPlatesApi::PythonExecutionThread *d_python_execution_thread;
		QBasicTimer d_timer;
	};
}

#endif  // GPLATES_QTWIDGETS_PYTHONEXECUTIONMONITORWIDGET_H
