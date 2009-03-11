/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATUREHANDLE_H
#define GPLATES_MODEL_FEATUREHANDLE_H

#include "FeatureRevision.h"
#include "FeatureId.h"
#include "FeatureType.h"
#include "ConstFeatureVisitor.h"
#include "FeatureVisitor.h"
#include "RevisionAwareIterator.h"
#include "WeakReference.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"


namespace GPlatesModel
{
	// Forward declaration to avoid circularity of headers.
	template<class T> class WeakObserverVisitor;
	class DummyTransactionHandle;
	class FeatureCollectionHandle;

	/**
	 * A feature handle acts as a persistent handle to the revisioned content of a conceptual
	 * feature.
	 *
	 * The feature is the bottom layer/component of the three-tiered conceptual hierarchy of
	 * revisioned objects contained in, and managed by, the feature store:  The feature is an
	 * abstract model of some geological or plate-tectonic object or concept of interest,
	 * consisting of a collection of properties and a feature type.  The feature store contains
	 * a single feature store root, which in turn contains all the currently-loaded feature
	 * collections.  Every currently-loaded feature is contained within a currently-loaded
	 * feature collection.
	 *
	 * The conceptual feature is implemented in two pieces: FeatureHandle and FeatureRevision. 
	 * A FeatureHandle instance contains and manages a FeatureRevision instance, which in turn
	 * contains the revisioned content of the conceptual feature (the mutable properties of the
	 * feature).  A FeatureHandle instance is contained within, and managed by, a
	 * FeatureCollectionRevision instance.
	 *
	 * A feature handle instance is "persistent" in the sense that it will endure, in the same
	 * memory location, for as long as the conceptual feature exists (which will be determined
	 * by the user's choice of when to "flush" deleted features and unloaded feature
	 * collections, after the feature has been deleted or its feature collection has been
	 * unloaded).  The revisioned content of the conceptual feature will be contained within a
	 * succession of feature revisions (with a new revision created as the result of every
	 * modification), but the handle will endure as a persistent means of accessing the current
	 * revision and the content within it.
	 *
	 * The feature handle also contains the properties of a feature which can never change: the
	 * feature type and the feature ID.
	 */
	class FeatureHandle
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<FeatureHandle,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FeatureHandle,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const FeatureHandle,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const FeatureHandle,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * The type of this class.
		 *
		 * This definition is used for template magic.
		 */
		typedef FeatureHandle this_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * The type which contains the revisioning component of a feature.
		 *
		 * This typedef is used by the RevisionAwareIterator.
		 */
		typedef FeatureRevision revision_component_type;

		/**
		 * The type used for const-iterating over the collection of property containers.
		 */
		typedef RevisionAwareIterator<const FeatureHandle,
				const revision_component_type::property_container_collection_type,
				boost::intrusive_ptr<const PropertyContainer> >
				properties_const_iterator;

		/**
		 * The type used for (non-const) iterating over the collection of property
		 * containers.
		 */
		typedef RevisionAwareIterator<FeatureHandle,
				revision_component_type::property_container_collection_type,
				boost::intrusive_ptr<PropertyContainer> >
				properties_iterator;

		/**
		 * The base type of all const weak observers of instances of this class.
		 */
		typedef WeakObserver<const FeatureHandle> const_weak_observer_type;

		/**
		 * The base type of all (non-const) weak observers of instances of this class.
		 */
		typedef WeakObserver<FeatureHandle> weak_observer_type;

		/**
		 * The type used for a weak-ref to a const feature handle.
		 */
		typedef WeakReference<const FeatureHandle> const_weak_ref;

		/**
		 * The type used for a weak-ref to a (non-const) feature handle.
		 */
		typedef WeakReference<FeatureHandle> weak_ref;

		/**
		 * Translate the non-const iterator @a iter to the equivalent const-iterator.
		 */
		static
		const properties_const_iterator
		get_const_iterator(
				const properties_iterator &iter)
		{
			if (iter.collection_handle_ptr() == NULL) {
				return properties_const_iterator();
			}
			return properties_const_iterator::create_index(
					*(iter.collection_handle_ptr()), iter.index());
		}

