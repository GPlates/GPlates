/* $Id$ */

/**
 * \file 
 * Contains the implementation of the FeatureCollectionHandle class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "FeatureCollectionHandle.h"

#include "BasicHandle.h"
#include "FeatureStoreRootHandle.h"
#include "WeakReferenceCallback.h"

#include "scribe/Scribe.h"


const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
GPlatesModel::FeatureCollectionHandle::create()
{
	return non_null_ptr_type(new FeatureCollectionHandle());
}


const GPlatesModel::FeatureCollectionHandle::weak_ref
GPlatesModel::FeatureCollectionHandle::create(
		const WeakReference<FeatureStoreRootHandle> &feature_store_root)
{
	non_null_ptr_type feature_collection = create();
	FeatureStoreRootHandle::iterator iter = feature_store_root->add(feature_collection);

	return (*iter)->reference();
}


GPlatesModel::FeatureCollectionHandle::FeatureCollectionHandle() :
	BasicHandle<FeatureCollectionHandle>(
			this,
			revision_type::create())
{
}


GPlatesScribe::TranscribeResult
GPlatesModel::FeatureCollectionHandle::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Transcribe the tags.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_tags, "tags"))
	{
		return scribe.get_transcribe_result();
	}

	if (scribe.is_saving())
	{
		// Get the current list of features.
		const std::vector<FeatureHandle::non_null_ptr_type> features(begin(), end());

		// Save the features.
		scribe.save(TRANSCRIBE_SOURCE, features, "features");
	}
	else // loading
	{
		// Load the features.
		std::vector<FeatureHandle::non_null_ptr_type> features;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, features, "features"))
		{
			return scribe.get_transcribe_result();
		}

		// Add the features.
		for (auto feature : features)
		{
			add(feature);
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
