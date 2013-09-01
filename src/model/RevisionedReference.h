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

#ifndef GPLATES_MODEL_REVISIONEDREFERENCE_H
#define GPLATES_MODEL_REVISIONEDREFERENCE_H

#include <boost/optional.hpp>

#include "ModelTransaction.h"
#include "Revision.h"
#include "Revisionable.h"
#include "RevisionContext.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesModel
{
	namespace Implementation
	{
		/**
		 * Non-template implementation for a reference to a revisionable object and one of its revision snapshots.
		 *
		 * Avoids instantiating a class for every derived revisionable type.
		 */
		class RevisionedReference
		{
		public:

			~RevisionedReference();

			static
			RevisionedReference
			attach(
					ModelTransaction &transaction,
					RevisionContext &revision_context,
					const Revisionable::non_null_ptr_type &revisionable);

			boost::optional<RevisionContext &>
			detach(
					ModelTransaction &transaction);

			void
			change(
					ModelTransaction &transaction,
					const Revisionable::non_null_ptr_type &revisionable);

			Revision::non_null_ptr_type
			clone_revision(
					ModelTransaction &transaction);

			void
			clone(
					RevisionContext &revision_context);

			Revisionable::non_null_ptr_type
			get_revisionable() const
			{
				return d_revisionable;
			}

		private:

			/**
			 * Helper class to manage revision reference counts.
			 */
			class RevisionRef
			{
			public:

				explicit
				RevisionRef(
						const Revision::non_null_ptr_to_const_type &revision) :
					d_revision(revision)
				{
					++d_revision->d_revision_reference_ref_count;
				}

				~RevisionRef()
				{
					--d_revision->d_revision_reference_ref_count;
				}

				//! Copy constructor.
				RevisionRef(
						const RevisionRef &other) :
					d_revision(other.d_revision)
				{
					++d_revision->d_revision_reference_ref_count;
				}

				//! Copy assignment operator.
				RevisionRef &
				operator=(
						RevisionRef other)
				{
					// Copy-and-swap idiom.
					swap(d_revision, other.d_revision);
					return *this;
				}

				//! Assignment operator.
				RevisionRef &
				operator=(
						const Revision::non_null_ptr_to_const_type &revision)
				{
					--d_revision->d_revision_reference_ref_count;
					d_revision = revision;
					++d_revision->d_revision_reference_ref_count;
					return *this;
				}

				Revision::non_null_ptr_to_const_type
				get_revision() const
				{
					return d_revision;
				}

			private:

				Revision::non_null_ptr_to_const_type d_revision;
			};


			explicit
			RevisionedReference(
					const Revisionable::non_null_ptr_type &revisionable,
					const Revision::non_null_ptr_to_const_type &revision) :
				d_revisionable(revisionable),
				d_revision(revision)
			{  }


			Revisionable::non_null_ptr_type d_revisionable;
			RevisionRef d_revision;
		};
	}


	/**
	 * Reference to a revisionable object and one of its revision snapshots.
	 *
	 * Note that the revision is not the current revision of the revisionable object until the
	 * associated @a ModelTransaction has been committed.
	 *
	 * Template parameter 'RevisionableType' is Revisionable or one of its derived types and
	 * can be const or non-const (eg, 'GpmlPlateId' or 'const GpmlPlateId').
	 */
	template <class RevisionableType>
	class RevisionedReference
	{
	public:

		/**
		 * Creates a revisioned reference by attaching the specified revisionable to the
		 * specific revision context.
		 */
		static
		RevisionedReference
		attach(
				ModelTransaction &transaction,
				RevisionContext &revision_context,
				const GPlatesUtils::non_null_intrusive_ptr<RevisionableType> &revisionable)
		{
			return RevisionedReference(
					Implementation::RevisionedReference::attach(transaction, revision_context, revisionable));
		}


		/**
		 * Detaches the current revisionable object from its revision context (leaving it without a context).
		 *
		 * Returns the detached revision context if there was one.
		 */
		boost::optional<RevisionContext &>
		detach(
				ModelTransaction &transaction)
		{
			return d_impl.detach(transaction);
		}


		/**
		 * Changes the revisionable object.
		 *
		 * This detaches the current revisionable object and attaches the specified revisionable object.
		 * And the revision context, if any, is transferred.
		 */
		void
		change(
				ModelTransaction &transaction,
				const GPlatesUtils::non_null_intrusive_ptr<RevisionableType> &revisionable)
		{
			d_impl.change(transaction, revisionable);
		}


		/**
		 * Makes 'this' revision reference a shallow copy of the current revisionable object.
		 *
		 * Essentially clones the revisionable object's revision (which does not recursively
		 * copy nested revisionable objects).
		 *
		 * Also returns the cloned revision as a modifiable (non-const) object.
		 */
		Revision::non_null_ptr_type
		clone_revision(
				ModelTransaction &transaction)
		{
			return d_impl.clone_revision(transaction);
		}


		/**
		 * Makes 'this' revision reference a deep copy of the current revisionable object.
		 *
		 * This recursively clones the revisionable object and its revision
		 * (including nested revisionable objects and their revisions).
		 */
		void
		clone(
				RevisionContext &revision_context)
		{
			d_impl.clone(revision_context);
		}


		/**
		 * Returns the revisionable object.
		 */
		GPlatesUtils::non_null_intrusive_ptr<RevisionableType>
		get_revisionable() const
		{
			return GPlatesUtils::dynamic_pointer_cast<RevisionableType>(d_impl.get_revisionable());
		}


		//
		// NOTE: We don't return the revisionable object 'revision' (const or non-const).
		// Since revisions are immutable, @a clone_revision should be used when a
		// revisionable object is to be modified.
		//

	private:

		explicit
		RevisionedReference(
				const Implementation::RevisionedReference &impl) :
			d_impl(impl)
		{  }


		Implementation::RevisionedReference d_impl;
	};
}

#endif // GPLATES_MODEL_REVISIONEDREFERENCE_H
