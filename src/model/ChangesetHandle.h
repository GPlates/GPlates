/* $Id$ */

/**
 * \file 
 * Contains the definition of the class ChangesetHandle.
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

#ifndef GPLATES_MODEL_CHANGESETHANDLE_H
#define GPLATES_MODEL_CHANGESETHANDLE_H

#include <string>
#include <set>
#include <boost/noncopyable.hpp>


namespace GPlatesModel
{
	class Model;

	/**
	 * ChangesetHandle allows model client code to group model transactions
	 * togther into one logical changeset.
	 *
	 * A model transaction is an atomic operation (such as the addition of one
	 * feature into a feature collection, or the changing of one property in a
	 * feature). However, these transactions may well be too fine-grained to be
	 * presented to the user as undoable operations. By grouping a set of model
	 * transactions into one changeset, the user is able to undo the changes made
	 * to the model in that changeset, unawares that the changeset is actually
	 * composed of many smaller atomic transactions.
	 *
	 * If the model transaction presents the correct level of granularity for
	 * user-undoable operations, then there is no need to use ChangesetHandle as
	 * the methods in *Handle that modify the state of the model generate an
	 * implicit ChangesetHandle.
	 *
	 * Client code uses ChangesetHandle in an RAII manner. From the construction
	 * of one instance to the destruction of that instance, all model transactions
	 * are automatically associated with that ChangesetHandle.
	 *
	 * ChangesetHandles can be nested. In such a case, only the outermost
	 * ChangesetHandle is operative. For example:
	 * 
	 *		FeatureHandle::non_null_ptr_type feature = ...;
	 *		FeatureCollectionHandle::weak_ref feature_collection = ...;
	 *
	 *		void f()
	 *		{
	 *			ChangesetHandle changeset(feature_collection->model_ptr(), "Adding a feature");
	 *			feature_collection->add(feature->deep_clone());
	 *		}
	 *
	 *		void g()
	 *		{
	 *			ChangesetHandle changeset(feature_collection->model_ptr(), "Adding two features");
	 *			f();
	 *			f();
	 *		}
	 *
	 * In the above example, assuming there is no ChangesetHandle active when g()
	 * is called, the (only) changeset recorded is "Adding two features".
	 *
	 * Note: Currently ChangesetHandle does nothing useful.
	 */
	class ChangesetHandle :
			public boost::noncopyable
	{
	
	public:
		
		/**
		 * Constructs a ChangesetHandle that will construct a changeset belonging to
		 * @a model upon destruction.
		 *
		 * @a model may be NULL. In that case, this ChangesetHandle has no effect.
		 *
		 * The @a description is used in the user interface.
		 */
		ChangesetHandle(
				Model *model_ptr,
				const std::string &description_ = std::string());

		/**
		 * Destructor.
		 */
		~ChangesetHandle();

		/**
		 * Returns the human-readable description of the changeset.
		 */
		const std::string &
		description() const;

		/**
		 * Registers @a handle as having been modified or added in this changeset.
		 */
		void
		add_handle(
				const void *handle);

		/**
		 * Returns true if @a handle has already been registered in this changeset.
		 */
		bool
		has_handle(
				const void *handle);

	private:

		Model *d_model_ptr;
		std::string d_description;

		/**
		 * This is a collection of raw pointers to Handles that have been modified or
		 * added in this changeset.
		 */
		std::set<const void *> d_modified_handles;

	};
}

#endif  // GPLATES_MODEL_CHANGESETHANDLE_H
