/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "GpmlScalarField3DFile.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/ModelTransaction.h"
#include "model/PropertyValueBubbleUpRevisionHandler.h"


const GPlatesPropertyValues::GpmlScalarField3DFile::non_null_ptr_type
GPlatesPropertyValues::GpmlScalarField3DFile::create(
		XsString::non_null_ptr_type filename_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(new GpmlScalarField3DFile(transaction, filename_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GpmlScalarField3DFile::set_file_name(
		XsString::non_null_ptr_type filename_)
{
	GPlatesModel::PropertyValueBubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().filename.change(
			revision_handler.get_model_transaction(), filename_);
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlScalarField3DFile::print_to(
		std::ostream &os) const
{
	return os << "GpmlScalarField3DFile";
}


GPlatesModel::PropertyValueRevision::non_null_ptr_type
GPlatesPropertyValues::GpmlScalarField3DFile::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const PropertyValue::non_null_ptr_to_const_type &child_property_value)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_property_value == revision.filename.get_property_value(),
			GPLATES_ASSERTION_SOURCE);

	return revision.filename.clone_revision(transaction);
}
