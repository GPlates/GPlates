/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_SCALARCOVERAGEFEATUREPROPERTIES_H
#define GPLATES_APP_LOGIC_SCALARCOVERAGEFEATUREPROPERTIES_H

#include <vector>
#include <boost/optional.hpp>

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/PropertyName.h"

#include "property-values/GmlDataBlockCoordinateList.h"


namespace GPlatesAppLogic
{
	namespace ScalarCoverageFeatureProperties
	{
		/**
		 * Returns true if the specified feature behaves like a scalar coverage feature.
		 */
		bool
		is_scalar_coverage_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature);

		/**
		 * Returns true if the specified feature collection contains a scalar coverage feature.
		 */
		bool
		contains_scalar_coverage_feature(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		/**
		 * Returns the property name of the range of a scalar coverage, if any, that is associated with
		 * the specified property name of a domain.
		 *
		 * For example, 'gpml:domainSet' will return 'gpml:rangeSet'.
		 */
		boost::optional<GPlatesModel::PropertyName>
		get_range_property_name_from_domain(
				const GPlatesModel::PropertyName &domain_property_name);


		/**
		 * A coverage maps a geometry domain property to a range property containing one or more scalar types.
		 */
		struct Coverage
		{
			Coverage(
					GPlatesModel::FeatureHandle::iterator domain_property_,
					GPlatesModel::FeatureHandle::iterator range_property_,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type domain_,
					const std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type> &range_) :
				domain_property(domain_property_),
				range_property(range_property_),
				domain(domain_),
				range(range_)
			{  }

			GPlatesModel::FeatureHandle::iterator domain_property;
			GPlatesModel::FeatureHandle::iterator range_property;

			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type domain;
			std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type> range;
		};


		/**
		 * Visits a scalar coverage feature and extracts domain/range coverages from it.
		 *
		 * The heuristic that we're using here is that it is a scalar coverage feature if there is:
		 *  - A geometry property and a GmlDataBlock property with property names that match
		 *    a list of predefined property names (eg, 'gpml:domainSet'/'gpml:rangeSet').
		 *    These are property names matched in @a get_range_property_name_from_domain.
		 *
		 * NOTE: The coverages are extracted at the specified reconstruction time.
		 *
		 * Returns false if no coverags were extracted.
		 */
		bool
		get_coverages(
				std::vector<Coverage> &coverages,
				const GPlatesModel::FeatureHandle::weak_ref &feature,
				const double &reconstruction_time = 0);
	}
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEFEATUREPROPERTIES_H
