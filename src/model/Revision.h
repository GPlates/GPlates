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

#ifndef GPLATES_MODEL_REVISION_H
#define GPLATES_MODEL_REVISION_H

#include <boost/optional.hpp>

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	// Forward declarations.
	class RevisionContext;
	namespace Implementation
	{
		class RevisionedReference;
	}


	/**
	 * Base revision class inherited by derived revision classes where mutable/revisionable
	 * state is stored so it can be revisioned.
	 */
	class Revision :
			public GPlatesUtils::ReferenceCount<Revision>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<Revision> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Revision> non_null_ptr_to_const_type;


		virtual
		~Revision()
		{  }


		/**
		 * A shallow clone that deep copies everything except nested revision references.
		 *
		 * @a context is the optional (parent) context within which this revision is nested.
		 * A revision that is not attached to a parent has no context.
		 *
		 * Since revisionable objects nested within this revision are already revisioned
		 * we don't need to deep copy them. In other words, for example, two parent property value
		 * revisions can share the same nested property value revision.
		 */
		virtual
		non_null_ptr_type
		clone_revision(
				boost::optional<RevisionContext &> context = boost::none) const = 0;


		/**
		 * Determine if two Revision instances ('this' and 'other') value compare equal.
		 *
		 * This should recursively test for equality as needed.
		 *
		 * A precondition of this method is that the type of 'this' is the same as the type of 'other'.
		 */
		virtual
		bool
		equality(
				const Revision &other) const
		{
			return true; // Terminates derived-to-base recursion.
		}


		/**
		 * Returns the (parent) context of this revision, if any.
		 *
		 * Note: There's no set method since it should not be possible to alter the context
		 * after a revision has been created.
		 */
		boost::optional<RevisionContext &>
		get_context() const
		{
			return d_context;
		}

	protected:

		/**
		 * Constructor specified optional (parent) context in which this revision is nested.
		 */
		explicit
		Revision(
				boost::optional<RevisionContext &> context = boost::none) :
			d_context(context),
			d_revision_reference_ref_count(0)
		{  }

	private:

		/**
		 * NOTE: Copy-constructor is intentionally not defined anywhere (not strictly necessary to do
		 * this since base class ReferenceCount is already non-copyable, but makes it more obvious).
		 *
		 * Use the constructor (accepting revision context) when cloning a revision.
		 */
		Revision(
				const Revision &other);


		/**
		 * The bubble up callback to the parent revisionable object, if any, that is called just
		 * prior to making a modification to 'this' revisionable object.
		 */
		boost::optional<RevisionContext &> d_context;


		/**
		 * The reference-count of this instance used by @a RevisionedReference.
		 *
		 * This is used to detach 'this' revision from its revision context when
		 * the last @a RevisionedReference referencing 'this' is destroyed.
		 */
		mutable int d_revision_reference_ref_count;

		friend class Implementation::RevisionedReference;
	};
}

#endif // GPLATES_MODEL_REVISION_H
