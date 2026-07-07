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

#include "RevisionedReference.h"


GPlatesModel::Implementation::RevisionedReference::~RevisionedReference()
{
	// Since this is a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		// If we're the last revisioned reference that references the revision...
		// Note that this doesn't necessarily mean the revision is about to be destroyed because
		// the revisionable object might currently be referencing it.
		//
		// NOTE: We test for '1' instead of '0' since our sub-objects have not yet been destroyed
		// and hence the 'd_revision' destructor has not yet decremented the reference count.
		if (d_revision.get_revision()->d_revision_reference_ref_count == 1)
		{
			// If the revisionable object is currently referencing the revision then detach it
			// by creating a revision with no context and setting that on the revisionable object.
			// This ensures that if the parent revisionable object (context) is destroyed then the
			// revisionable object (revision) isn't left with a dangling reference back up to it.
			// Note that the client still should call 'detach' (or 'change') when they remove
			// a child revisionable object from a parent revisionable object so
			// that the child revisionable object can then be attached to a different parent
			// revisionable object.
			// So these are two different things and both are needed.
			if (d_revisionable->d_current_revision == d_revision.get_revision())
			{
				// Normally this is done as a model transaction but we do it directly here
				// since we're in a destructor where the client has no opportunity to
				// create a model transaction object and pass it to us.
				d_revisionable->d_current_revision = d_revision.get_revision()->clone_revision();
			}
		}
	}
	catch (...)
	{
	}
}


GPlatesModel::Implementation::RevisionedReference
GPlatesModel::Implementation::RevisionedReference::attach(
		ModelTransaction &transaction,
		RevisionContext &revision_context,
		const Revisionable::non_null_ptr_type &revisionable)
{
	// Clone the revisionable object's current revision but attach to the new revision context.
	RevisionedReference revisioned_reference(
			revisionable,
			revisionable->d_current_revision->clone_revision(revision_context));

	transaction.add_revision_transaction(
			ModelTransaction::RevisionTransaction(
					revisioned_reference.d_revisionable,
					revisioned_reference.d_revision.get_revision()));

	return revisioned_reference;
}


boost::optional<GPlatesModel::RevisionContext &>
GPlatesModel::Implementation::RevisionedReference::detach(
		ModelTransaction &transaction)
{
	boost::optional<RevisionContext &> revision_context =
			d_revision.get_revision()->get_context();

	// Detach the current revisionable object by creating a revision with no context.
	d_revision = d_revision.get_revision()->clone_revision();

	transaction.add_revision_transaction(
			ModelTransaction::RevisionTransaction(d_revisionable, d_revision.get_revision()));

	return revision_context;
}


void
GPlatesModel::Implementation::RevisionedReference::change(
		ModelTransaction &transaction,
		const Revisionable::non_null_ptr_type &revisionable)
{
	boost::optional<RevisionContext &> revision_context =
			d_revision.get_revision()->get_context();

	// Detach the current revisionable object by creating a revision with no context.
	transaction.add_revision_transaction(
			ModelTransaction::RevisionTransaction(
					d_revisionable,
					d_revision.get_revision()->clone_revision()));

	// Attach the new revisionable object by creating a revision with the detached context.
	d_revisionable = revisionable;
	d_revision = d_revisionable->d_current_revision->clone_revision(revision_context);
	transaction.add_revision_transaction(
			ModelTransaction::RevisionTransaction(d_revisionable, d_revision.get_revision()));
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesModel::Implementation::RevisionedReference::clone_revision(
		ModelTransaction &transaction)
{
	boost::optional<RevisionContext &> revision_context =
			d_revision.get_revision()->get_context();

	// The cloned revision's context is the same as the original revision.
	// Essentially this means the parent revisionable object is the same for both revisions.
	Revision::non_null_ptr_type mutable_revision =
			d_revision.get_revision()->clone_revision(revision_context);
	d_revision = mutable_revision;

	transaction.add_revision_transaction(
			ModelTransaction::RevisionTransaction(d_revisionable, d_revision.get_revision()));

	return mutable_revision;
}


void
GPlatesModel::Implementation::RevisionedReference::clone(
		RevisionContext &revision_context)
{
	d_revisionable = d_revisionable->clone_impl(revision_context);

	d_revision = d_revisionable->d_current_revision;

	// No model transaction needed here since the cloned revisionable object already
	// points to its cloned revision.
}
