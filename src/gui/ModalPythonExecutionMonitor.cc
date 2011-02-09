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
 
#include <algorithm>
#include <boost/foreach.hpp>
#include <QEvent>
#include <QApplication>
#include <QDebug>

#include "ModalPythonExecutionMonitor.h"

#include "qt-widgets/QtWidgetUtils.h"


namespace
{
	QEvent::Type PERMITTED_EVENTS[] = {
		QEvent::LayoutRequest,
		QEvent::MetaCall,
		QEvent::Move,
		QEvent::Paint,
		QEvent::Resize,
		QEvent::Show,
		QEvent::ShowToParent,
		QEvent::Timer,
		QEvent::UpdateRequest,
		QEvent::ZOrderChange
	};

	std::size_t NUM_PERMITTED_EVENTS = sizeof(PERMITTED_EVENTS) / sizeof(QEvent::Type);
	QEvent::Type *PERMITTED_EVENTS_END = PERMITTED_EVENTS + NUM_PERMITTED_EVENTS;

	/**
	 * Basically, we want to block all user interaction while ensuring that the UI
	 * can refresh itself (e.g. when Python prints something out).
	 *
	 * If you're finding that a certain widget isn't responding the way it should
	 * during execution of Python code, see which events are being discarded and
	 * add the appropriate ones to the @a PERMITTED_EVENTS array above (but make
	 * sure that nothing happens when the user clicks or types anything!).
	 */
	bool
	is_permitted_while_monitoring(
			QEvent::Type type)
	{
		return type >= QEvent::User ||
			(std::find(PERMITTED_EVENTS, PERMITTED_EVENTS_END, type) != PERMITTED_EVENTS_END);
	}

	bool
	is_ancestor(
			QWidget *widget,
			QObject *obj)
	{
		while (obj != NULL)
		{
			if (widget == obj)
			{
				return true;
			}
			obj = obj->parent();
		}

		return false;
	}

	bool
	is_exempt(
			QObject *obj,
			const std::set<QWidget *> &exempt_widgets)
	{
		BOOST_FOREACH(QWidget *widget, exempt_widgets)
		{
			if (is_ancestor(widget, obj))
			{
				return true;
			}
		}
		return false;
	}
}


GPlatesGui::ModalPythonExecutionMonitor::ModalPythonExecutionMonitor(
		long python_thread_id) :
	GPlatesApi::PythonExecutionMonitor(python_thread_id),
	d_thread_interrupted(false)
{  }


bool
GPlatesGui::ModalPythonExecutionMonitor::exec()
{
	qApp->installEventFilter(this);
	bool result = GPlatesApi::PythonExecutionMonitor::exec();
	qApp->removeEventFilter(this);
	return result;
}


void
GPlatesGui::ModalPythonExecutionMonitor::add_blackout_exemption(
		QWidget *widget)
{
	d_exempt_widgets.insert(widget);
}


void
GPlatesGui::ModalPythonExecutionMonitor::remove_blackout_exemption(
		QWidget *widget)
{
	d_exempt_widgets.erase(widget);
}


bool
GPlatesGui::ModalPythonExecutionMonitor::eventFilter(
		QObject *obj,
		QEvent *ev)
{
	if (ev->type() == QEvent::KeyPress &&
			GPlatesQtWidgets::QtWidgetUtils::is_control_c(
				static_cast<QKeyEvent *>(ev)))
	{
		interrupt_python_thread_if_not_yet_interrupted();
		return true;
	}
	else if (is_exempt(obj, d_exempt_widgets) ||
		is_permitted_while_monitoring(ev->type()))
	{
		return GPlatesApi::PythonExecutionMonitor::eventFilter(obj, ev);
	}
	else
	{
		ev->ignore();
		return true;
	}
}


void
GPlatesGui::ModalPythonExecutionMonitor::interrupt_python_thread_if_not_yet_interrupted()
{
	if (!d_thread_interrupted)
	{
		interrupt_python_thread();
		emit python_thread_interrupted();
		d_thread_interrupted = true;
	}
}

