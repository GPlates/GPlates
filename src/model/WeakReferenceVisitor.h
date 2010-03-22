/* $Id$ */

/**
 * \file 
 * Contains the definition of the class WeakReferenceVisitor.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_WEAKREFERENCEVISITOR_H
#define GPLATES_MODEL_WEAKREFERENCEVISITOR_H

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "WeakObserverVisitor.h"
#include "WeakReference.h"

namespace GPlatesModel
{
	template<typename H>
	class WeakReferenceVisitorImpl;

	/**
	 * A visitor that visits WeakReference<H> instances. Its behaviour is
	 * determined at run-time by the WeakReferenceVisitorImpl instance passed into
	 * the constructor, following the Strategy design pattern.
	 */
	template<typename H>
	class WeakReferenceVisitor :
		public WeakObserverVisitor<H>,
		public boost::noncopyable
	{
		
	public:

		/**
		 * Constructs a WeakReferenceVisitor with a WeakReferenceVisitorImpl instance.
		 * @param impl A pointer to a WeakReferenceVisitorImpl; the WeakReferenceVisitor
		 * takes ownership of the pointer.
		 */
		WeakReferenceVisitor(
				WeakReferenceVisitorImpl<H> *impl) :
			d_impl(impl)
		{
		}

		virtual
		~WeakReferenceVisitor()
		{
		}

		virtual
		void
		visit_weak_reference(
				WeakReference<H> &weak_reference);

	private:

		boost::scoped_ptr<WeakReferenceVisitorImpl<H> > d_impl;

	};

	/**
	 * Impl base class for WeakReferenceVisitor.
	 */
	template<typename H>
	class WeakReferenceVisitorImpl
	{
	
	public:

		virtual
		~WeakReferenceVisitorImpl()
		{
		}

		virtual
		void
		visit_weak_reference(
				WeakReference<H> &weak_reference) = 0;

	};

	template<typename H>
	void
	WeakReferenceVisitor<H>::visit_weak_reference(
			WeakReference<H> &weak_reference)
	{
		d_impl->visit_weak_reference(weak_reference);
	}

	/**
	 * Notifies the WeakReference that its publisher has been modified.
	 */
	template<typename H>
	class WeakReferencePublisherModifiedVisitor :
		public WeakReferenceVisitorImpl<H>
	{

	public:

		virtual
		void
		visit_weak_reference(
				WeakReference<H> &weak_reference)
		{
			weak_reference.publisher_modified();
		}

	};

	/**
	 * Notifies the WeakReference that its publisher has been deactivated (conceptually deleted).
	 */
	template<typename H>
	class WeakReferencePublisherDeactivatedVisitor :
		public WeakReferenceVisitorImpl<H>
	{
	
	public:

		virtual
		void
		visit_weak_reference(
				WeakReference<H> &weak_reference)
		{
			weak_reference.publisher_deactivated();
		}

	};

	/**
	 * Notifies the WeakReference that its publisher has been reactivated (conceptually undeleted).
	 */
	template<typename H>
	class WeakReferencePublisherReactivatedVisitor :
		public WeakReferenceVisitorImpl<H>
	{
	
	public:

		virtual
		void
		visit_weak_reference(
				WeakReference<H> &weak_reference)
		{
			weak_reference.publisher_reactivated();
		}

	};

	/**
	 * Notifies the WeakReference that its publisher is about to be destroyed (in the C++ sense).
	 */
	template<typename H>
	class WeakReferencePublisherDestroyedVisitor :
		public WeakReferenceVisitorImpl<H>
	{
	
	public:

		virtual
		void
		visit_weak_reference(
				WeakReference<H> &weak_reference)
		{
			weak_reference.publisher_about_to_be_destroyed();
		}

	};

}

#endif  // GPLATES_MODEL_WEAKREFERENCEVISITOR_H
