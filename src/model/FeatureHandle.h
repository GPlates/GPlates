/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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
#include "HandleContainerIterator.h"
#include "contrib/non_null_intrusive_ptr.h"

namespace GPlatesModel
{
	class DummyTransactionHandle;

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
		 * A convenience typedef for GPlatesContrib::non_null_intrusive_ptr<FeatureHandle>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<FeatureHandle> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<const FeatureHandle>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<const FeatureHandle>
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
		 * This typedef is used by the HandleContainerIterator.
		 */
		typedef FeatureRevision revision_component_type;

		/**
		 * The type used for const-iterating over the collection of property containers.
		 */
		typedef HandleContainerIterator<const FeatureHandle,
				const revision_component_type::property_container_collection_type,
				boost::intrusive_ptr<const PropertyContainer> > const_iterator;

		/**
		 * The type used for (non-const) iterating over the collection of property
		 * containers.
		 */
		typedef HandleContainerIterator<FeatureHandle,
				revision_component_type::property_container_collection_type,
				boost::intrusive_ptr<PropertyContainer> > iterator;

		/**
		 * Translate the non-const iterator @a iter to the equivalent const-iterator.
		 */
		static
		const const_iterator
		get_const_iterator(
				iterator iter)
		{
			return const_iterator(*(iter.d_collection_handle_ptr), iter.d_index);
		}

		~FeatureHandle()
		{  }

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
			non_null_ptr_type ptr(*(new FeatureHandle(feature_type_, feature_id_)));
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
			non_null_ptr_type dup(*(new FeatureHandle(*this)));
			return dup;
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
		const const_iterator
		properties_begin() const
		{
			return const_iterator::create_begin(*this);
		}

		/**
		 * Return the "begin" iterator to iterate over the collection of property
		 * containers.
		 */
		const iterator
		properties_begin()
		{
			return iterator::create_begin(*this);
		}

		/**
		 * Return the "end" const-iterator used during iteration over the collection of
		 * property containers.
		 */
		const const_iterator
		properties_end() const
		{
			return const_iterator::create_end(*this);
		}

		/**
		 * Return the "end" iterator used during iteration over the collection of property
		 * containers.
		 */
		const iterator
		properties_end()
		{
			return iterator::create_end(*this);
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
		const iterator
		append_property_container(
				PropertyContainer::non_null_ptr_type new_property_container,
				DummyTransactionHandle &transaction)
		{
			FeatureRevision::property_container_collection_type::size_type new_index =
					current_revision()->append_property_container(
							new_property_container, transaction);
			return iterator(*this, new_index);
		}

		/**
		 * Remove the property container indicated by @a iter in the property container
		 * collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a feature-slot will become NULL.
		 */
		void
		remove_property_container(
				const_iterator iter,
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
				iterator iter,
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
		 */
		void
		set_current_revision(
				FeatureRevision::non_null_ptr_type rev)
		{
			d_current_revision = rev;
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
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
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
		 * GPlatesContrib::non_null_intrusive_ptr.
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
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureHandle(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_) :
			d_ref_count(0),
			d_current_revision(FeatureRevision::create()),
			d_feature_type(feature_type_),
			d_feature_id(feature_id_)
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
			d_feature_id(other.d_feature_id)
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

}

#endif  // GPLATES_MODEL_FEATUREHANDLE_H
