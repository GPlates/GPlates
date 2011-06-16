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
#include "FeatureCollection.h"

#include "global/python.h"

#if !defined(GPLATES_NO_PYTHON)
GPlatesApi::FeatureCollection::FeatureCollection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection) :
	d_feature_collection(feature_collection.handle_ptr())
{  }


std::size_t
GPlatesApi::FeatureCollection::size() const
{
	return d_feature_collection->size();
}


void
export_feature_collection()
{
	using namespace boost::python;

	class_<GPlatesApi::FeatureCollection, GPlatesApi::FeatureCollection::non_null_ptr_type, boost::noncopyable>("FeatureCollection", no_init /* for now, disable creation */)
		.add_property("size", &GPlatesApi::FeatureCollection::size);
}
#endif   //GPLATES_NO_PYTHON

