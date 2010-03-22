/* $Id$ */

/**
 * \file 
 * Contains the definition of the class HandleData<FeatureHandle>.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATUREHANDLEDATA_H
#define GPLATES_MODEL_FEATUREHANDLEDATA_H

#include "FeatureType.h"
#include "FeatureId.h"
#include "HandleData.h"

namespace GPlatesModel
{
	class FeatureHandle;

	/**
	 * Specialisation of HandleData for FeatureHandle.
	 */
	template<>
	class HandleData<FeatureHandle>
	{

	public:

		HandleData(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_) :
			d_feature_type(feature_type_),
			d_feature_id(feature_id_)
		{
		}

		/**
		 * Returns the feature type of this feature. No "setter" method is provided
		 * because the feature type of a feature should never be changed.
		 */
		const FeatureType &
		feature_type() const
		{
			return d_feature_type;
		}

		/**
		 * Returns the feature ID of this feature. No "setter" method is provided
		 * because the feature ID of a feature should never be changed.
		 */
		const FeatureId &
		feature_id() const
		{
			return d_feature_id;
		}

	private:

		//! The type of this feature.
		FeatureType d_feature_type;

		//! The unique feature ID of this feature.
		FeatureId d_feature_id;

		friend class FeatureHandle;

	};
}

#endif  // GPLATES_MODEL_HANDLEDATA_H
