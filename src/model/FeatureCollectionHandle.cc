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


namespace GPlatesModel
{
	class FeatureCollectionHandleUnsavedChangesCallback :
			public WeakReferenceCallback<FeatureCollectionHandle>
	{
	public:

		explicit
		FeatureCollectionHandleUnsavedChangesCallback(
				bool &contains_unsaved_changes) :
			d_contains_unsaved_changes(contains_unsaved_changes)
		{
		}

		void
		publisher_modified(
				const WeakReferencePublisherModifiedEvent<FeatureCollectionHandle> &)
		{
			d_contains_unsaved_changes = true;
		}

	private:

		bool &d_contains_unsaved_changes;
	};
}


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


bool
GPlatesModel::FeatureCollectionHandle::contains_unsaved_changes() const
{
	return d_contains_unsaved_changes;
}


void
GPlatesModel::FeatureCollectionHandle::clear_unsaved_changes()
{
	d_contains_unsaved_changes = false;
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
			revision_type::create()),
	d_contains_unsaved_changes(false),
	d_weak_ref_to_self(WeakReference<FeatureCollectionHandle>(*this))
{
	// Attach callback to weak-ref.
	d_weak_ref_to_self.attach_callback(
			new FeatureCollectionHandleUnsavedChangesCallback(
				d_contains_unsaved_changes));
}

