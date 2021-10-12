/* $Id$ */

/**
 * \file 
 * Contains the definition of the class WeakReference.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_WEAKREFERENCE_H
#define GPLATES_MODEL_WEAKREFERENCE_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "HandleTraits.h"
#include "WeakObserver.h"
#include "WeakObserverVisitor.h"
#include "WeakReferenceCallback.h"

#include "utils/SafeBool.h"


namespace GPlatesModel
{
	/**
	 * This class is a weak-reference to a handle.
	 *
	 * To quote the article on weak-references in Wikipedia, the free encyclopedia:
	 *
	 * @par
	 * In computer programming, a weak reference is a reference that does not protect the
	 * referent object from collection by a garbage collector. An object referenced only by
	 * weak references is considered unreachable (or "weakly reachable") and so may be
	 * collected at any time. Weak references are used to prevent circular references and to
	 * avoid keeping in memory referenced but unneeded objects.
	 *
	 * To quote the section Weak References on the home-page for mxProxy: a Generic Proxy
	 * Wrapper Type for Python:
	 *
	 * @par
	 * Weak references are called weak because they don't keep the object alive by incrementing
	 * the reference count on the referenced object.
	 *
	 * GPlates uses "weak-references" as "smart pointers" which enable objects to be referenced
	 * without keeping the objects alive. Specifically, weak-references are used to implement
	 * references in the Application-Logic tier to objects inside the Model.
	 *
	 * Why don't we want to keep certain objects alive?  Because certain objects (such as
	 * features and feature collections) should be deallocated at certain times:
	 *  -# for logical reasons (i.e., when the object should no longer be available -- recall
	 * that the feature ID in a feature is automatically registered when the feature is created
	 * (so that it's possible to access the feature which defines that feature ID) for the
	 * entire duration of the feature's lifetime; if a feature is not deallocated, its feature
	 * ID will remain registered for that feature, which may lead to multiple registrations of
	 * the same feature ID, but pointing to different feature instances)
	 *  -# for memory-releasing reasons
	 *
	 * Hence, we don't want the references in the Application-Logic tier to override the
	 * careful and specific lifetime-control of objects in the Model.  For this reason, we
	 * don't want the references in the Application-Logic tier to increment the ref-counts of
	 * the referenced objects.
	 *
	 * But OTOH, we also don't want to end up with dangling pointers (yuck!) when features and
	 * feature collections are deallocated but there are still references which point to them.
	 * Hence, we use "weak-references", which don't keep the referenced objects alive, but *do*
	 * know when the referenced objects are deallocated.
	 *
	 * @par Validity for dereference
	 * The member function @a is_valid is used to determine whether a weak-reference is valid
	 * to be dereferenced.
	 *
	 * @par Important
	 * @strong Always check that the weak-ref @a is_valid before every dereference operation!
	 */
	template<typename H>
	class WeakReference:
			public WeakObserver<H>,
			public GPlatesUtils::SafeBool<WeakReference<H> >
	{

	public:

		/**
		 * This is the type of the handle.
		 *
		 * (For example, 'FeatureCollectionHandle' or 'const FeatureCollectionHandle'.)
		 */
		typedef H handle_type;

		/**
		 * Default constructor.
		 *
		 * Weak reference instances which are initialised using the default constructor are
		 * not valid to be dereferenced.
		 *
		 * The pointer to the handle will be NULL.
		 */
		WeakReference()
		{  }

		/**
		 * Construct a reference to @a handle.
		 *
		 * This constructor will not throw.
		 *
		 * The pointer to the handle will be non-NULL, and the instance will be valid for
		 * dereferencing.
		 */
		explicit
		WeakReference(
				handle_type &handle) :
			WeakObserver<H>(handle)
		{  }

		virtual
		~WeakReference()
		{  }

		/**
		 * Converts this WeakReference<H> into a WeakReference<const H>, where H is
		 * non-const. If H is const, this will never be called.
		 *
		 * Note: If a callback is attached to this WeakReference<H>, it is not copied
		 * over to the WeakReference<const H> returned.
		 */
		operator WeakReference<const H>() const
		{
			if (WeakObserver<H>::publisher_ptr())
			{
				return WeakReference<const H>(*WeakObserver<H>::publisher_ptr());
			}
			else
			{
				return WeakReference<const H>();
			}
		}

		/**
		 * Return the pointer to the handle.
		 *
		 * The pointer returned will be NULL if this instance does not reference a handle;
		 * non-NULL otherwise.
		 *
		 * This function will not throw.
		 *
		 * Note that we return a non-const instance of handle_type from a const member
		 * function:  handle_type may already be "const", in which case, the "const" would
		 * be redundant; OTOH, if handle_type *doesn't* include "const", an instance of
		 * this class should behave like an STL iterator (or a pointer) rather than an STL
		 * const-iterator.  We're simply declaring the member function "const" to ensure
		 * that it may be invoked on const instances too.
		 */
		handle_type *
		handle_ptr() const
		{
			return WeakObserver<H>::publisher_ptr();
		}

		/**
		 * Return whether this pointer is valid to be dereferenced, and also whether
		 * the handle is active (i.e. not conceptually deleted).
		 *
		 * You should @strong always check this before @em ever dereferencing a weak-ref.
		 * Do not dereference the weak-ref if this function returns false.
		 *
		 * This function will not throw.
		 */
		bool
		is_valid() const
		{
			return handle_ptr() != NULL && handle_ptr()->is_active();
		}

		/**
		 * Return whether this pointer is valid to be deferenced. This is equivalent
		 * to calling is_valid() on the WeakReference.
		 *
		 * This function will not throw.
		 *
		 * This function is provided for the benefit of the SafeBool base class, which
		 * provides operator bool().
		 */
		bool
		boolean_test() const
		{
			return is_valid();
		}

		/**
		 * Copy-assignment operator.
		 *
		 * The effect of this operation is that this instance will reference the handle
		 * which is referenced by @a other (if any).
		 *
		 * The callback (if any) registered with the @a other handle is copied across
		 * to this instance.
		 *
		 * This function will not throw.
		 */
		WeakReference &
		operator=(
				const WeakReference &other)
		{
			WeakObserver<H>::operator=(other);
			d_callback = other.d_callback;
			return *this;
		}

		/**
		 * Return whether this weak-reference references @a that_handle.
		 *
		 * This function will not throw.
		 */
		bool
		references(
				const handle_type &that_handle) const
		{
			return (handle_ptr() == &that_handle);
		}

		/**
		 * Return whether this instance is equal to @a other.
		 *
		 * This instance will be considered equal to @a other if both instances reference
		 * the same handle, or if neither instance references any handle; the instances
		 * will be considered unequal otherwise.
		 *
		 * This function will not throw.
		 */
		bool
		operator==(
				const WeakReference &other) const
		{
			return (handle_ptr() == other.handle_ptr());
		}

		/**
		 * Return whether this instance is not equal to @a other.
		 *
		 * This instance will be considered equal to @a other if both instances reference
		 * the same handle, or if neither instance references any handle; the instances
		 * will be considered unequal otherwise.
		 *
		 * This function will not throw.
		 */
		bool
		operator!=(
				const WeakReference &other) const
		{
			return (handle_ptr() != other.handle_ptr());
		}

		/**
		 * The dereference operator.
		 *
		 * This operator should only be invoked when the reference is valid (i.e., when
		 * @a is_valid would return true).
		 *
		 * Note that we return a non-const instance of handle_type from a const member
		 * function:  handle_type may already be "const", in which case, the "const" would
		 * be redundant; OTOH, if handle_type *doesn't* include "const", an instance of
		 * this class should behave like an STL iterator (or a pointer) rather than an STL
		 * const-iterator.  We're simply declaring the member function "const" to ensure
		 * that it may be invoked on const instances too.
		 */
		handle_type &
		operator*() const
		{
			return *handle_ptr();
		}

		/**
		 * The pointer-indirection-member-access operator.
		 *
		 * This operator should only be invoked when the reference is valid (i.e., when
		 * @a is_valid would return true).
		 *
		 * Note that we return a non-const instance of handle_type from a const member
		 * function:  handle_type may already be "const", in which case, the "const" would
		 * be redundant; OTOH, if handle_type *doesn't* include "const", an instance of
		 * this class should behave like an STL iterator (or a pointer) rather than an STL
		 * const-iterator.  We're simply declaring the member function "const" to ensure
		 * that it may be invoked on const instances too.
		 */
		handle_type *
		operator->() const
		{
			return handle_ptr();
		}

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				WeakObserverVisitor<H> &visitor)
		{
			visitor.visit_weak_reference(*this);
		}

		/**
		 * Attach a callback to this WeakReference. The callback will be notified
		 * when the WeakReference's publisher is modified or about to be deleted.
		 */
		void
		attach_callback(
				typename WeakReferenceCallback<H>::maybe_null_ptr_type callback_) const
		{
			d_callback = callback_;
		}

		/**
		 * Gets the callback attached to this WeakReference, if any.
		 */
		typename WeakReferenceCallback<H>::maybe_null_ptr_type
		callback() const
		{
			return d_callback;
		}

		/**
		 * Unattaches the callback, if any, from this WeakReference.
		 */
		void
		unattach_callback() const
		{
			d_callback = WeakReferenceCallback<H>::maybe_null_ptr_type();
		}

		/**
		 * Notify the callback that the publisher has been modified.
		 *
		 * Used by WeakReferencePublisherModifiedVisitor.
		 */
		void
		publisher_modified(
				typename WeakReferencePublisherModifiedEvent<H>::Type type) const
		{
			if (d_callback)
			{
				d_callback->publisher_modified(
						WeakReferencePublisherModifiedEvent<H>(*this, type));
			}
		}

		/**
		 * Notify the callback that the publisher has added new children.
		 *
		 * Used by WeakReferencePublisherAddedVisitor.
		 */
		void
		publisher_added(
				const typename WeakReferencePublisherAddedEvent<H>::new_children_container_type &new_children) const
		{
			if (d_callback)
			{
				d_callback->publisher_added(
						WeakReferencePublisherAddedEvent<H>(*this, new_children));
			}
		}

		/**
		 * Notify the callback that the publisher has been deactivated (conceptually deleted).
		 *
		 * Used by WeakReferencePublisherDeactivatedVisitor.
		 */
		void
		publisher_deactivated() const
		{
			if (d_callback)
			{
				d_callback->publisher_deactivated(
						WeakReferencePublisherDeactivatedEvent<H>(*this));
			}
		}

		/**
		 * Notify the callback that the publisher has been reactivated (conceptually undeleted).
		 *
		 * Used by WeakReferencePublisherDeactivatedVisitor.
		 */
		void
		publisher_reactivated() const
		{
			if (d_callback)
			{
				d_callback->publisher_reactivated(
						WeakReferencePublisherReactivatedEvent<H>(*this));
			}
		}

		/**
		 * Notify the callback that the publisher is about to be destroyed (in the C++ sense).
		 *
		 * Used by WeakReferencePublisherAboutToBeDestroyedVisitor.
		 */
		void
		publisher_about_to_be_destroyed() const
		{
			if (d_callback)
			{
				d_callback->publisher_about_to_be_destroyed(
						WeakReferencePublisherAboutToBeDestroyedEvent<H>(*this));
			}
		}

		bool
		operator<(
				const WeakReference<H> &other) const
		{
			return handle_ptr() < other.handle_ptr();
		}

	private:

		/**
		 * An optional callback to use when publisher is modified or about to be deleted.
		 *
		 * Note that it's mutable because we want to be able to attach a callback to
		 * a WeakReference that is being used as a key in a std::map. This is fine
		 * because the callback is not used to compare two WeakReferences.
		 */
		mutable typename WeakReferenceCallback<H>::maybe_null_ptr_type d_callback;

	};

}

#endif  // GPLATES_MODEL_WEAKREFERENCE_H
