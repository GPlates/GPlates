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
#include <boost/type_traits.hpp>
#include <boost/optional.hpp>
#include <QApplication>
#include <QEvent>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>


namespace GPlatesUtils
{
	class AbstractDeferredCallEvent :
			public QEvent
	{
	public:

		static const QEvent::Type TYPE;

		AbstractDeferredCallEvent();

		virtual
		void
		execute() = 0;
	};

	/**
	 * DeferredCallEvent is useful if you don't want to process something right now
	 * but you want to put it onto the event queue for later processing.
	 *
	 * In particular, a DeferredCallEvent can be created by an object if a
	 * GUI-related action is requested of it from a non-GUI thread, and posted to
	 * itself for processing on the GUI thread.
	 */
	class DeferredCallEvent :
			public AbstractDeferredCallEvent
	{
	public:

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
		virtual
		void
		execute();

	private:

		deferred_call_type d_deferred_call;
	};


	/**
	 * This version provides facilities for blocking the calling thread until the
	 * execution has finished on the target thread.
	 */
	class BlockingDeferredCallEvent :
			public AbstractDeferredCallEvent
	{
	public:

		typedef DeferredCallEvent::deferred_call_type deferred_call_type;

		explicit
		BlockingDeferredCallEvent(
				const deferred_call_type &deferred_call,
				QMutex &mutex,
				QWaitCondition &condition);

		virtual
		void
		execute();

	private:

		deferred_call_type d_deferred_call;

		QMutex &d_mutex;
		QWaitCondition &d_condition;
	};


	namespace DeferredCallWithResultEventInternals
	{
		template<typename ResultType, bool IsReference /* = false */>
		struct traits_helper
		{
			typedef boost::optional<typename boost::remove_const<ResultType>::type> storage_type;

			static
			inline
			storage_type
			result_to_storage(
					ResultType result)
			{
				return result;
			}
		};

		template<typename ResultType>
		struct traits_helper<ResultType, true>
		{
			typedef typename boost::add_pointer<ResultType>::type storage_type;

			static
			inline
			storage_type
			result_to_storage(
					ResultType result)
			{
				return &result;
			}
		};

		template<typename ResultType>
		struct traits
		{
			typedef typename traits_helper<
				ResultType,
				boost::is_reference<ResultType>::value
			>::storage_type storage_type;

			static
			inline
			storage_type
			result_to_storage(
					ResultType result)
			{
				return traits_helper<
					ResultType,
					boost::is_reference<ResultType>::value
				>::result_to_storage(result);
			}
		};
	}


	/**
	 * The same idea as @a DeferredCallEvent above but this has facilities for
	 * returning the return value from the function call.
	 *
	 * This version expects @a ResultType to not be a reference type.
	 */
	template<typename ResultType>
	class DeferredCallWithResultEvent :
			public AbstractDeferredCallEvent
	{
	public:

		typedef boost::function< ResultType () > deferred_call_type;
		typedef typename DeferredCallWithResultEventInternals::traits<ResultType>::storage_type storage_type;

		explicit
		DeferredCallWithResultEvent(
				const deferred_call_type &deferred_call,
				QMutex &mutex,
				QWaitCondition &condition,
				storage_type &result) :
			d_deferred_call(deferred_call),
			d_mutex(mutex),
			d_condition(condition),
			d_result(result)
		{  }

		virtual
		void
		execute()
		{
			d_result = DeferredCallWithResultEventInternals::traits<ResultType>::result_to_storage(d_deferred_call());
			d_mutex.lock();
			d_condition.wakeAll();
			d_mutex.unlock();
		}

	private:

		deferred_call_type d_deferred_call;

		QMutex &d_mutex;
		QWaitCondition &d_condition;
		storage_type &d_result;
	};


	template<typename ResultType = void>
	struct DeferCall
	{
		/**
		 * If called from a thread other than the GUI thread:
		 *
		 * Constructs a @a DeferredCallEvent with the given @a deferred_call and posts
		 * it to the @a QApplication instance living in the main GUI thread. This then
		 * blocks the calling thread until the GUI thread has completed execution and
		 * returns the return value from the function call.
		 *
		 * If called from the GUI thread:
		 *
		 * Runs @a deferred_call immediately and returns the result.
		 *
		 * The @a blocking parameter is always ignored.
		 *
		 * Note: This function should only be used if you wish the @a deferred_call to
		 * be run on the GUI thread. If you wish it to be run on another thread,
		 * create the event object yourself and use @a QCoreApplication::postEvent,
		 * where the receiving object lives on the target thread. The receiving object
		 * must handle the @a DeferredCallEvent in its @a event function.
		 */
		static
		ResultType
		defer_call(
				const typename DeferredCallWithResultEvent<ResultType>::deferred_call_type &deferred_call,
				bool blocking = false)
		{
			if (QThread::currentThread() == qApp->thread())
			{
				return deferred_call();
			}
			else
			{
				typedef DeferredCallWithResultEvent<ResultType> event_type;
				typename event_type::storage_type result;
				QMutex mutex;
				QWaitCondition condition;
				event_type *ev = new event_type(deferred_call, mutex, condition, result);

				mutex.lock();
				QApplication::postEvent(qApp, ev);
				//in case I forget, http://en.wikipedia.org/wiki/Spurious_wakeup
				//we need to check if the return result is really ready.
				condition.wait(&mutex);
				mutex.unlock();

				return *result;
			}
		}
	};


	template<>
	struct DeferCall<void>
	{
		/**
		 * If called from a thread other than the GUI thread:
		 *
		 * Constructs a @a DeferredCallEvent with the given @a deferred_call and posts
		 * it to the @a QApplication instance living in the main GUI thread. This
		 * returns immediately without waiting for the GUI thread to finish execution.
		 * If @a blocking is true, the calling thread is blocked until the GUI thread
		 * has completed execution.
		 *
		 * If called from the GUI thread:
		 *
		 * Runs @a deferred_call immediately if @a blocking is true.
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
				const DeferredCallEvent::deferred_call_type &deferred_call,
				bool blocking = false)
		{
			if (blocking)
			{
				if (QThread::currentThread() == qApp->thread())
				{
					deferred_call();
				}
				else
				{
					QMutex mutex;
					QWaitCondition condition;
					BlockingDeferredCallEvent *ev = new BlockingDeferredCallEvent(deferred_call, mutex, condition);

					mutex.lock();
					QApplication::postEvent(qApp, ev);
					condition.wait(&mutex);
					mutex.unlock();
				}
			}
			else
			{
				QApplication::postEvent(qApp, new DeferredCallEvent(deferred_call));
			}
		}
	};
}

#endif  /* GPLATES_UTILS_DEFERREDCALLEVENT_H */
