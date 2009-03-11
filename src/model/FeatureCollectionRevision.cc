/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class FeatureCollectionRevision.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include "FeatureCollectionRevision.h"
#include "DummyTransactionHandle.h"


GPlatesModel::FeatureCollectionRevision::feature_collection_type::size_type
GPlatesModel::FeatureCollectionRevision::append_feature(
		FeatureHandle::non_null_ptr_type new_feature,
		DummyTransactionHandle &transaction)
{
	// FIXME:  Use the TransactionHandle properly to perform revisioning.
	d_features.push_back(get_intrusive_ptr(new_feature));
	new_feature->set_feature_collection_handle_ptr(d_handle_ptr);
	return (size() - 1);
}


void
GPlatesModel::FeatureCollectionRevision::remove_feature(
		feature_collection_type::size_type index,
		DummyTransactionHandle &transaction)
{
	if (d_features[index] == NULL) {
		// That's strange, it appears that the feature at this index has already been
		// removed.  (Note that this should not occur.)  Since this has happened, we should
		// log a warning about this strange behaviour, so we can work out why.
		// FIXME: Log a warning.
		return;  // Nothing to do.
	}
	d_features[index]->set_feature_collection_handle_ptr(NULL);

	// FIXME:  Use the TransactionHandle properly to perform revisioning.
	if (index < size()) {
		d_features[index] = NULL;
	}
}
