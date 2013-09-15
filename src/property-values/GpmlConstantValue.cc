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

#include "GpmlConstantValue.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
GPlatesPropertyValues::GpmlConstantValue::create(
		PropertyValue::non_null_ptr_type value_,
		const StructuralType &value_type_,
		boost::optional<GPlatesUtils::UnicodeString> description_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(new GpmlConstantValue(transaction, value_, value_type_, description_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GpmlConstantValue::set_value(
		PropertyValue::non_null_ptr_type value_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().value.change(
			revision_handler.get_model_transaction(), value_);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlConstantValue::set_description(
		boost::optional<GPlatesUtils::UnicodeString> new_description)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().description = new_description;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlConstantValue::print_to(
		std::ostream &os) const
{
	return os << *get_current_revision<Revision>().value.get_revisionable();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlConstantValue::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// There's only one nested property value so it must be that.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_revisionable == revision.value.get_revisionable(),
			GPLATES_ASSERTION_SOURCE);

	// Create a new revision for the child property value.
	return revision.value.clone_revision(transaction);
}
