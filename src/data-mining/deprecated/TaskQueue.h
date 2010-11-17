/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATESDATAMINING_TASKQUEUE_H
#define GPLATESDATAMINING_TASKQUEUE_H

#include <QDebug>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/xtime.hpp>
#include "Prospector.h"

namespace GPlatesDataMining
{
#define MaxConcurrentThreads 4

	class TaskQueue;
	class TaskScheduler
	{
	public:
		TaskScheduler(TaskQueue*);

		void
		operator()();

	private:
		TaskQueue* d_task_queue;
	};

	class TaskQueue 
	{
	public:
		TaskQueue() :
		  d_shutdown(false)
		{
		
		}
		
		~TaskQueue()
		{
			for(unsigned i = 0; i < d_threads.size(); ++i )
			{
				//d_threads[i]->join();
				delete d_threads[i];
				qDebug() << "destructing task queue";
			}
		}

		void
		init()
		{
			d_shutdown = false;
			for(int i = 0; i < MaxConcurrentThreads; ++i )
			{
				TaskScheduler t(this);
				d_threads.push_back(new boost::thread(t));
			}
		}

		void
		add(Prospector* r)
		{
			boost::mutex::scoped_lock lk(d_wait_queue_mux);
			d_wait_queue.push(r);
			qDebug() << "Add a task and notify waiter.";
			lk.unlock();
			d_wait_task_cond.notify_one();
		}
		
		Prospector*
		fetch()
		{
			boost::mutex::scoped_lock lk(d_wait_queue_mux);
			if(!d_wait_queue.empty())
			{
				Prospector* tmp = d_wait_queue.front();
				d_wait_queue.pop();
				return tmp;
			}
			else
			{
				if(shutdown_flag())
				{
					return NULL;
				}
				qDebug() << "Wait for task available.";
				lk.unlock();
				d_queue_empty_cond.notify_one();
				
				return NULL;
			}
		}

		void
		shutdown()
		{
			boost::mutex::scoped_lock lk(d_wait_queue_mux);
			if(!d_wait_queue.empty())
			{
				d_queue_empty_cond.wait(lk);
			}

			{
				boost::mutex::scoped_lock shutdown_lk(d_shutdown_mux);
				d_shutdown = true;
			}

			lk.unlock();
			d_wait_task_cond.notify_all();
			
			for(unsigned i = 0; i < d_threads.size(); ++i )
			{
				d_threads[i]->join();
			}
				return;
		}

		inline
		bool
		shutdown_flag()
		{
			boost::mutex::scoped_lock shutdown_lk(d_shutdown_mux);
			return d_shutdown;
		}
		
	private:
		boost::mutex d_wait_queue_mux;
		boost::mutex d_shutdown_mux;
		boost::mutex d_done_queue_mux;
		boost::condition d_queue_empty_cond;
		boost::condition d_wait_task_cond;
		bool d_shutdown;
		std::queue < Prospector* > d_wait_queue;
		std::queue < Prospector* > d_done_queue;
		std::vector <boost::thread* > d_threads;
	};

	TaskScheduler::TaskScheduler(
			TaskQueue* q) :
		d_task_queue(q)
	{
		
	}

	void
	TaskScheduler::operator()()
	{
		while(1)
		{
			Prospector* task = d_task_queue->fetch();
			
			if(task)
			{
				task->do_job();
			}
			if(d_task_queue->shutdown_flag())
			{	
				qDebug() << "prospector function returned.*************************";
				return;
			}
		}
	}
	
}

#endif