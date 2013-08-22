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

#include "GpmlTopologicalLineSection.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/ModelTransaction.h"
#include "model/PropertyValueBubbleUpRevisionHandler.h"


const GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalLineSection::create(
		GpmlPropertyDelegate::non_null_ptr_type source_geometry_,
		const bool reverse_order) 
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(new GpmlTopologicalLineSection(transaction, source_geometry_, reverse_order));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GpmlTopologicalLineSection::set_source_geometry(
		GpmlPropertyDelegate::non_null_ptr_type source_geometry_)
{
	GPlatesModel::PropertyValueBubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().source_geometry.change(
			revision_handler.get_model_transaction(), source_geometry_);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlTopologicalLineSection::set_reverse_order(
		bool reverse_order)
{
	GPlatesModel::PropertyValueBubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().reverse_order = reverse_order;
	revision_handler.commit();
}


GPlatesModel::PropertyValueRevision::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalLineSection::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const PropertyValue::non_null_ptr_to_const_type &child_property_value)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_property_value == revision.source_geometry.get_property_value(),
			GPLATES_ASSERTION_SOURCE);

	return revision.source_geometry.clone_revision(transaction);
}
