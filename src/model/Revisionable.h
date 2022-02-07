/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_REVISIONABLE_H
#define GPLATES_MODEL_REVISIONABLE_H

#include <boost/optional.hpp>
#include <boost/operators.hpp>

#include "Revision.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	// Forward declarations.
	class Model;
	class ModelTransaction;
	class BubbleUpRevisionHandler;
	class RevisionContext;
	namespace Implementation
	{
		class RevisionedReference;
	}


	/**
	 * This class is the abstract base of all revisionable model entities.
	 */
	class Revisionable :
			public GPlatesUtils::ReferenceCount<Revisionable>,
			public boost::equality_comparable<Revisionable>
	{
	public:

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<Revisionable>.
		typedef GPlatesUtils::non_null_intrusive_ptr<Revisionable> non_null_ptr_type;

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const Revisionable>.
		typedef GPlatesUtils::non_null_intrusive_ptr<const Revisionable> non_null_ptr_to_const_type;


		virtual
		~Revisionable()
		{  }

		/**
		 * Create a duplicate of this Revisionable instance, including a recursive copy
		 * of any Revisionable objects this instance might contain.
		 */
		const non_null_ptr_type
		clone() const
		{
			return clone_impl();
		}

		/**
		 * Returns a (non-const) reference to the Model to which this revisionable object belongs.
		 *
		 * Returns none if this revisionable object is not currently attached to the model.
		 * This can happen, for example, if a property value has no parent (eg, top-level property)
		 * or if the parent has no parent, etc.
		 */
		boost::optional<Model &>
		get_model();

		/**
		 * Returns a const reference to the Model to which this revisionable object belongs.
		 *
		 * Returns none if this revisionable object is not currently attached to the model.
		 * This can happen, for example, if a property value has no parent (eg, top-level property)
		 * or if the parent has no parent, etc.
		 */
		boost::optional<const Model &>
		get_model() const;

		/**
		 * Value equality comparison operator.
		 *
		 * Returns false if the types of @a other and 'this' aren't the same type, otherwise
		 * returns true if their values (tested recursively as needed) compare equal.
		 *
		 * Inequality provided by boost equality_comparable.
		 */
		bool
		operator==(
				const Revisionable &other) const;

	protected:

		/**
		 * Construct a Revisionable instance.
		 */
		explicit
		Revisionable(
				const Revision::non_null_ptr_to_const_type &revision) :
			d_current_revision(revision)
		{  }

		/**
		 * Returns the current immutable revision as the base revision type.
		 *
		 * Revisions are immutable - use @a BubbleUpRevisionHandler to modify revisions.
		 */
		Revision::non_null_ptr_to_const_type
		get_current_revision() const
		{
			return d_current_revision;
		}

		/**
		 * Returns the current immutable revision as the specified derived revision type.
		 *
		 * Revisions are immutable - use @a BubbleUpRevisionHandler to modify revisions.
		 */
		template <class RevisionType>
		const RevisionType &
		get_current_revision() const
		{
			return dynamic_cast<const RevisionType &>(*d_current_revision);
		}

		/**
		 * Create a new bubble-up revision by delegating to the (parent) revision context
		 * if there is one, otherwise create a new revision without any context.
		 */
		Revision::non_null_ptr_type
		create_bubble_up_revision(
				ModelTransaction &transaction) const;

		/**
		 * Same as the other overload of @a create_bubble_up_revision but downcasts to
		 * the specified derived revision type.
		 */
		template <class RevisionType>
		RevisionType &
		create_bubble_up_revision(
				ModelTransaction &transaction) const
		{
			// The returned revision is kept alive by either the model transaction (if uncommitted),
			// or 'this' revisionable object (if committed).
			return dynamic_cast<RevisionType &>(*create_bubble_up_revision(transaction));
		}

		/**
		 * Create a duplicate of this Revisionable instance, including a recursive copy
		 * of any revisionable objects this instance might contain.
		 *
		 * @a context is non-null if this revisionable object is nested within a parent context.
		 */
		virtual
		const non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const = 0;

		/**
		 * Determine if two Revisionable instances ('this' and 'other') value compare equal.
		 *
		 * This should recursively test for equality as needed.
		 * Note that the revision testing is done by Revisionable::equality(), since the revisions
		 * are contained in class Revisionable, so derived revisionable classes only need to test
		 * any non-revisioned data that they may contain - and if there is none then this method
		 * does not need to be implemented by that derived revisionable class.
		 *
		 * A precondition of this method is that the type of 'this' is the same as the type of 'object'
		 * so static_cast can be used instead of dynamic_cast.
		 */
		virtual
		bool
		equality(
				const Revisionable &other) const;

	private:

		/**
		 * NOTE: Copy-constructor is intentionally not defined anywhere (not strictly necessary to do
		 * this since base class ReferenceCount is already non-copyable, but makes it more obvious).
		 *
		 * Use the constructor (accepting revision) when cloning a revisionable object.
		 * Note that two Revisionable instances should not point to the same Revision instance as
		 * this will prevent the revisioning system from functioning correctly - this doesn't mean
		 * that two Revision instances can't reference the same (nested) Revision instance though.
		 * Essentially this highlights the difference between full cloning and revision cloning.
		 */
		Revisionable(
				const Revisionable &other);

		/**
		 * The current revision of this revisionable object.
		 *
		 * The current revision is immutable since it has already been initialised and once
		 * initialised it cannot be modified. A modification involves creating a new revision object.
		 * Keeping the current revision 'const' prevents inadvertent modifications by derived
		 * revisionable classes.
		 *
		 * The revision also contains the current parent reference such that when a different
		 * revision is swapped in (due to undo/redo) it will automatically reference the correct parent.
		 *
		 * The pointer is declared 'mutable' so that revisions can be swapped in 'const' property values.
		 */
		mutable Revision::non_null_ptr_to_const_type d_current_revision;


		friend class ModelTransaction;
		friend class BubbleUpRevisionHandler;

		friend class Implementation::RevisionedReference;
	};
}

#endif // GPLATES_MODEL_REVISIONABLE_H
