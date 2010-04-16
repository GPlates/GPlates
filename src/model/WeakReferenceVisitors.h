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

#include <vector>
#include <boost/shared_ptr.hpp>

#include "HandleTraits.h"
#include "WeakObserverVisitor.h"
#include "WeakReference.h"
#include "WeakReferenceCallback.h"

namespace GPlatesModel
{
	/**
	 * Notifies the WeakReference that its publisher has been modified.
	 */
	template<typename H>
	class WeakReferencePublisherModifiedVisitor :
			public WeakObserverVisitor<H>
	{

	public:

		WeakReferencePublisherModifiedVisitor(
				typename WeakReferencePublisherModifiedEvent<H>::Type type) :
			d_type(type)
		{
		}

		virtual
		void
		visit_weak_reference(
				WeakReference<H> &weak_reference)
		{
			weak_reference.publisher_modified(d_type);
		}

	private:

		typename WeakReferencePublisherModifiedEvent<H>::Type d_type;

	};

	template<typename H>
	class WeakReferencePublisherAddedVisitor :
			public WeakObserverVisitor<H>
	{
	
	public:

		WeakReferencePublisherAddedVisitor(
				const typename WeakReferencePublisherAddedEvent<H>::new_children_container_type &new_children) :
			d_new_children(new_children)
		{
		}

		virtual
		void
		visit_weak_reference(
				WeakReference<H> &weak_reference)
		{
			weak_reference.publisher_added(d_new_children);
		}

	private:

		const typename WeakReferencePublisherAddedEvent<H>::new_children_container_type &d_new_children;

	};

	/**
	 * Notifies the WeakReference that its publisher has been deactivated (conceptually deleted).
	 */
	template<typename H>
	class WeakReferencePublisherDeactivatedVisitor :
			public WeakObserverVisitor<H>
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
			public WeakObserverVisitor<H>
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
			public WeakObserverVisitor<H>
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
