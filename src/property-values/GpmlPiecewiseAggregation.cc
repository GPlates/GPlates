/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#include <iostream>
#include <boost/foreach.hpp>

#include "GpmlPiecewiseAggregation.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


const GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
GPlatesPropertyValues::GpmlPiecewiseAggregation::create(
		const std::vector<GpmlTimeWindow::non_null_ptr_type> &time_windows_,
		const StructuralType &value_type_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(
			new GpmlPiecewiseAggregation(
					transaction,
					GPlatesModel::RevisionedVector<GpmlTimeWindow>::create(time_windows_),
					value_type_));
	transaction.commit();
	return ptr;
}


std::ostream &
GPlatesPropertyValues::GpmlPiecewiseAggregation::print_to(
		std::ostream &os) const
{
	const GPlatesModel::RevisionedVector<GpmlTimeWindow> &windows = time_windows();

	os << "[ ";

	GPlatesModel::RevisionedVector<GpmlTimeWindow>::const_iterator windows_iter =
			windows.begin();
	GPlatesModel::RevisionedVector<GpmlTimeWindow>::const_iterator windows_end =
			windows.end();
	for ( ; windows_iter != windows_end; ++windows_iter)
	{
		os << *windows_iter->get();
	}

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlPiecewiseAggregation::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.
	if (child_revisionable == revision.time_windows.get_revisionable())
	{
		return revision.time_windows.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}