		/**
		 * Translate the non-const weak-ref @a ref to the equivalent const-weak-ref.
		 */
		static
		const const_weak_ref
		get_const_weak_ref(
				const weak_ref &ref)
		{
			if (ref.handle_ptr() == NULL) {
				return const_weak_ref();
			}
			return const_weak_ref(*(ref.handle_ptr()));
		}

		/**
		 * Destructor.
		 *
		 * Unsubscribes all weak observers.
		 */
		~FeatureHandle();

		/**
		 * Create a new FeatureHandle instance with feature type @a feature_type_ and
		 * feature ID @a feature_id_.
		 */
		static
		const non_null_ptr_type
		create(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_)
		{
			non_null_ptr_type ptr(new FeatureHandle(feature_type_, feature_id_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Create a new FeatureHandle instance with feature type @a feature_type_ and
		 * feature ID @a feature_id_.
		 */
		static
		const non_null_ptr_type
		create(
				const FeatureType &feature_type_,
				const RevisionId &revision_id_)
		{
			non_null_ptr_type ptr(new FeatureHandle(feature_type_, revision_id_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}


		/**
		 * Create a new FeatureHandle instance with feature type @a feature_type_ and
		 * feature ID @a feature_id_.
		 */
		static
		const non_null_ptr_type
		create(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_,
				const RevisionId &revision_id_)
		{
			non_null_ptr_type ptr(new FeatureHandle(feature_type_, feature_id_, revision_id_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}


		/**
		 * Create a duplicate of this FeatureHandle instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new FeatureHandle(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		/**
		 * Return a const-weak-ref to this FeatureHandle instance.
		 */
		const const_weak_ref
		reference() const
		{
			const_weak_ref ref(*this);
			return ref;
		}

		/**
		 * Return a (non-const) weak-ref to this FeatureHandle instance.
		 */
		const weak_ref
		reference()
		{
			weak_ref ref(*this);
			return ref;
		}

		/**
		 * Return the feature type of this feature.
		 *
		 * Note that no "setter" is provided:  The feature type of a feature should never
		 * be changed.
		 */
		const FeatureType &
		feature_type() const
		{
			return d_feature_type;
		}

		/**
		 * Return the feature ID of this feature.
		 *
		 * Note that no "setter" is provided:  The feature ID of a feature should never be
		 * changed.
		 */
		const FeatureId &
		feature_id() const
		{
			return d_feature_id;
		}

		/**
		 * Return the revision ID of the current revision.
		 *
		 * Note that no "setter" is provided:  The revision ID of a revision should never
		 * be changed.
		 */
		const RevisionId &
		revision_id() const
		{
			return current_revision()->revision_id();
		}

		/**
		 * Return the "begin" const-iterator to iterate over the collection of property
		 * containers.
		 */
		const properties_const_iterator
		properties_begin() const
		{
			return properties_const_iterator::create_begin(*this);
		}

		/**
		 * Return the "begin" iterator to iterate over the collection of property
		 * containers.
		 */
		const properties_iterator
		properties_begin()
		{
			return properties_iterator::create_begin(*this);
		}

		/**
		 * Return the "end" const-iterator used during iteration over the collection of
		 * property containers.
		 */
		const properties_const_iterator
		properties_end() const
		{
			return properties_const_iterator::create_end(*this);
		}

		/**
		 * Return the "end" iterator used during iteration over the collection of property
		 * containers.
		 */
		const properties_iterator
		properties_end()
		{
			return properties_iterator::create_end(*this);
		}

		/**
		 * Append @a new_property_container to the property container collection.
		 *
		 * An iterator is returned which points to the new element in the collection.
		 *
		 * After the PropertyContainer has been appended, the "end" iterator will have
		 * advanced -- the length of the sequence will have increased by 1, so what was the
		 * iterator to the last element of the sequence (the "back" of the container), will
		 * now be the iterator to the second-last element of the sequence; what was the
		 * "end" iterator will now be the iterator to the last element of the sequence.
		 */
		const properties_iterator
		append_property_container(
				PropertyContainer::non_null_ptr_type new_property_container,
				DummyTransactionHandle &transaction)
		{
			FeatureRevision::property_container_collection_type::size_type new_index =
					current_revision()->append_property_container(
							new_property_container, transaction);
			return properties_iterator::create_index(*this, new_index);
		}

		/**
		 * Remove the property container indicated by @a iter in the property container
		 * collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a property container-slot will become NULL.
		 */
		void
		remove_property_container(
				properties_const_iterator iter,
				DummyTransactionHandle &transaction)
		{
			current_revision()->remove_property_container(iter.index(), transaction);
		}

		/**
		 * Remove the property container indicated by @a iter in the property container
		 * collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a property container-slot will become NULL.
		 */
		void
		remove_property_container(
				properties_iterator iter,
				DummyTransactionHandle &transaction)
		{
			current_revision()->remove_property_container(iter.index(), transaction);
		}

		/**
		 * Access the current revision of this feature.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for const FeatureHandle instances; it
		 * returns a pointer to a const FeatureRevision instance.
		 */
		const FeatureRevision::non_null_ptr_to_const_type
		current_revision() const
		{
			return d_current_revision;
		}

		/**
		 * Access the current revision of this feature.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for non-const FeatureHandle instances;
		 * it returns a C++ reference to a pointer to a non-const FeatureRevision instance.
		 *
		 * Note that, because the copy-assignment operator of FeatureRevision is private,
		 * the FeatureRevision referenced by the return-value of this function cannot be
		 * assigned-to, which means that this function does not provide a means to directly
		 * switch the FeatureRevision within this FeatureHandle instance.  (This
		 * restriction is intentional.)
		 *
		 * To switch the FeatureRevision within this FeatureHandle instance, use the
		 * function @a set_current_revision below.
		 */
		const FeatureRevision::non_null_ptr_type
		current_revision()
		{
			return d_current_revision;
		}

		/**
		 * Set the current revision of this feature to @a rev.
		 *
		 * Client code should not use this function!
		 */
		void
		set_current_revision(
				FeatureRevision::non_null_ptr_type rev)
		{
			d_current_revision = rev;
		}

		/**
		 * Set the pointer to the FeatureCollectionHandle which contains this feature.
		 *
		 * Client code should not use this function!
		 *
		 * This function should only be invoked by a FeatureCollectionRevision instance
		 * when it has appended or removed a feature.  This is part of the mechanism which
		 * tracks whether a feature collection contains unsaved changes, and (later) part
		 * of the Bubble-Up mechanism.
		 */
		void
		set_feature_collection_handle_ptr(
				FeatureCollectionHandle *new_ptr)
		{
			d_feature_collection_handle_ptr = new_ptr;
		}

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const
		{
			visitor.visit_feature_handle(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		void
		accept_visitor(
				FeatureVisitor &visitor)
		{
			visitor.visit_feature_handle(*this);
		}

		/**
		 * Apply the supplied WeakObserverVisitor to all weak observers of this instance.
		 */
		void
		apply_weak_observer_visitor(
				WeakObserverVisitor<FeatureHandle> &visitor);

		/**
		 * Access the first const weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		const_weak_observer_type *&
		first_const_weak_observer() const
		{
			return d_first_const_weak_observer;
		}

		/**
		 * Access the first weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		weak_observer_type *&
		first_weak_observer()
		{
			return d_first_weak_observer;
		}

		/**
		 * Access the last const weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		const_weak_observer_type *&
		last_const_weak_observer() const
		{
			return d_last_const_weak_observer;
		}

		/**
		 * Access the last weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		weak_observer_type *&
		last_weak_observer()
		{
			return d_last_weak_observer;
		}

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		void
		increment_ref_count() const
		{
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const
		{
			return --d_ref_count;
		}

	private:

		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

		/**
		 * The current revision of this feature.
		 */
		FeatureRevision::non_null_ptr_type d_current_revision;

		/**
		 * The type of this feature.
		 */
		FeatureType d_feature_type;

		/**
		 * The unique feature ID of this feature instance.
		 */
		FeatureId d_feature_id;

		/**
		 * The first const weak observer of this instance.
		 */
		mutable const_weak_observer_type *d_first_const_weak_observer;

		/**
		 * The first weak observer of this instance.
		 */
		mutable weak_observer_type *d_first_weak_observer;

		/**
		 * The last const weak observer of this instance.
		 */
		mutable const_weak_observer_type *d_last_const_weak_observer;

		/**
		 * The last weak observer of this instance.
		 */
		mutable weak_observer_type *d_last_weak_observer;

		/**
		 * The feature collection which contains this feature.
		 *
		 * Note that this should be held via a (regular, raw) pointer rather than a
		 * ref-counting pointer (or any other type of smart pointer) because:
		 *  -# The feature collection instance conceptually manages the instance of this
		 * class, not the other way around.
		 *  -# A feature collection instance will outlive the features it contains; thus,
		 * it doesn't make sense for a feature collection to have its memory managed by its
		 * contained features.
		 *  -# Class FeatureCollectionHandle contains a ref-counting pointer to class
		 * FeatureCollectionRevision, which contains a vector which contains ref-counting
		 * pointers to class FeatureHandle, and we don't want to set up a ref-counting loop
		 * (which would lead to memory leaks).
		 */
		FeatureCollectionHandle *d_feature_collection_handle_ptr;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureHandle(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_) :
			d_ref_count(0),
			d_current_revision(FeatureRevision::create()),
			d_feature_type(feature_type_),
			d_feature_id(feature_id_),
			d_first_const_weak_observer(NULL),
			d_first_weak_observer(NULL),
			d_last_const_weak_observer(NULL),
			d_last_weak_observer(NULL),
			d_feature_collection_handle_ptr(NULL)
		{  }


		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureHandle(
				const FeatureType &feature_type_,
				const RevisionId &revision_id_) :
			d_ref_count(0),
			d_current_revision(FeatureRevision::create(revision_id_)),
			d_feature_type(feature_type_),
			d_first_const_weak_observer(NULL),
			d_first_weak_observer(NULL),
			d_last_const_weak_observer(NULL),
			d_last_weak_observer(NULL),
			d_feature_collection_handle_ptr(NULL)
		{  }


		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureHandle(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_,
				const RevisionId &revision_id_) :
			d_ref_count(0),
			d_current_revision(FeatureRevision::create(revision_id_)),
			d_feature_type(feature_type_),
			d_feature_id(feature_id_),
			d_first_const_weak_observer(NULL),
			d_first_weak_observer(NULL),
			d_last_const_weak_observer(NULL),
			d_last_weak_observer(NULL),
			d_feature_collection_handle_ptr(NULL)
		{  }


		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new non_null_intrusive_ptr reference to
		 * the new duplicate.  Since initially the only reference to the new duplicate will
		 * be the one returned by the 'clone' function, *before* the new intrusive-pointer
		 * is created, the ref-count of the new FeatureHandle instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureHandle(
				const FeatureHandle &other) :
			d_ref_count(0),
			d_current_revision(other.d_current_revision),
			d_feature_type(other.d_feature_type),
			d_feature_id(other.d_feature_id),
			d_first_const_weak_observer(NULL),
			d_first_weak_observer(NULL),
			d_last_const_weak_observer(NULL),
			d_last_weak_observer(NULL),
			d_feature_collection_handle_ptr(NULL)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive-pointer to another.
		FeatureHandle &
		operator=(
				const FeatureHandle &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureHandle *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureHandle *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}


	/**
	 * Get the first weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This function mimics the Boost
	 * intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	inline
	WeakObserver<const FeatureHandle> *&
	weak_observer_get_first(
			const FeatureHandle *publisher_ptr,
			const WeakObserver<const FeatureHandle> *)
	{
		return publisher_ptr->first_const_weak_observer();
	}


	/**
	 * Get the last weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This style of function mimics the
	 * Boost intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	inline
	WeakObserver<const FeatureHandle> *&
	weak_observer_get_last(
			const FeatureHandle *publisher_ptr,
			const WeakObserver<const FeatureHandle> *)
	{
		return publisher_ptr->last_const_weak_observer();
	}


	/**
	 * Get the first weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This function mimics the Boost
	 * intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	inline
	WeakObserver<FeatureHandle> *&
	weak_observer_get_first(
			FeatureHandle *publisher_ptr,
			const WeakObserver<FeatureHandle> *)
	{
		return publisher_ptr->first_weak_observer();
	}


	/**
	 * Get the last weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This style of function mimics the
	 * Boost intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	inline
	WeakObserver<FeatureHandle> *&
	weak_observer_get_last(
			FeatureHandle *publisher_ptr,
			const WeakObserver<FeatureHandle> *)
	{
		return publisher_ptr->last_weak_observer();
	}
}

#endif  // GPLATES_MODEL_FEATUREHANDLE_H
