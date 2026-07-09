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

#include <typeinfo>

#include "Revisionable.h"

#include "ModelTransaction.h"
#include "RevisionContext.h"


bool
GPlatesModel::Revisionable::operator==(
		const Revisionable &other) const
{
	// Both objects must have the same type before testing for equality.
	// This also means derived classes need no type-checking.
	if (typeid(*this) != typeid(other))
	{
		return false;
	}

	// Compare the derived type objects.
	// Since most (all) of the value data is contained in the revisions, which is handled by the
	// base Revisionable class, the derived revisionable classes don't typically do any comparison
	// and so its usually all handled by Revisionable::equality() which compares the revisions.
	return equality(other);
}


bool
GPlatesModel::Revisionable::equality(
		const Revisionable &other) const
{
	// Compare the mutable data that is contained in the revisions.
	return d_current_revision->equality(*other.d_current_revision);
}


boost::optional<GPlatesModel::Model &>
GPlatesModel::Revisionable::get_model()
{
	if (!d_current_revision->get_context())
	{
		return boost::none;
	}

	return d_current_revision->get_context()->get_model();
}


boost::optional<const GPlatesModel::Model &>
GPlatesModel::Revisionable::get_model() const
{
	if (!d_current_revision->get_context())
	{
		return boost::none;
	}

	boost::optional<GPlatesModel::Model &> model = d_current_revision->get_context()->get_model();
	if (!model)
	{
		return boost::none;
	}

	return model.get();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesModel::Revisionable::create_bubble_up_revision(
		ModelTransaction &transaction) const
{
	// If we don't have a (parent) context then just clone the current revision without any context.
	if (!get_current_revision()->get_context())
	{
		Revision::non_null_ptr_type cloned_revision = get_current_revision()->clone_revision();

		transaction.add_revision_transaction(
				ModelTransaction::RevisionTransaction(this, cloned_revision));

		return cloned_revision;
	}

	// We have a parent context so bubble up the revision towards the root
	// (feature store). And our parent will create a new revision for us...
	return get_current_revision()->get_context()->bubble_up(transaction, this);
}
