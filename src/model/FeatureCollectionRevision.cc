/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class FeatureCollectionRevision.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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


void
GPlatesModel::FeatureCollectionRevision::append_feature(
		boost::intrusive_ptr<FeatureHandle> new_feature,
		DummyTransactionHandle &transaction)
{
	// FIXME:  Use the TransactionHandle properly to perform revisioning.
	d_features.push_back(new_feature);
}


void
GPlatesModel::FeatureCollectionRevision::remove_feature(
		feature_collection_type::size_type index,
		DummyTransactionHandle &transaction)
{
	// FIXME:  Use the TransactionHandle properly to perform revisioning.
	if (index < size()) {
		d_features[index] = NULL;
	}
}
