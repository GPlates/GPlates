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
 
#include <QLayout>
#include <QPalette>

#include "PythonExecutionMonitorWidget.h"

#include "gui/ModalPythonExecutionMonitor.h"


GPlatesQtWidgets::PythonExecutionMonitorWidget::PythonExecutionMonitorWidget(
		GPlatesGui::ModalPythonExecutionMonitor *monitor,
		QWidget *parent_) :
	QWidget(parent_),
	d_monitor(monitor),
	d_system_exit_messagebox(
			new QMessageBox(
				QMessageBox::Critical,
				tr("Python Exception"),
				QString(),
				QMessageBox::Ok,
				this))
{
	setupUi(this);

	int spacing = layout()->spacing();
	layout()->setContentsMargins(spacing, 0, spacing, 0);

	QPalette this_palette = palette();
	this_palette.setColor(QPalette::Window, Qt::darkGray);
	this_palette.setColor(QPalette::WindowText, Qt::white);
	setPalette(this_palette);
	setFont(QFont());

	resize(sizeHint());
	reposition();

	QObject::connect(
			cancel_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_cancel_button_clicked()));

	QObject::connect(
			monitor,
			SIGNAL(python_thread_interrupted()),
			this,
			SLOT(handle_python_thread_interrupted()));
	QObject::connect(
			monitor,
			SIGNAL(exec_interactive_command_finished(bool)),
			this,
			SLOT(handle_execution_finished()));
	QObject::connect(
			monitor,
			SIGNAL(exec_finished()),
			this,
			SLOT(handle_execution_finished()));
	QObject::connect(
			monitor,
			SIGNAL(system_exit_exception_raised(int, const boost::optional<QString> &)),
			this,
			SLOT(handle_system_exit_exception_raised(int, const boost::optional<QString> &)));
	QObject::connect(
			this,
			SIGNAL(python_thread_interruption_requested()),
			monitor,
			SLOT(interrupt_python_thread_if_not_yet_interrupted()));

	d_timer.start(APPEARANCE_TIME, this);

	parent_->installEventFilter(this);
	monitor->add_blackout_exemption(this);
}


GPlatesQtWidgets::PythonExecutionMonitorWidget::~PythonExecutionMonitorWidget()
{
	parentWidget()->removeEventFilter(this);
}


void
GPlatesQtWidgets::PythonExecutionMonitorWidget::timerEvent(
		QTimerEvent *ev)
{
	d_timer.stop();
	show();
}


bool
GPlatesQtWidgets::PythonExecutionMonitorWidget::eventFilter(
		QObject *watched,
		QEvent *ev)
{
	if (watched == parentWidget() && ev->type() == QEvent::Resize)
	{
		reposition();
		return true;
	}
	else
	{
		return QWidget::eventFilter(watched, ev);
	}
}


void
GPlatesQtWidgets::PythonExecutionMonitorWidget::handle_cancel_button_clicked()
{
	emit python_thread_interruption_requested();
}


void
GPlatesQtWidgets::PythonExecutionMonitorWidget::handle_python_thread_interrupted()
{
	cancel_button->setEnabled(false);
}


void
GPlatesQtWidgets::PythonExecutionMonitorWidget::handle_execution_finished()
{
	d_timer.stop();
	hide();
}


void
GPlatesQtWidgets::PythonExecutionMonitorWidget::handle_system_exit_exception_raised(
		int exit_status,
		const boost::optional<QString> &error_message)
{
	// Only show a warning if the exit status is not 0. 0 usually means success
	// so let's not scare the user!
	if (exit_status != 0)
	{
		QString warning;
		if (error_message)
		{
			warning = tr("A Python script raised an unhandled SystemExit exception \"%1\" with exit status %2.")
				.arg(*error_message).arg(exit_status);
		}
		else
		{
			warning = tr("A Python script raised an unhandled SystemExit exception with exit status %1.")
				.arg(exit_status);
		}
		warning += tr("\nThis is a serious error that usually results in program termination. "
			"Please consider saving your work and restarting GPlates.");

		d_system_exit_messagebox->setText(warning);
		d_system_exit_messagebox->exec();
	}
}


void
GPlatesQtWidgets::PythonExecutionMonitorWidget::reposition()
{
	move(parentWidget()->width() - width(), parentWidget()->height() - height());
}

