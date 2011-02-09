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
 
#ifndef GPLATES_GUI_MODALPYTHONEXECUTIONMONITOR_H
#define GPLATES_GUI_MODALPYTHONEXECUTIONMONITOR_H

#include <set>
#include <QWidget>

#include "api/PythonExecutionMonitor.h"


namespace GPlatesGui
{
	/**
	 * ModalPythonExecutionMonitor monitors the execution of the Python thread, and
	 * allows for the interruption of that thread by the user pressing Ctrl+C (or
	 * control+C on the Mac). All events other than those necessary for refreshing
	 * the user interface are discarded; the user cannot interact with GPlates
	 * while the Python execution thread is doing its job. The rationale for
	 * locking down the user interface is that the GPlates model is single-threaded
	 * and so we should not allow the user to interact with GPlates during Python
	 * execution, which may modify the model.
	 */
	class ModalPythonExecutionMonitor :
			public GPlatesApi::PythonExecutionMonitor
	{
		Q_OBJECT

	public:

		/**
		 * Constructs an object that monitors the execution of Python code on a thread
		 * with @a python_thread_id. Any warnings/errors are displayed using
		 * @a QMessageBox with parent @a message_box_parent if not NULL.
		 */
		explicit
		ModalPythonExecutionMonitor(
				long python_thread_id);

		/**
		 * Reimplementation of function in base class. In addition to starting the
		 * local event loop, we install ourselves as an application-wide event filter.
		 */
		virtual
		bool
		exec();

		/**
		 * Exempt @a widget from the event blackout. All events will be delivered to
		 * @a widget and its children as usual.
		 */
		void
		add_blackout_exemption(
				QWidget *widget);

		/**
		 * Removes @a widget from event blackout exemption. Only certain events will
		 * now be delivered to @a widget and its children.
		 */
		void
		remove_blackout_exemption(
				QWidget *widget);

	public slots:

		void
		interrupt_python_thread_if_not_yet_interrupted();

	signals:

		void
		python_thread_interrupted();

	protected:

		virtual
		bool
		eventFilter(
				QObject *obj,
				QEvent *ev);

	private:

		bool d_thread_interrupted;
		std::set<QWidget *> d_exempt_widgets;
	};
}

#endif	// GPLATES_GUI_MODALPYTHONEXECUTIONMONITOR_H
