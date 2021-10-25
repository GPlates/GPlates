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


GPlatesModel::FeatureCollectionHandle::tags_type &
GPlatesModel::FeatureCollectionHandle::tags()
{
	return d_tags;
}


const GPlatesModel::FeatureCollectionHandle::tags_type &
GPlatesModel::FeatureCollectionHandle::tags() const
{
	return d_tags;
}


GPlatesModel::FeatureCollectionHandle::FeatureCollectionHandle() :
	BasicHandle<FeatureCollectionHandle>(
			this,
			revision_type::create())
{  }


GPlatesScribe::TranscribeResult
GPlatesModel::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GPlatesModel::FeatureCollectionHandle> &construct_feature_collection_handle)
{
	if (scribe.is_saving())
	{
		// Default saves no data because load uses default constructor
		// which has no arguments and hence loads no data.
	}
	else // loading...
	{
		// Get information that is not transcribed into the archive.
		GPlatesScribe::TranscribeContext<GPlatesModel::FeatureCollectionHandle> &transcribe_context =
				scribe.get_transcribe_context<GPlatesModel::FeatureCollectionHandle>();

		construct_feature_collection_handle.construct_object();

		GPlatesModel::FeatureCollectionHandle &feature_collection_handle =
				construct_feature_collection_handle.get_object();

		// Turn into a non-null pointer just so we can add it to the model.
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
				feature_collection_handle_non_null_ptr(&feature_collection_handle);

		// Make sure the feature collection handle stays alive by adding it to the model.
		transcribe_context.model_interface->root()->add(feature_collection_handle_non_null_ptr);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
