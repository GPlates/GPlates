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

#include <utility>
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
		 * Returns the default domain/range property names ('gpml:domainSet' and 'gpml:rangeSet').
		 */
		std::pair<GPlatesModel::PropertyName/*domain*/, GPlatesModel::PropertyName/*range*/>
		get_default_domain_range_property_names();


		/**
		 * A coverage maps a geometry domain property to a range property containing one or more scalar types.
		 *
		 * The range has one scalar per point of the domain, in the order 'GeometryUtils::get_geometry_points'
		 * gives them: a polygon's exterior ring, then each of its interior rings.
		 */
		struct Coverage
		{
			Coverage(
					GPlatesModel::FeatureHandle::iterator domain_property_,
					GPlatesModel::FeatureHandle::iterator range_property_,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type domain_,
					const std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type> &range_,
					bool exterior_ring_only_) :
				domain_property(domain_property_),
				range_property(range_property_),
				domain(domain_),
				range(range_),
				exterior_ring_only(exterior_ring_only_)
			{  }

			GPlatesModel::FeatureHandle::iterator domain_property;
			GPlatesModel::FeatureHandle::iterator range_property;

			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type domain;
			std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type> range;

			/**
			 * True if the domain property is a polygon with interior rings whose range has scalars for
			 * its exterior ring only, as older files have (see 'READ_EXTERIOR_RING_ONLY_COVERAGES' in
			 * the '.cc' file).
			 *
			 * Then @a domain is the exterior ring alone (see @a get_exterior_ring_domain), and anything
			 * that pairs the scalars with the points of the domain *property*, or of a geometry
			 * reconstructed from it, must use only the exterior ring's points.
			 */
			bool exterior_ring_only;
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
		 * Returns false if no coverages were extracted.
		 */
		bool
		get_coverages(
				std::vector<Coverage> &coverages,
				const GPlatesModel::FeatureHandle::weak_ref &feature,
				const double &reconstruction_time = 0);


		/**
		 * Returns the geometry an exterior-ring-only coverage covers (see 'Coverage::exterior_ring_only'):
		 * a polygon of @a domain_geometry's exterior ring alone, or @a domain_geometry itself if it is not
		 * a polygon or has no interior rings.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_exterior_ring_domain(
				const GPlatesMaths::GeometryOnSphere &domain_geometry);
	}
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEFEATUREPROPERTIES_H
