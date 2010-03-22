/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class FeatureRevision.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009 The University of Sydney, Australia
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

#include "FeatureRevision.h"
#include "DummyTransactionHandle.h"


GPlatesModel::container_size_type
GPlatesModel::FeatureRevision::append_child(
		TopLevelProperty::non_null_ptr_type new_top_level_property,
		DummyTransactionHandle &transaction)
{
	// FIXME:  Use the TransactionHandle properly to perform revisioning.
	d_properties.push_back(get_intrusive_ptr(new_top_level_property));
	return (size() - 1);
}


void
GPlatesModel::FeatureRevision::remove_child(
		container_size_type index,
		DummyTransactionHandle &transaction)
{
	// FIXME:  Use the TransactionHandle properly to perform revisioning.
	if (index < size()) {
		d_properties[index] = NULL;
	}
}
