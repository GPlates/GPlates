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

#include "GpmlKeyValueDictionary.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/NotYetImplementedException.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
GPlatesPropertyValues::GpmlKeyValueDictionary::create(
	const std::vector<GpmlKeyValueDictionaryElement> &elements)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(new GpmlKeyValueDictionary(transaction, elements));
	transaction.commit();
	return ptr;
}


void
GPlatesPropertyValues::GpmlKeyValueDictionary::set_elements(
		const std::vector<GpmlKeyValueDictionaryElement> &elements)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().elements = elements;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlKeyValueDictionary::print_to(
		std::ostream &os) const
{
	const std::vector<GpmlKeyValueDictionaryElement> &elements = get_elements();

	os << "[ ";

	for (std::vector<GpmlKeyValueDictionaryElement>::const_iterator elements_iter = elements.begin();
		elements_iter != elements.end();
		++elements_iter)
	{
		os << *elements_iter;
	}

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlKeyValueDictionary::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Currently this can't be reached because we don't attach to our children yet.
	throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
}
