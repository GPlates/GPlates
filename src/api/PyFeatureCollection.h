/* $Id: FeatureCollection.h 11961 2011-07-07 03:49:38Z mchin $ */

/**
 * \file 
 * $Revision: 11961 $
 * $Date: 2011-07-07 13:49:38 +1000 (Thu, 07 Jul 2011) $
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

#include "PyFeature.h"
#include "global/python.h"
#include "model/FeatureHandle.h"
#include "model/FeatureCollectionHandle.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"

namespace GPlatesApi
{
	/**
	 * Wrapper around FeatureCollectionHandle for exposing to Python.
	 *
	 */
	class FeatureCollection 
	{
	public:

		static
		FeatureCollection
		create(
				GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection)
		{
			return FeatureCollection(feature_collection);
		}

		std::size_t
		size() const;

		boost::python::list
		features()
		{
			boost::python::list result;
			if(d_feature_collection.is_valid())
			{
				GPlatesModel::FeatureCollectionHandle::iterator 
					it = d_feature_collection->begin(),
					it_end = d_feature_collection->end();
				for(; it != it_end; it++)
				{
					result.append(Feature((*it)->reference()));
				}
			}
			return result;
		}

	private:

		explicit
		FeatureCollection(
				GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		GPlatesModel::FeatureCollectionHandle::weak_ref d_feature_collection;
	};
}

#endif  // GPLATES_API_FEATURECOLLECTION_H
