/* $Id$ */

/**
 * \file 
 * Registry allowing fast lookup of FeatureHandle::weak_ref when given
 * a FeatureId.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATUREIDREGISTRY_H
#define GPLATES_MODEL_FEATUREIDREGISTRY_H

#include <boost/optional.hpp>
#include <map>
#include "FeatureHandle.h"
#include "WeakReference.h"
#include "FeatureId.h"


namespace GPlatesModel
{
	/**
	 * The FeatureIdRegistry is used to resolve a FeatureId to a
	 * FeatureHandle::weak_ref. Since Features are normally stored
	 * as FeatureHandles inside a FeatureCollection, iterating over
	 * the entire feature collection(s) just to find the one feature
	 * you are interested in would get very slow very quickly.
	 *
	 * The FeatureIdRegistry belongs to the Model, and should be
	 * accessed via the 'find_feature_by_id' method, or
	 * an appropriate ModelUtils function. It should not be used
	 * directly by non-Model code.
	 *
	 * Internally, it is implemented as a std::map that caches
	 * weak_refs. This may change in the future.
	 */
	class FeatureIdRegistry
	{
	public:

		/**
		 * Convenience typedef for the map storing feature ids.
		 */
		typedef std::map<GPlatesModel::FeatureId, GPlatesModel::FeatureHandle::weak_ref> id_map_type;
		typedef id_map_type::const_iterator id_map_const_iterator;
	
		FeatureIdRegistry();
		
		/**
		 * Adds a feature to the registry. The registry does not
		 * take ownership of the FeatureHandle.
		 * 
		 * FIXME: Throw exception if duplicate ID registered?
		 *
		 * Note that this method is not simply named 'register', as
		 * that is, of course, a reserved keyword - and will cause
		 * hilariously misleading and confusing compile errors.
		 */
		void
		register_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref);
		
		/**
		 * Removes a feature from the registry. This only removes
		 * the weak_ref, it does not remove the feature from any
		 * FeatureCollections.
		 */
		void
		deregister_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref);
		
		/**
		 * Searches the registry to locate a FeatureHandle
		 * from the given FeatureId. This will return boost::none
		 * if the FeatureId does not exist in the registry.
		 *
		 * Bear in mind also that the returned weak_ref may
		 * be invalid for other reasons.
		 */
		boost::optional<GPlatesModel::FeatureHandle::weak_ref>
		find(
				const GPlatesModel::FeatureId &feature_id);
	
	private:

		id_map_type d_id_map;
	};
}

#endif // GPLATES_MODEL_FEATUREIDREGISTRY_H
