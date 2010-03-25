/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYINTERNALUTILS_H
#define GPLATES_APP_LOGIC_TOPOLOGYINTERNALUTILS_H

#include <utility>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <QString>

#include "maths/GeometryOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"
#include "model/ReconstructedFeatureGeometry.h"

#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTopologicalIntersection.h"
#include "property-values/GpmlTopologicalSection.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesModel
{
	class Reconstruction;
	class ReconstructionTree;
}

namespace GPlatesPropertyValues
{
	class GpmlPropertyDelegate;
}

namespace GPlatesAppLogic
{
	/**
	 * This namespace contains utilities that are used internally in topology-related code.
	 */
	namespace TopologyInternalUtils
	{
		/**
		 * Creates and returns a gpml topological section property value that references
		 * the geometry property @a geometry_property.
		 *
		 * Returns false if @a geometry_property is invalid or does not refer to one
		 * of @a GmlPoint, @a GmlLineString or @a GmlPolygon.
		 *
		 * FIXME: Don't return false for a @a GmlMultiPoint.
		 *
		 * If @a start_intersecting_geometry_property and/or @a end_intersecting_geometry_property
		 * is true (and if @a present_day_reference_point is true) then appropriate
		 * start and end @a GpmlTopologicalIntersection objects are set on the
		 * topological section being created (provided it is of type
		 * @a GpmlTopologicalLineSection).
		 * NOTE: These should only be set if the previous/next sections relative
		 * to the topological section (in the topological polygon) are intersecting
		 * (or contain geometry that is intersectable such as line geometry).
		 *
		 * @a reverse_order is only used if a @a GpmlTopologicalLineSection is returned.
		 *
		 * The @a geometry_property is a feature properties iterator but currently
		 * it we just get the property name from it and use that to create the
		 * property delegate - when a better method of property delegation is devised
		 * (one that works in the presence of multiple geometry properties with the
		 * same property name inside a single feature) then the properties iterator
		 * might refer to something that has a unique property id.
		 */
		boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
		create_gpml_topological_section(
				const GPlatesModel::FeatureHandle::children_iterator &geometry_property,
				bool reverse_order = false,
				const boost::optional<GPlatesModel::FeatureHandle::children_iterator>
						&start_intersecting_geometry_property = boost::none,
				const boost::optional<GPlatesModel::FeatureHandle::children_iterator>
						&end_intersecting_geometry_property = boost::none,
				const boost::optional<GPlatesMaths::PointOnSphere> &
						present_day_reference_point = boost::none);


		/**
		 * Creates and returns a gpml topological intersection property value that references
		 * the geometry property @a adjacent_geometry_property and has unrotated click point
		 * @a present_day_reference_point.
		 *
		 * Returns false if @a adjacent_geometry_property is invalid or does not refer to one
		 * of @a GmlLineString or @a GmlPolygon - this is because point and multi-point
		 * are not considered intersectable geometries.
		 *
		 * This function is used internally by @a create_gpml_topological_section.
		 */
		boost::optional<GPlatesPropertyValues::GpmlTopologicalIntersection>
		create_gpml_topological_intersection(
				const GPlatesModel::FeatureHandle::children_iterator &adjacent_geometry_property,
				const GPlatesMaths::PointOnSphere &present_day_reference_point);


		/**
		 * Create a geometry property delegate from a feature properties iterator and
		 * a property value type string (eg, "LineString").
		 *
		 * Returns false if @a geometry_property is invalid.
		 *
		 * This function is used internally by @a create_gpml_topological_section and
		 * @a create_gpml_topological_intersection.
		 */
		boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type>
		create_geometry_property_delegate(
				const GPlatesModel::FeatureHandle::children_iterator &geometry_property,
				const QString &property_value_type);


		/**
		 * Retrieves a @a FeatureHandle weak reference associated with @a feature_id.
		 *
		 * If number of feature handles associated with @a feature_id is not one
		 * then a null feature handle reference is returned.
		 */
		GPlatesModel::FeatureHandle::weak_ref
		resolve_feature_id(
				const GPlatesModel::FeatureId &feature_id);


		/**
		 * Finds the reconstructed feature geometry, contained in @a reconstruction,
		 * for the geometry property referenced by the property delegate @a geometry_delegate.
		 *
		 * Returns false if:
		 * - there is *not* exactly *one* feature referencing the delegate feature id, or
		 * - there is *not* exactly *one* RFG in @a reconstruction that is referencing
		 *   the delegate feature (this means there are multiple geometry properties
		 *   in the delegate feature that have the same delegate property name.
		 *
		 * WARNING: Property delegates need to be improved because they do not uniquely
		 * identify a property since they use the property name and a feature can
		 * have multiple properties with the same name.
		 */
		boost::optional<GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type>
		find_reconstructed_feature_geometry(
				const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate,
				GPlatesModel::Reconstruction *reconstruction);


