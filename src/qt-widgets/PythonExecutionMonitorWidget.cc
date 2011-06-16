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
#include <QApplication>

#include "PythonExecutionMonitorWidget.h"
#include "QtWidgetUtils.h"

#include "api/PythonExecutionThread.h"

#if !defined(GPLATES_NO_PYTHON)
GPlatesQtWidgets::PythonExecutionMonitorWidget::PythonExecutionMonitorWidget(
		GPlatesApi::PythonExecutionThread *python_execution_thread,
		QWidget *parent_) :
	QWidget(parent_),
	d_python_execution_thread(python_execution_thread)
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

	d_timer.start(APPEARANCE_TIME, this);
	qApp->installEventFilter(this);
}


GPlatesQtWidgets::PythonExecutionMonitorWidget::~PythonExecutionMonitorWidget()
{
	qApp->removeEventFilter(this);
}


void
GPlatesQtWidgets::PythonExecutionMonitorWidget::showEvent(
		QShowEvent *ev)
{
	parentWidget()->installEventFilter(this);
}


void
GPlatesQtWidgets::PythonExecutionMonitorWidget::hideEvent(
		QHideEvent *ev)
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
	else if (ev->type() == QEvent::KeyPress &&
			QtWidgetUtils::is_control_c(
				static_cast<QKeyEvent *>(ev)))
	{
		d_python_execution_thread->raise_keyboard_interrupt_exception();
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
	d_python_execution_thread->raise_keyboard_interrupt_exception();
}


void
GPlatesQtWidgets::PythonExecutionMonitorWidget::reposition()
{
	move(parentWidget()->width() - width(), parentWidget()->height() - height());
}
#endif //GPLATES_NO_PYTHON
