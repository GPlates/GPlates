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

#include "GmlOrientableCurve.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


const GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
GPlatesPropertyValues::GmlOrientableCurve::create(
		GPlatesModel::PropertyValue::non_null_ptr_type base_curve_,
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &xml_attributes_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(new GmlOrientableCurve(transaction, base_curve_, xml_attributes_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GmlOrientableCurve::set_base_curve(
		GPlatesModel::PropertyValue::non_null_ptr_type bc)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().base_curve.change(
			revision_handler.get_model_transaction(), bc);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlOrientableCurve::set_xml_attributes(
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &xml_attributes)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().xml_attributes = xml_attributes;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlOrientableCurve::print_to(
		std::ostream &os) const
{
	return get_current_revision<Revision>().base_curve.get_revisionable()->print_to(os);
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GmlOrientableCurve::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// There's only one nested property value so it must be that.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_revisionable == revision.base_curve.get_revisionable(),
			GPLATES_ASSERTION_SOURCE);

	// Create a new revision for the child property value.
	return revision.base_curve.clone_revision(transaction);
}