		/**
		 * Finds the reconstructed feature geometry, contained in @a reconstruction,
		 * for the geometry properties iterator @a geometry_property.
		 *
		 * Returns false if:
		 * - @a geometry_property is invalid, or
		 * - there is *not* exactly *one* RFG in @a reconstruction that is referencing
		 *   the delegate feature (this means there are multiple geometry properties
		 *   in the delegate feature that have the same delegate property name).
		 */
		boost::optional<GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type>
		find_reconstructed_feature_geometry(
				const GPlatesModel::FeatureHandle::children_iterator &geometry_property,
				GPlatesModel::Reconstruction *reconstruction);


		/**
		 * Returns the @a FiniteRotation obtained by looking up @a reconstruction_tree
		 * using the plate id from the "gpml:reconstructionPlateId" property of
		 * @a reconstruction_plateid_feature.
		 *
		 * If no "gpml:reconstructionPlateId" property is found then false is returned.
		 */
		boost::optional<GPlatesMaths::FiniteRotation>
		get_finite_rotation(
				const GPlatesModel::FeatureHandle::weak_ref &reconstruction_plateid_feature,
				const GPlatesModel::ReconstructionTree &reconstruction_tree);


		/**
		 * Intersects geometries @a first_section_reconstructed_geometry and
		 * @a second_section_reconstructed_geometry if they are intersectable
		 * (see @a get_polylines_for_intersection) and for each intersected section
		 * (assuming each section is intersected into two segments)
		 * returns the segment that is closest to the respective reference point.
		 *
		 * If the geometries are not intersectable then the original geometries are returned.
		 *
		 * The optional @a PointOnSphere returned is the intersection point if the sections
		 * intersected - this can be tested for 'false' to see if an intersection occurred.
		 * Even if 'false' is returned the two returned segment geometries are still valid
		 * (but they are just the geometries passed into this function).
		 *
		 * FIXME: If the number of intersections is two or more then the original unintersected
		 * geometries are returned.
		 */
		boost::tuple<
				boost::optional<GPlatesMaths::PointOnSphere>/*intersection point*/, 
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type/*first_section_closest_segment*/,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type/*second_section_closest_segment*/>
		intersect_topological_sections(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_section_reconstructed_geometry,
				const GPlatesMaths::PointOnSphere &first_section_reconstructed_reference_point,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_section_reconstructed_geometry,
				const GPlatesMaths::PointOnSphere &second_section_reconstructed_reference_point);


		/**
		 * Same as the other overload of @a intersect_topological_sections but accepts and
		 * returns @a PolylineOnSphere objects instead of @a GeometryOnSphere objects.
		 *
		 * The other overload of @a intersect_topological_sections basically calls
		 * @a get_polylines_for_intersection and this function internally.
		 *
		 * This method is useful when you need to know whether the two geometries are even
		 * intersectable (eg, consist of polylines and/or polygons).
		 * 
		 */
		boost::tuple<
				boost::optional<GPlatesMaths::PointOnSphere>/*intersection point*/, 
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type/*first_section_closest_segment*/,
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type/*second_section_closest_segment*/>
		intersect_topological_sections(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_reconstructed_geometry,
				const GPlatesMaths::PointOnSphere &first_section_reconstructed_reference_point,
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_reconstructed_geometry,
				const GPlatesMaths::PointOnSphere &second_section_reconstructed_reference_point);


		/**
		 * Returns geometries @a first_geometry and @a second_geometry as @a PolylineOnSphere
		 * geometries if they are intersectable, otherwise returns false.
		 *
		 * If either geometries is a point or multi-point then the geometries are
		 * not intersectable and false is returned. If the two geometries consist of polylines
		 * and/or polygons then the polygon geometries get converted to polylines before returning.
		 *
		 * The output of this function is designed to be used with
		 * @a PolylineIntersections::partition_intersecting_polylines.
		 */
		boost::optional<
			std::pair<
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> >
		get_polylines_for_intersection(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_section_geometry,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_section_geometry);


		/**
		 * Returns the closest segment (@a first_intersected_segment or
		 * @a second_intersected_segment) to @a section_reference_point.
		 *
		 * FIXME: if the reference point is not close enough to either segment
		 * (within a implementation-defined limit) then the first segment is returned.
		 */
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		find_closest_intersected_segment_to_reference_point(
				const GPlatesMaths::PointOnSphere &section_reference_point,
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_intersected_segment,
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_intersected_segment);


		
		/**
		 * A useful predicate fuction used to include only reconstructed feature geometries.
		 *
		 * Returns true if @a recon_geom is of type @a ReconstructedFeatureGeometry.
		 */
		bool
		include_only_reconstructed_feature_geometries(
				const GPlatesModel::ReconstructionGeometry::non_null_ptr_type &recon_geom);
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYINTERNALUTILS_H
