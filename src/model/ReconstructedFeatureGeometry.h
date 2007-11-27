/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRY_H
#define GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRY_H

#include "FeatureHandle.h"
#include "feature-visitors/PlateIdFinder.h"
#include "utils/non_null_intrusive_ptr.h"
#include <boost/optional.hpp>


namespace GPlatesModel
{
	// At the moment I'm just going to stick an underlying object in this class

	template<class T>
	class ReconstructedFeatureGeometry
	{
	public:
		typedef T geometry_type;

	private:
		GPlatesUtils::non_null_intrusive_ptr<const geometry_type> d_geometry_ptr;

		FeatureHandle::weak_ref d_feature_ref;

		/**
		 * We cache the plate id here so that it can be extracted by drawing code
		 * for use with on-screen colouring.
		 */
		boost::optional<integer_plate_id_type> d_reconstruction_plate_id;

	public:
		ReconstructedFeatureGeometry(
				GPlatesUtils::non_null_intrusive_ptr<const geometry_type> geometry_ptr,
				FeatureHandle &feature_handle) :
			d_geometry_ptr(geometry_ptr),
			d_feature_ref(feature_handle.reference())
		{
			GPlatesFeatureVisitors::PlateIdFinder finder(PropertyName("gpml:plateId"));
			feature_handle.accept_visitor(finder);
			// Select the first plate id, if we found one.
			if (finder.found_plate_ids_begin() != finder.found_plate_ids_end()) {
				d_reconstruction_plate_id = *finder.found_plate_ids_begin();
			}
		}

		const GPlatesUtils::non_null_intrusive_ptr<const geometry_type>
		geometry() const
		{
			return d_geometry_ptr;
		}

		const FeatureHandle::weak_ref
		feature_ref() const
		{
			return d_feature_ref;
		}

		/**
		 * Return the cached plate id.
		 */
		const boost::optional<integer_plate_id_type> &
		reconstruction_plate_id() const {

			return d_reconstruction_plate_id;
		}
	};
}

#endif  // GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRY_H
