/* $Id$ */

/**
 * @file 
 * Contains the definition of the TypeTraits template class.
 *
 * Most recent change:
 *   $Date$
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

#include "DeferredCallEvent.h"


const QEvent::Type
GPlatesUtils::AbstractDeferredCallEvent::TYPE = static_cast<QEvent::Type>(QEvent::registerEventType());


GPlatesUtils::AbstractDeferredCallEvent::AbstractDeferredCallEvent() :
	QEvent(TYPE)
{  }


GPlatesUtils::DeferredCallEvent::DeferredCallEvent(
		const deferred_call_type &deferred_call) :
	d_deferred_call(deferred_call)
{  }


void
GPlatesUtils::DeferredCallEvent::execute()
{
	d_deferred_call();
}


GPlatesUtils::BlockingDeferredCallEvent::BlockingDeferredCallEvent(
		const deferred_call_type &deferred_call,
		QMutex &mutex,
		QWaitCondition &condition) :
	d_deferred_call(deferred_call),
	d_mutex(mutex),
	d_condition(condition)
{  }


void
GPlatesUtils::BlockingDeferredCallEvent::execute()
{
	d_deferred_call();
	d_mutex.lock();
	d_condition.wakeAll();
	d_mutex.unlock();
}

