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

#ifndef GPLATES_UTILS_DEFERREDCALLEVENT_H
#define GPLATES_UTILS_DEFERREDCALLEVENT_H

#include <boost/function.hpp>
#include <QEvent>


namespace GPlatesUtils
{
	/**
	 * DeferredCallEvent is useful if you don't want to process something right now
	 * but you want to put it onto the event queue for later processing.
	 *
	 * In particular, a DeferredCallEvent can be created by an object if a
	 * GUI-related action is requested of it from a non-GUI thread, and posted to
	 * itself for processing on the GUI thread.
	 */
	class DeferredCallEvent :
			public QEvent
	{
	public:

		static const QEvent::Type TYPE;

		typedef boost::function< void () > deferred_call_type;

		/**
		 * Constructs a @a DeferredCallEvent with the given @a deferred_call.
		 */
		explicit
		DeferredCallEvent(
				const deferred_call_type &deferred_call);

		/**
		 * Executes the stored deferred call.
		 */
		void
		execute();

		/**
		 * Constructs a @a DeferredCallEvent with the given @a deferred_call and posts
		 * it to the @a QApplication instance living in the main GUI thread.
		 *
		 * Note: This function should only be used if you wish the @a deferred_call to
		 * be run on the GUI thread. If you wish it to be run on another thread,
		 * create the event object yourself and use @a QCoreApplication::postEvent,
		 * where the receiving object lives on the target thread. The receiving object
		 * must handle the @a DeferredCallEvent in its @a event function.
		 */
		static
		void
		defer_call(
				const deferred_call_type &deferred_call);

	private:

		deferred_call_type d_deferred_call;
	};
}

#endif  /* GPLATES_UTILS_DEFERREDCALLEVENT_H */
