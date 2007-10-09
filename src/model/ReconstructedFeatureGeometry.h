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
#include "utils/non_null_intrusive_ptr.h"


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

	public:
		ReconstructedFeatureGeometry(
				GPlatesUtils::non_null_intrusive_ptr<const geometry_type> geometry_ptr,
				FeatureHandle &feature_handle) :
			d_geometry_ptr(geometry_ptr),
			d_feature_ref(feature_handle.reference())
		{  }

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

		// ...
	};
}

#endif  // GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRY_H
