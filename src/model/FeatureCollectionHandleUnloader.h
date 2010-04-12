/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATURECOLLECTIONHANDLESHAREDREF_H
#define GPLATES_MODEL_FEATURECOLLECTIONHANDLESHAREDREF_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "FeatureCollectionHandle.h"
#include "FeatureStoreRootHandle.h"

namespace GPlatesModel
{
	/**
	 * Manages unloading of a feature collection handle.
	 *
	 * Wraps around a FeatureCollectionHandle::weak_ref to make sure
	 * it gets unloaded when it's not needed/referenced
	 */
	class FeatureCollectionHandleUnloader :
			private boost::noncopyable
	{
	public:
		/**
		 * Shared reference to a @a FeatureCollectionHandleUnloader.
		 */
		typedef boost::shared_ptr<FeatureCollectionHandleUnloader> shared_ref;


		/**
		 * Returns a shared reference to a @a FeatureCollectionHandleUnloader which
		 * will unload @a feature_collection when the last reference is destroyed.
		 */
		static
		shared_ref
		create(
				const FeatureCollectionHandle::weak_ref &feature_collection)
		{
			return shared_ref(new FeatureCollectionHandleUnloader(feature_collection));
		}


		~FeatureCollectionHandleUnloader()
		{
			unload_feature_collection();
		}


		/**
		 * Returns a weak reference to the feature collection handle.
		 */
		const FeatureCollectionHandle::weak_ref
		get_feature_collection() const
		{
			return d_feature_collection;
		}

	private:
		FeatureCollectionHandle::weak_ref d_feature_collection;


		/**
		 * Constructor - created object is not copyable - use @a create if want ability to copy.
		 *
		 * Is private so that only way to create is via @a create.
		 *
		 * Destructor will unload feature collection if it is valid.
		 */
		FeatureCollectionHandleUnloader(
				const FeatureCollectionHandle::weak_ref &feature_collection) :
			d_feature_collection(feature_collection)
		{  }


		void
		unload_feature_collection()
		{
			// Check to see if the feature collection has already been unloaded
			// or if the model that contains it has been destroyed (effectively unloading it).
			if (d_feature_collection.is_valid())
			{
				FeatureStoreRootHandle *parent = d_feature_collection->parent_ptr();
				FeatureStoreRootHandle::iterator iter =
					FeatureStoreRootHandle::iterator(*parent, d_feature_collection->index_in_container());
				// DummyTransactionHandle transaction(__FILE__, __LINE__);
				parent->remove(iter);
				// transaction.commit();
			}
		}
	};
}

#endif // GPLATES_MODEL_FEATURECOLLECTIONHANDLESHAREDREF_H
