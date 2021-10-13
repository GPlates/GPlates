/* $Id: FeatureCollection.cc 11961 2011-07-07 03:49:38Z mchin $ */

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
#include "PyFeatureCollection.h"

#if !defined(GPLATES_NO_PYTHON)
GPlatesApi::FeatureCollection::FeatureCollection(
		GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection) :
	d_feature_collection(feature_collection)
{  }


std::size_t
GPlatesApi::FeatureCollection::size() const
{
	if(!d_feature_collection.is_valid())
		return 0;
	else
		return d_feature_collection->size();
}


void
export_feature_collection()
{
	using namespace boost::python;

	class_<GPlatesApi::FeatureCollection>("FeatureCollection", no_init /* for now, disable creation */)
		//.add_property("size", &GPlatesApi::FeatureCollection::size)
		.def("size", &GPlatesApi::FeatureCollection::size)
		.def("features", &GPlatesApi::FeatureCollection::features)
		;
}
#endif   //GPLATES_NO_PYTHON

