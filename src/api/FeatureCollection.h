/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_API_FEATURECOLLECTION_H
#define GPLATES_API_FEATURECOLLECTION_H

#include "model/FeatureCollectionHandle.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesApi
{
	/**
	 * Wrapper around FeatureCollectionHandle for exposing to Python.
	 *
	 * Note: this holds a strong reference to a FeatureCollectionHandle, because we
	 * don't want Python users to have to worry about checking weak-ref validity.
	 */
	class FeatureCollection :
			public GPlatesUtils::ReferenceCount<FeatureCollection>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<FeatureCollection> non_null_ptr_type;

		static
		non_null_ptr_type
		create(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
		{
			return new FeatureCollection(feature_collection);
		}

		std::size_t
		size() const;

	private:

		explicit
		FeatureCollection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		GPlatesUtils::non_null_intrusive_ptr<GPlatesModel::FeatureCollectionHandle> d_feature_collection;
	};
}

#endif  // GPLATES_API_FEATURECOLLECTION_H
