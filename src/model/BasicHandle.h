/* $Id$ */

/**
 * \file 
 * Contains the definition of the class BasicHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
 * (also incorporating code from "FeatureHandle.h", "FeatureHandle.cc", "FeatureCollectionHandle.h",
 * "FeatureCollectionHandle.cc", "FeatureStoreRootHandle.h" and "FeatureStoreRootHandle.cc")
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

#ifndef GPLATES_MODEL_BASICHANDLE_H
#define GPLATES_MODEL_BASICHANDLE_H

#include <boost/optional.hpp>

#include "ChangesetHandle.h"
#include "HandleTraits.h"
#include "Model.h"
#include "WeakObserverPublisher.h"
#include "WeakReference.h"
#include "WeakReferenceCallback.h"
#include "WeakReferenceVisitors.h"
#include "types.h"

#include "global/PointerTraits.h"

namespace GPlatesModel
{
	class Model;

	/**
	 * BasicHandle contains functionality common to all Handle classes. This
	 * common functionality is brought into the Handle classes by way of
	 * inheritance. For example, FeatureHandle is derived from
	 * BasicHandle<FeatureHandle>. (Although delegation is usually preferred to
	 * inheritance, the use of inheritance in this case significantlly simplifies
	 * the Handle class interfaces.
	 */
	template<class HandleType> // HandleType is one of FeatureHandle, FeatureCollectionHandle or FeatureStoreRootHandle.
	class BasicHandle :
			public WeakObserverPublisher<HandleType>
	{

	public:

		// Get typedefs from HandleTraits.
		typedef HandleType handle_type;
		typedef typename HandleTraits<handle_type>::non_null_ptr_type non_null_ptr_type;
		typedef typename HandleTraits<handle_type>::non_null_ptr_to_const_type non_null_ptr_to_const_type;
		typedef typename HandleTraits<handle_type>::weak_ref weak_ref;
		typedef typename HandleTraits<handle_type>::const_weak_ref const_weak_ref;
		typedef typename HandleTraits<handle_type>::iterator iterator;
		typedef typename HandleTraits<handle_type>::const_iterator const_iterator;
		typedef typename HandleTraits<handle_type>::revision_type revision_type;
		typedef typename HandleTraits<handle_type>::parent_type parent_type;
		typedef typename HandleTraits<handle_type>::child_type child_type;

		/**
		 * The type of this class.
		 */
		typedef BasicHandle<handle_type> this_type;

		/**
		 * Destructor.
		 */
		~BasicHandle();

		/**
		 * Returns a const-weak-ref to this Handle instance.
		 */
		const const_weak_ref
		reference() const;

		/**
		 * Returns a (non-const) weak-ref to this Handle instance.
		 */
		const weak_ref
		reference();

		/**
		 * Returns the "begin" const-iterator to iterate over the collection of children.
		 */
		const const_iterator
		begin() const;

		/**
		 * Returns the "begin" iterator to iterate over the collection of children.
		 */
		const iterator
		begin();

		/**
		 * Returns the "end" const-iterator used during iteration over the collection of children.
		 */
		const const_iterator
		end() const;

		/**
		 * Returns the "end" iterator used during iteration over the collection of children.
		 */
		const iterator
		end();

		/**
		 * Returns the number of children elements this Handle contains as of the
		 * current revision.
		 */
		container_size_type
		size() const;

		/**
		 * Adds @a new_child to the collection.
		 * 
		 * @a new_child must be a pointer to a child_type that has not already been
		 * added to a feature_collection. Behaviour is undefined if a child_type is
		 * added to two different Handles.
		 *
		 * Returns an iterator that points to the new element in the collection.
		 *
		 * NOTE: this function may make a clone of the parameter @a new_child to
		 * insert into the model. Therefore, you must not use the parameter after
		 * this call; use the returned iterator instead.
		 *
		 * After the child has been appended, the "end" iterator will have
		 * advanced -- the length of the sequence will have increased by 1, so what was the
		 * iterator to the last element of the sequence (the "back" of the container), will
		 * now be the iterator to the second-last element of the sequence; what was the
		 * "end" iterator will now be the iterator to the last element of the sequence.
		 */
		const iterator
		add(
				typename GPlatesGlobal::PointerTraits<child_type>::non_null_ptr_type new_child);

		/**
		 * Removes the child indicated by @a iter in the collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a child-slot will become NULL.
		 */
		void
		remove(
				const_iterator iter);

		/**
		 * Gets the child indicated by @a iter in the collection.
		 * 
		 * The results of this operation are only defined if @a iter is a valid
		 * iterator, and the iterator belongs to this Handle.
		 */
		typename GPlatesGlobal::PointerTraits<child_type>::non_null_ptr_type
		get(
				iterator iter);

		/**
		 * Gets the child indicated by @a iter in the collection.
		 * 
		 * The results of this operation are only defined if @a iter is a valid
		 * iterator, and the iterator belongs to this Handle.
		 */
		typename GPlatesGlobal::PointerTraits<const child_type>::non_null_ptr_type
		get(
				const_iterator iter) const;

		/**
		 * Sets the pointer to the parent object that contains this feature.
		 *
		 * Client code should not use this function!
		 *
		 * This function should only be invoked by a Revision instance
		 * when it has appended or removed a feature.  This is part of the mechanism which
		 * tracks whether a feature collection contains unsaved changes, and (later) part
		 * of the Bubble-Up mechanism.
		 */
		void
		set_parent_ptr(
				parent_type *new_ptr,
				container_size_type new_index);

		/**
		 * Gets a pointer to the parent object that contains this feature.
		 */
		parent_type *
		parent_ptr();

		/**
		 * Returns the index of this Handle in its parent container.
		 */
		container_size_type
		index_in_container();

		/**
		 * Returns a pointer to the Model to which this Handle belongs. Returns NULL
		 * if this Handle has no parent set.
		 */
		Model *
		model_ptr() const;

		/**
		 * Returns true if the Handle is active and in the current state of the model.
		 * If false, the Handle has been conceptually deleted.
		 */
		bool
		is_active() const;

		/**
		 * Sets whether this Handle is active or not. An event is emitted to
		 * callbacks registered with weak references to this Handle.
		 *
		 * If @a active is true, the Handle is reactivated and if @a active is false,
		 * the Handle is deactivated.
		 *
		 * All children (and children of children, etc) of this Handle have their
		 * active flag set to @a active too.
		 *
		 * This function has no effect if @a active is the same as is_active().
		 */
		void
		set_active(
				bool active = true);

		/**
		 * This function should be called by a child when the child is modified.
		 * An event is emitted to callbacks registered with weak references to this
		 * Handle.
		 */
		void
		handle_child_modified();

		/**
		 * Flushes pending notifications that were held up due to an active NotificationGuard.
		 *
		 * This will call flush_children_pending_notifications() to recurively call
		 * flush_pending_notifications() in children objects.
		 */
		void
		flush_pending_notifications();

	protected:

		/**
		 * Accesses the current revision of the conceptual object accessed by this Handle.
		 */
		const typename GPlatesGlobal::PointerTraits<const revision_type>::non_null_ptr_type
		current_revision() const;

		/**
		 * Accesses the current revision of the conceptual object accessed by this Handle.
		 */
		const typename GPlatesGlobal::PointerTraits<revision_type>::non_null_ptr_type
		current_revision();

		/**
		 * Constructor, given a particular revision object.
		 */
		BasicHandle(
				handle_type *handle_ptr_,
				typename GPlatesGlobal::PointerTraits<revision_type>::non_null_ptr_type revision);

		/**
		 * Notify our listeners of the modification of this Handle.
		 *
		 * If @a publisher_modified is true, this means that something in this Handle
		 * itself was modified
		 *
		 * If @a child_modified is true, this means that one of this Handle's
		 * children was modified instead.
		 */
		void
		notify_listeners_of_modification(
				bool publisher_modified,
				bool child_modified);

		/**
		 * Notify our listeners of deactivation (conceptual deletion) of this Handle.
		 */
		void
		notify_listeners_of_deactivation();

		/**
		 * Notify our listeners of reactivation (conceptual undeletion) of this Handle.
		 */
		void
		notify_listeners_of_reactivation();

		/**
		 * Notify our listeners of the impending destruction of this Handle in the C++ sense.
		 */
		void
		notify_listeners_of_impending_destruction();

	private:

		/**
		 * Sets the active flag in a particular child of this Handle.
		 */
		void
		set_child_active(
				const_iterator iter,
				bool active);

		/**
		 * Sets the active flag in children of this Handle.
		 */
		void
		set_children_active(
				bool active);

		/**
		 * Notifies our parent of our modification (or a modification in one of our
		 * children).
		 */
		void
		notify_parent_of_modification();

		/**
		 * Does the job of notify_listeners_of_modification() without the guard checks.
		 */
		void
		actual_notify_listeners_of_modification(
				bool publisher_modified,
				bool child_modified);

		/**
		 * Does the job of notify_listeners_of_deactivation() without the guard checks.
		 */
		void
		actual_notify_listeners_of_deactivation();

		/**
		 * Does the job of notify_listeners_of_reactivation() without the guard checks.
		 */
		void
		actual_notify_listeners_of_reactivation();

		/**
		 * Calls flush_pending_notifications() in children objects.
		 */
		void
		flush_children_pending_notifications();

		/**
		 * This constructor should not be defined, because we don't want to be able
		 * to copy construct one of these objects.
		 */
		BasicHandle(
				const this_type &other);

		/**
		 * This should not be defined, because we don't want to be able to copy
		 * one of these objects.
		 */
		this_type &
		operator=(
				const this_type &);

		/**
		 * The current revision of the conceptual object managed by this Handle.
		 */
		typename GPlatesGlobal::PointerTraits<revision_type>::non_null_ptr_type d_current_revision;

		/**
		 * A pointer to an instance of the template parameter Handle type.
		 */
		handle_type *d_handle_ptr;

		/**
		 * The parent that contains the Handle.
		 */
		parent_type *d_parent_ptr;

		/**
		 * The position of this element in its parent's container.
		 */
		container_size_type d_index_in_container;

		/**
		 * If true, the Handle is active and in the current state of the model.
		 * If false, the handle has been conceptually deleted (but it could be
		 * undeleted later).
		 */
		bool d_is_active;

		// Used for holding notifications while a NotificationGuard is active.
		bool d_has_pending_publisher_modification_notification;
		bool d_has_pending_child_modification_notification;
		bool d_has_pending_activation_notification; // either deactivation/reactivation
		boost::optional<bool> d_is_active_before_pending_notifications;

		friend class RevisionAwareIterator<HandleType>;
		friend class RevisionAwareIterator<const HandleType>;

	};


	template<class HandleType>
	BasicHandle<HandleType>::~BasicHandle()
	{
		notify_listeners_of_impending_destruction();
	}


	template<class HandleType>
	const typename BasicHandle<HandleType>::const_weak_ref
	BasicHandle<HandleType>::reference() const
	{
		return const_weak_ref(*d_handle_ptr);
	}


	template<class HandleType>
	const typename BasicHandle<HandleType>::weak_ref
	BasicHandle<HandleType>::reference()
	{
		return weak_ref(*d_handle_ptr);
	}


	template<class HandleType>
	const typename BasicHandle<HandleType>::const_iterator
	BasicHandle<HandleType>::begin() const
	{
		return const_iterator(*d_handle_ptr, 0);
	}


	template<class HandleType>
	const typename BasicHandle<HandleType>::iterator
	BasicHandle<HandleType>::begin()
	{
		return iterator(*d_handle_ptr, 0);
	}


	template<class HandleType>
	const typename BasicHandle<HandleType>::const_iterator
	BasicHandle<HandleType>::end() const
	{
		return const_iterator(*d_handle_ptr, current_revision()->container_size());
	}


	template<class HandleType>
	const typename BasicHandle<HandleType>::iterator
	BasicHandle<HandleType>::end()
	{
		return iterator(*d_handle_ptr, current_revision()->container_size());
	}


	template<class HandleType>
	container_size_type
	BasicHandle<HandleType>::size() const
	{
		return current_revision()->size();
	}


	template<class HandleType>
	const typename BasicHandle<HandleType>::iterator
	BasicHandle<HandleType>::add(
			typename GPlatesGlobal::PointerTraits<child_type>::non_null_ptr_type new_child)
	{
		ChangesetHandle changeset(model_ptr());

		container_size_type new_index =
			current_revision()->add(new_child);
		new_child->set_parent_ptr(d_handle_ptr, new_index);

		notify_listeners_of_modification(true, false);

		return iterator(*d_handle_ptr, new_index);
	}


	// Template specialisations are in the .cc file.
	template<>
	const BasicHandle<FeatureHandle>::iterator
	BasicHandle<FeatureHandle>::add(
			GPlatesGlobal::PointerTraits<TopLevelProperty>::non_null_ptr_type new_child);


	template<class HandleType>
	void
	BasicHandle<HandleType>::remove(
			const_iterator iter)
	{
		ChangesetHandle changeset(model_ptr());

		// Deactivate the child.
		set_child_active(iter, false);

		// Remove from Revision object (currently, this would destroy it in the C++ sense).
		current_revision()->remove(iter.index());

		notify_listeners_of_modification(true, false);
	}


	template<class HandleType>
	typename GPlatesGlobal::PointerTraits<typename BasicHandle<HandleType>::child_type>::non_null_ptr_type
	BasicHandle<HandleType>::get(
			iterator iter)
	{
		boost::intrusive_ptr<child_type> child_ptr = current_revision()->get(iter.index());
		return child_ptr.get();
	}


	template<class HandleType>
	typename GPlatesGlobal::PointerTraits<const typename BasicHandle<HandleType>::child_type>::non_null_ptr_type
	BasicHandle<HandleType>::get(
			const_iterator iter) const
	{
		boost::intrusive_ptr<const child_type> child_ptr = current_revision()->get(iter.index());
		return child_ptr.get();
	}


	template<class HandleType>
	const typename GPlatesGlobal::PointerTraits<const typename BasicHandle<HandleType>::revision_type>::non_null_ptr_type
	BasicHandle<HandleType>::current_revision() const
	{
		return d_current_revision;
	}


	template<class HandleType>
	const typename GPlatesGlobal::PointerTraits<typename BasicHandle<HandleType>::revision_type>::non_null_ptr_type
	BasicHandle<HandleType>::current_revision()
	{
		return d_current_revision;
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::set_parent_ptr(
			parent_type *new_ptr,
			container_size_type new_index)
	{
		d_parent_ptr = new_ptr;
		d_index_in_container = new_index;
	}


	template<class HandleType>
	typename BasicHandle<HandleType>::parent_type *
	BasicHandle<HandleType>::parent_ptr()
	{
		return d_parent_ptr;
	}


	template<class HandleType>
	container_size_type
	BasicHandle<HandleType>::index_in_container()
	{
		return d_index_in_container;
	}


	template<class HandleType>
	Model *
	BasicHandle<HandleType>::model_ptr() const
	{
		if (d_parent_ptr)
		{
			return d_parent_ptr->model_ptr();
		}
		else
		{
			return NULL;
		}
	}


	// Template specialisations are in the .cc file.
	template<>
	Model *
	BasicHandle<FeatureStoreRootHandle>::model_ptr() const;


	template<class HandleType>
	bool
	BasicHandle<HandleType>::is_active() const
	{
		return d_is_active;
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::set_active(
			bool active)
	{
		if (active != d_is_active)
		{
			d_is_active = active;

			if (active)
			{
				notify_listeners_of_reactivation();
			}
			else
			{
				notify_listeners_of_deactivation();
			}

			set_children_active(active);
		}
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::set_child_active(
			const_iterator iter,
			bool active)
	{
		BasicHandle<child_type> &child = dynamic_cast<BasicHandle<child_type> &>(
				*current_revision()->get(iter.index()));
		child.set_active(active);
	}


	// Template specialisations are in the .cc file.
	template<>
	void
	BasicHandle<FeatureHandle>::set_child_active(
			const_iterator iter,
			bool active);


	template<class HandleType>
	void
	BasicHandle<HandleType>::set_children_active(
			bool active)
	{
		for (const_iterator iter = begin(); iter != end(); ++iter)
		{
			set_child_active(iter, active);
		}
	}


	// Template specialisations are in the .cc file.
	template<>
	void
	BasicHandle<FeatureHandle>::set_children_active(
			bool active);


	template<class HandleType>
	BasicHandle<HandleType>::BasicHandle(
			handle_type *handle_ptr_,
			typename GPlatesGlobal::PointerTraits<revision_type>::non_null_ptr_type revision) :
		d_current_revision(revision),
		d_handle_ptr(handle_ptr_),
		d_parent_ptr(NULL),
		d_index_in_container(INVALID_INDEX),
		d_is_active(true),
		d_has_pending_publisher_modification_notification(false),
		d_has_pending_child_modification_notification(false),
		d_has_pending_activation_notification(false)
	{
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::handle_child_modified()
	{
		notify_listeners_of_modification(false, true);
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::notify_listeners_of_modification(
			bool publisher_modified,
			bool child_modified)
	{
		Model *model = model_ptr();

		if (model && model->has_notification_guard())
		{
			// Just remember what notifications we need to send later.
			if (publisher_modified)
			{
				d_has_pending_publisher_modification_notification = true;
			}
			if (child_modified)
			{
				d_has_pending_child_modification_notification = true;
			}
		}
		else
		{
			actual_notify_listeners_of_modification(
					publisher_modified,
					child_modified);
		}

		// We always notify the parent even if there is a NotificationGuard.
		// It's the parent's job to hold the notification until the guard is lifted.
		notify_parent_of_modification();
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::actual_notify_listeners_of_modification(
			bool publisher_modified,
			bool child_modified)
	{
		int publisher_bit = publisher_modified ?
			WeakReferencePublisherModifiedEvent<HandleType>::PUBLISHER_MODIFIED :
			WeakReferencePublisherModifiedEvent<HandleType>::NONE;
		int child_bit = child_modified ?
			WeakReferencePublisherModifiedEvent<HandleType>::CHILD_MODIFIED :
			WeakReferencePublisherModifiedEvent<HandleType>::NONE;
		typename WeakReferencePublisherModifiedEvent<HandleType>::Type type =
			static_cast<typename WeakReferencePublisherModifiedEvent<HandleType>::Type>(
					publisher_bit | child_bit);

		WeakReferencePublisherModifiedVisitor<HandleType> visitor(type);
		apply_weak_observer_visitor(visitor);
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::notify_listeners_of_deactivation()
	{
		Model *model = model_ptr();

		if (model && model->has_notification_guard())
		{
			// Just remember what notifications we need to send later.
			d_has_pending_activation_notification = true;
			if (!d_is_active_before_pending_notifications)
			{
				d_is_active_before_pending_notifications = d_is_active;
			}
		}
		else
		{
			actual_notify_listeners_of_deactivation();
		}
	}

	
	template<class HandleType>
	void
	BasicHandle<HandleType>::actual_notify_listeners_of_deactivation()
	{
		WeakReferencePublisherDeactivatedVisitor<HandleType> visitor;
		apply_weak_observer_visitor(visitor);
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::notify_listeners_of_reactivation()
	{
		Model *model = model_ptr();

		if (model && model->has_notification_guard())
		{
			// Just remember what notifications we need to send later.
			d_has_pending_activation_notification = true;
			if (!d_is_active_before_pending_notifications)
			{
				d_is_active_before_pending_notifications = d_is_active;
			}
		}
		else
		{
			actual_notify_listeners_of_reactivation();
		}
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::actual_notify_listeners_of_reactivation()
	{
		WeakReferencePublisherReactivatedVisitor<HandleType> visitor;
		apply_weak_observer_visitor(visitor);
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::notify_listeners_of_impending_destruction()
	{
		// We always notify our listeners of impending destruction, even if there is
		// a NotificationGuard active.
		WeakReferencePublisherDestroyedVisitor<HandleType> visitor;
		apply_weak_observer_visitor(visitor);
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::notify_parent_of_modification()
	{
		if (d_parent_ptr)
		{
			BasicHandle<parent_type> &parent = *(dynamic_cast<BasicHandle<parent_type> *>(d_parent_ptr));
			parent.handle_child_modified();
		}
	}


	// Template specialisations are in the .cc file.
	template<>
	void
	BasicHandle<FeatureStoreRootHandle>::notify_parent_of_modification();


	template<class HandleType>
	void
	BasicHandle<HandleType>::flush_pending_notifications()
	{
		flush_children_pending_notifications();

		if (d_has_pending_publisher_modification_notification ||
				d_has_pending_child_modification_notification)
		{
			actual_notify_listeners_of_modification(
					d_has_pending_publisher_modification_notification,
					d_has_pending_child_modification_notification);
			d_has_pending_publisher_modification_notification = false;
			d_has_pending_child_modification_notification = false;
		}

		if (d_has_pending_activation_notification)
		{
			// We were deactivated and/or reactivated, possibly multiple times.
			// Our listeners need not be bothered with such trivialities, so we will
			// only send a notification if d_is_active has changed.
			bool was_active = *d_is_active_before_pending_notifications;
			if (d_is_active && !was_active)
			{
				actual_notify_listeners_of_reactivation();
			}
			else if (!d_is_active && was_active)
			{
				actual_notify_listeners_of_deactivation();
			}

			d_has_pending_activation_notification = false;
		}
	}


	template<class HandleType>
	void
	BasicHandle<HandleType>::flush_children_pending_notifications()
	{
		for (iterator iter = begin(); iter != end(); ++iter)
		{
			BasicHandle<child_type> &child = *dynamic_cast<BasicHandle<child_type> *>((*iter).get());
			child.flush_pending_notifications();
		}
	}


	// Template specialisations are in the .cc file.
	template<>
	void
	BasicHandle<FeatureHandle>::flush_children_pending_notifications();

}

#endif  // GPLATES_MODEL_HANDLE_H
