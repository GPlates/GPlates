/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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
#ifndef GPLATES_UTILS_GEOMETRYUTIL_H
#define GPLATES_UTILS_GEOMETRYUTIL_H

#include <algorithm>
#include <iostream>
#include <cmath>
#include <vector>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/FiniteRotation.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolylineIntersections.h"

#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/PropertyName.h"
#include "model/ReconstructedFeatureGeometryFinder.h"
#include "model/ReconstructionTree.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTopologicalIntersection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlPoint.h"

#include "utils/UnicodeStringUtils.h"

#include "view-operations/GeometryType.h"


namespace GPlatesUtils
{
	namespace GeometryUtils
	{

		typedef std::vector<GPlatesMaths::PointOnSphere> point_seq_type;

		/**
		 * Copies the @a PointOnSphere points from @a geometry_on_sphere to the @a points array.
		 *
		 * Does not clear @a points - just appends whatever points it
		 * finds in @a geometry_on_sphere.
		 *
		 * If @a reverse_points is true then the order of the points in @a geometry_on_sphere
		 * are reversed before appending to @a points.
		 */
		void
		get_geometry_points(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				std::vector<GPlatesMaths::PointOnSphere> &points,
				bool reverse_points = false);


		/**
		 * Returns the end points of @a geometry_on_sphere.
		 *
		 * If @a reverse_points is true then the order of the returned end points
		 * is reversed.
		 *
		 * This is faster than calling @a get_geometry_points and then picking out the
		 * first and last points as it doesn't retrieve all the points.
		 */
		std::pair<
				GPlatesMaths::PointOnSphere/*start point*/,
				GPlatesMaths::PointOnSphere/*end point*/>
		get_geometry_end_points(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				bool reverse_points = false);

		/**
		 * Create PropertyValue object given iterator of points and geometry type
		 *
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		create_geometry_property_value(
				point_seq_type::const_iterator begin, 
				point_seq_type::const_iterator end,
				GPlatesViewOperations::GeometryType::Value type);
		
		/**
		* Removes any properties that contain geometry from @a feature_ref.
		*/
		void
		remove_geometry_properties_from_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref);
	
	}

}

#endif //GPLATES_UTILS_GEOMETRYUTIL_H

