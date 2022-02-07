/* $Id$ */

/**
 * \file 
 * Contains the definition of the class WeakReferenceCallback.
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

#ifndef GPLATES_MODEL_WEAKREFERENCECALLBACK_H
#define GPLATES_MODEL_WEAKREFERENCECALLBACK_H

#include <vector>
#include <boost/intrusive_ptr.hpp>

#include "HandleTraits.h"
#include "utils/ReferenceCount.h"

namespace GPlatesModel
{
	// Forward declaration of weak reference.
	template<typename H> class WeakReference;


	/**
	 * Parameter of publisher_modified() function in WeakReferenceCallback<H>.
	 */
	template<typename H>
	class WeakReferencePublisherModifiedEvent
	{
	public:
		enum Type
		{
			NONE = 0,

			/**
			 * Signifies that the publisher itself was modified, e.g. WeakReference to
			 * feature collection, feature added to that feature collection.
			 */
			PUBLISHER_MODIFIED = 1,

			/**
			 * Signifies that a child of the publisher was modified, e.g. WeakReference
			 * to feature collection, property of feature in that feature collection modified.
			 */
			CHILD_MODIFIED = 2,

			/**
			 * Signifies that both the publisher and a child of the publisher was
			 * modified. Normally, in one transaction, either the publisher or a child
			 * (but not both) is modified. Such an event, however, could be emitted
			 * after the lifting of a NotificationGuard that suppressed two separate
			 * events, one where the publisher was modified and another where a child
			 * was modified.
			 */
			PUBLISHER_AND_CHILD_MODIFIED = (PUBLISHER_MODIFIED | CHILD_MODIFIED)
		};

		explicit
		WeakReferencePublisherModifiedEvent(
				Type type_) :
			d_type(type_)
		{
		}

		Type
		type() const
		{
			return d_type;
		}

	private:

		Type d_type;
	};

	/**
	 * Parameter of publisher_added() function in WeakReferenceCallback<H>.
	 */
	template<typename H>
	class WeakReferencePublisherAddedEvent
	{
	private:

		// Helper traits class to choose appropriate const-ness for added children.
		template<class T>
		struct Traits
		{
			typedef typename HandleTraits<T>::iterator iterator;
		};

		template<class T>
		struct Traits<const T>
		{
			typedef typename HandleTraits<T>::const_iterator iterator;
		};

	public:

		typedef std::vector<typename Traits<H>::iterator> new_children_container_type;

		explicit
		WeakReferencePublisherAddedEvent(
				const new_children_container_type &new_children_) :
			d_new_children(new_children_)
		{
		}

		const new_children_container_type &
		new_children() const
		{
			return d_new_children;
		}

	private:

		const new_children_container_type &d_new_children;
	};

	/**
	 * Parameter of publisher_deactivated() function in WeakReferenceCallback<H>.
	 */
	template<typename H>
	class WeakReferencePublisherDeactivatedEvent
	{
	public:
	};

	/**
	 * Parameter of publisher_reactivated() function in WeakReferenceCallback<H>.
	 */
	template<typename H>
	class WeakReferencePublisherReactivatedEvent
	{
	public:
	};

	/**
	 * Parameter of publisher_about_to_be_destroyed() function in WeakReferenceCallback<H>.
	 */
	template<typename H>
	class WeakReferencePublisherAboutToBeDestroyedEvent
	{
	public:
	};

	/**
	 * WeakReferenceCallback instances can be attached to WeakReference instances
	 * to enable the owner of a WeakReference to receive callbacks when the
	 * WeakReference's publisher is modified, deactivated, reactivated and
	 * about to be destroyed.
	 */
	template<typename H>
	class WeakReferenceCallback :
			public GPlatesUtils::ReferenceCount<WeakReferenceCallback<H> >
	{
	
	public:

		/**
		 * A convenience typedef for weak reference.
		 */
		typedef WeakReference<H> weak_reference_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<WeakReferenceCallback<H> >.
		 */
		typedef boost::intrusive_ptr<WeakReferenceCallback<H> > maybe_null_ptr_type;

		typedef WeakReferencePublisherModifiedEvent<H> modified_event_type;
		typedef WeakReferencePublisherAddedEvent<H> added_event_type;
		typedef WeakReferencePublisherDeactivatedEvent<H> deactivated_event_type;
		typedef WeakReferencePublisherReactivatedEvent<H> reactivated_event_type;
		typedef WeakReferencePublisherAboutToBeDestroyedEvent<H> about_to_be_destroyed_event_type;

		//! Virtual destructor.
		virtual
		~WeakReferenceCallback()
		{
		}

		/**
		 * Called by WeakReference when its publisher is modified.
		 * Reimplement in a subclass to customise behaviour on publisher modification.
		 */
		virtual
		void
		publisher_modified(
				const weak_reference_type &reference,
				const modified_event_type &event)
		{
		}

		/**
		 * Called by WeakReference when its publisher has added new children.
		 * Reimplement in a subclass to customise behaviour on publisher addition.
		 */
		virtual
		void
		publisher_added(
				const weak_reference_type &reference,
				const added_event_type &event)
		{
		}

		/**
		 * Called by WeakReference when its publisher is deactivated. A publisher is
		 * deactivated when it is conceptually deleted from the model, but the object
		 * still exists for undo purposes.
		 * Reimplement in a subclass to customise behaviour on publisher deactivation.
		 */
		virtual
		void
		publisher_deactivated(
				const weak_reference_type &reference,
				const deactivated_event_type &event)
		{
		}

		/**
		 * Called by WeakReference when its publisher is reactivated. A publisher is
		 * reactivated when it was conceptually deleted from the model, but the user
		 * requested that the deletion be undone.
		 * Reimplement in a subclass to customise behaviour on publisher reactivation.
		 */
		virtual
		void
		publisher_reactivated(
				const weak_reference_type &reference,
				const reactivated_event_type &event)
		{
		}

		/**
		 * Called by WeakReference when its publisher is about to be destroyed (in
		 * the C++ sense). This might occur when the publisher was conceptually
		 * deleted from the model, and the user requested that the undo history stack
		 * be purged.
		 * Reimplement in a subclass to customise behaviour on publisher destruction.
		 */
		virtual
		void
		publisher_about_to_be_destroyed(
				const weak_reference_type &reference,
				const about_to_be_destroyed_event_type &event)
		{
		}

	};
}

#endif  // GPLATES_MODEL_WEAKREFERENCECALLBACK_H
