/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "ReconstructedFeatureGeometry.h"
#include "TopologyGeometryType.h"

#include "maths/GeometryOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"

#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/StructuralType.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesPropertyValues
{
	class GpmlPropertyDelegate;
}

namespace GPlatesAppLogic
{
	class ReconstructionTree;

	/**
	 * This namespace contains utilities that are used internally in topology-related code.
	 */
	namespace TopologyInternalUtils
	{
		/**
		 * Determines the type of topological geometry property value.
		 *
		 * Returns boost::none if the specified property is not topological.
		 */
		boost::optional<GPlatesPropertyValues::StructuralType>
		get_topology_geometry_property_value_type(
				const GPlatesModel::FeatureHandle::const_iterator &property);


		/**
		 * Creates and returns a gpml topological section property value that references
		 * the geometry property @a geometry_property.
		 *
		 * Returns false if @a geometry_property is invalid or does not refer to one
		 * of @a GmlPoint, @a GmlLineString, @a GmlPolygon or @a GpmlTopologicalLine.
		 *
		 * FIXME: Don't return false for a @a GmlMultiPoint.
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
				const GPlatesModel::FeatureHandle::iterator &geometry_property,
				bool reverse_order = false);


		/**
		 * Creates and returns a gpml topological network interior that references
		 * the geometry property @a geometry_property.
		 *
		 * Returns false if @a geometry_property is invalid or does not refer to one of
		 * @a GmlPoint, @a GmlMultiPoint, @a GmlLineString, @a GmlPolygon or @a GpmlTopologicalLine.
		 *
		 * The @a geometry_property is a feature properties iterator but currently
		 * it we just get the property name from it and use that to create the
		 * property delegate - when a better method of property delegation is devised
		 * (one that works in the presence of multiple geometry properties with the
		 * same property name inside a single feature) then the properties iterator
		 * might refer to something that has a unique property id.
		 */
		boost::optional<GPlatesPropertyValues::GpmlTopologicalNetwork::Interior>
		create_gpml_topological_network_interior(
				const GPlatesModel::FeatureHandle::iterator &geometry_property);


		/**
		 * Create a geometry property delegate from a feature properties iterator and
		 * a property value type string (eg, "gml:LineString").
		 *
		 * Returns false if @a geometry_property is invalid.
		 *
		 * This function is used internally by @a create_gpml_topological_section.
		 */
		boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type>
		create_geometry_property_delegate(
				const GPlatesModel::FeatureHandle::iterator &geometry_property,
				const GPlatesPropertyValues::StructuralType &property_value_type);


		/**
		 * Retrieves a @a FeatureHandle weak reference associated with @a feature_id.
		 *
		 * If number of feature handles associated with @a feature_id is not one
		 * then a null feature handle reference is returned and an error message is
		 * output to the console.
		 */
		GPlatesModel::FeatureHandle::weak_ref
		resolve_feature_id(
				const GPlatesModel::FeatureId &feature_id);


		/**
		 * Finds the reconstruction geometry for the geometry property referenced by the
		 * property delegate @a geometry_delegate.
		 *
		 * If @a reconstruct_handles is specified then an RG is returned only if it has a
		 * reconstruct handle in that set.
		 *
		 * NOTE: If more than one RG is found then the first found is returned and a warning message
		 * is output to the console.
		 *
		 * NOTE: The RGs must be generated before calling this function otherwise no RGs will be found.
		 * This includes reconstructing static geometries and resolving topological lines since both
		 * types of reconstruction geometry can be used as topological sections.
		 *
		 * Returns false if:
		 * - there is *not* exactly *one* feature referencing the delegate feature id
		 *   (in this case an warning message is output to the console), or
		 * - there are *no* RGs satisfying the specified constraints (reconstruct handles).
		 *
		 * If there is no RG that is reconstructed from @a geometry_delegate, and satisfying the
		 * other constraints, then either:
		 *  - the reconstruction time @a reconstruction_time is outside the age range of the
		 *    delegate feature (this is OK and useful in some situations), or
		 *  - the reconstruction time @a reconstruction_time is inside the age range of the
		 *    delegate feature but the RG still cannot be found (in which case a warning is logged).
		 *
		 * If there is more than one RG referencing the delegate feature then either:
		 * - there are multiple geometry properties in the delegate feature that have the same
		 *   delegate property name, and/or
		 * - somewhere, in GPlates, more than one RG is being generated for the same
		 *   property iterator (might currently happen with flowlines - although we should
		 *   consider moving away from that behaviour), and/or
		 * - the same delegate feature is reconstructed more than once in different reconstruction
		 *   contexts (eg, multiple layers reconstructing the same feature).
		 *
		 * WARNING: Property delegates need to be improved because they do not uniquely
		 * identify a property since they use the property name and a feature can
		 * have multiple properties with the same name. Alternatively we could just reference
		 * all properties (in a feature) with the same property name?
		 */
		boost::optional<ReconstructionGeometry::non_null_ptr_type>
		find_topological_reconstruction_geometry(
				const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate,
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles = boost::none);


		/**
		 * Finds the reconstruction geometry for the geometry properties iterator @a geometry_property.
		 *
		 * If @a reconstruct_handles is specified then an RG is returned only if it has a
		 * reconstruct handle in that set.
		 *
		 * NOTE: If more than one RG is found then the first found is returned and a warning message
		 * is output to the console.
		 *
		 * NOTE: The RGs must be generated before calling this function otherwise no RGs will be found.
		 * This includes reconstructing static geometries and resolving topological lines since both
		 * types of reconstruction geometry can be used as topological sections.
		 *
		 * Returns false if:
		 * - @a geometry_property is invalid, or
		 * - there are *no* RGs satisfying the specified constraints (reconstruct handles).
		 *
		 * If there is no RG that is reconstructed from @a geometry_delegate, and satisfying the
		 * other constraints, then either:
		 *  - the reconstruction time @a reconstruction_time is outside the age range of the
		 *    delegate feature (this is OK and useful in some situations), or
		 *  - the reconstruction time @a reconstruction_time is inside the age range of the
		 *    delegate feature but the RG still cannot be found (in which case a warning is logged).
		 *
		 * If there is more than one RG referencing the feature (containing @a geometry_property) then either:
		 * - somewhere, in GPlates, more than one RG is being generated for the same
		 *   property iterator (might currently happen with flowlines - although we should
		 *   consider moving away from that behaviour), and/or
		 * - the same feature (containing @a geometry_property) is reconstructed more than once in
		 *   different reconstruction contexts (eg, multiple layers reconstructing the same feature).
		 */
		boost::optional<ReconstructionGeometry::non_null_ptr_type>
		find_topological_reconstruction_geometry(
				const GPlatesModel::FeatureHandle::iterator &geometry_property,
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles = boost::none);


		/**
		 * Returns the @a FiniteRotation obtained by looking up @a reconstruction_tree
		 * using the plate id from the "gpml:reconstructionPlateId" property of
		 * @a reconstruction_plateid_feature.
		 *
		 * If @a reconstruction_plateid_feature is not valid or
		 * no "gpml:reconstructionPlateId" property is found then false is returned.
		 */
		boost::optional<GPlatesMaths::FiniteRotation>
		get_finite_rotation(
				const GPlatesModel::FeatureHandle::weak_ref &reconstruction_plateid_feature,
				const ReconstructionTree &reconstruction_tree);


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
		 * If there is more than one intersection then only the first intersection is
		 * considered such that this function still divides the two input sections into
		 * (up to) two segments each (instead of several segments each).
		 * It does this by concatenating divided segments (of each section) before or after
		 * the intersection point into a single segment.
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
		 * Same as above overload of @a intersect_topological_sections but accepts and
		 * returns @a PolylineOnSphere objects instead of @a GeometryOnSphere objects.
		 *
		 * The above overload of @a intersect_topological_sections basically calls
		 * @a get_polylines_for_intersection and then calls this function internally.
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
		 * Intersects geometries @a first_section_reconstructed_geometry and
		 * @a second_section_reconstructed_geometry if they are intersectable
		 * (see @a get_polylines_for_intersection) and returns the two intersected
		 * head and tail segments for each section.
		 *
		 * If the geometries are not intersectable (eg, point/polygon) then false is returned.
		 * If the geometries are intersectable but do not intersect then false is returned.
		 *
		 * The returned @a PointOnSphere is the intersection point if the sections
		 * intersected.
		 *
		 * It's possible for the intersection to form a T-junction where
		 * one polyline is divided into two but the other meets at either
		 * its own start or end point - which will result in one of the sections
		 * returning a head and tail while the other returning either a head or a tail
		 * depending on whether the T-junction is a regular T or an upside-down T respectively.
		 *
		 * Or even a V-junction where each polyline meets at one of its endpoints
		 * (either begin or end). Again there are various possible combinations
		 * leading to different head/tail combinations in the returned result:
		 *    first section's start point meets second section's start point or,
		 *    first section's start point meets second section's end point or,
		 *    first section's end point meets second section's start point or,
		 *    first section's end point meets second section's end point.
		 *
		 * It is guaranteed that if there's an intersection then each section will
		 * return at least either a head or a tail.
		 *
		 * If there is more than one intersection then only the first intersection is
		 * considered such that this function still divides the two input sections into
		 * (up to) two segments each (instead of several segments each).
		 * It does this by concatenating divided segments (of each section) before or after
		 * the intersection point into a single segment.
		 */
		boost::optional<
			boost::tuple<
				/*intersection point*/
				GPlatesMaths::PointOnSphere,
				/*head_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
				/*tail_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
				/*head_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
				/*tail_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> > >
		intersect_topological_sections(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_section_reconstructed_geometry,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_section_reconstructed_geometry);


		/**
		 * Same as above overload of @a intersect_topological_sections but accepts and
		 * returns @a PolylineOnSphere objects instead of @a GeometryOnSphere objects.
		 *
		 * The above overload of @a intersect_topological_sections basically calls
		 * @a get_polylines_for_intersection and then calls this function internally.
		 *
		 * This method is useful when you need to know whether the two geometries are even
		 * intersectable (eg, consist of polylines and/or polygons).
		 * 
		 */
		boost::optional<
			boost::tuple<
				/*intersection point*/
				GPlatesMaths::PointOnSphere,
				/*head_first_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
				/*tail_first_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
				/*head_second_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
				/*tail_second_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > >
		intersect_topological_sections(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_reconstructed_geometry,
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_reconstructed_geometry);


		/**
		 * Intersects geometries @a first_section_reconstructed_geometry and
		 * @a second_section_reconstructed_geometry if they are intersectable
		 * (see @a get_polylines_for_intersection) allowing up to two intersections
		 * and returns optional head, tail and middle segments for each section.
		 *
		 * If the geometries are not intersectable (eg, point/polygon) then false is returned.
		 * If the geometries are intersectable but do not intersect then false is returned.
		 *
		 * The first @a PointOnSphere returned is if there's one intersection.
		 * The second optional @a PointOnSphere returned is if there's a second intersection.
		 *
		 * The middle segments will be null unless there is a second intersection.
		 *
		 * If there are two intersections then it's possible to form a circle where
		 * the endpoints of both sections meet - which will result in the head and tail segments
		 * of each section returning null. There are various other combinations that involve
		 * some head and tail segments being non-null all the way to the general case of
		 * all head, tail and middle segments being non-null.
		 *
		 * If there is one intersection it is guaranteed that each section will
		 * return at least either a head or a tail segment.
		 *
		 * If there are two intersections it is guaranteed that each section will
		 * return at least a middle segment.
		 *
		 * If there are more than two intersections then only the first two intersections
		 * are considered such that this function still divides the two input sections into
		 * (up to) three segments each (instead of several segments each).
		 * It does this by concatenating divided segments (of each section) before the first
		 * intersection point or after the second intersection point into a single segment.
		 */
		boost::optional<
			boost::tuple<
				/*first intersection point*/
				GPlatesMaths::PointOnSphere,
				/*optional second intersection point*/
				boost::optional<GPlatesMaths::PointOnSphere>,
				/*optional info if two intersections and middle segments form a cycle*/
				boost::optional<bool>,
				/*optional head_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
				/*optional middle_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
				/*optional tail_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
				/*optional head_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
				/*optional middle_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
				/*optional tail_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> > >
		intersect_topological_sections_allowing_two_intersections(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_section_reconstructed_geometry,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_section_reconstructed_geometry);


		/**
		 * Same as above overload of @a intersect_topological_sections_allowing_two_intersections
		 * but accepts and returns @a PolylineOnSphere objects instead of @a GeometryOnSphere objects.
		 *
		 * The above overload of @a intersect_topological_sections_allowing_two_intersections
		 * basically calls @a get_polylines_for_intersection and then calls this function internally.
		 */
		boost::optional<
			boost::tuple<
				/*first intersection point*/
				GPlatesMaths::PointOnSphere,
				/*optional second intersection point*/
				boost::optional<GPlatesMaths::PointOnSphere>,
				/*optional info if two intersections and middle segments form a cycle*/
				boost::optional<bool>,
				/*optional head_first_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
				/*optional middle_first_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
				/*optional tail_first_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
				/*optional head_second_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
				/*optional middle_second_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
				/*optional tail_second_section*/
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > >
		intersect_topological_sections_allowing_two_intersections(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_reconstructed_geometry,
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_reconstructed_geometry);


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
		 * Returns true if @a recon_geom can be used as a topological section for a resolved line.
		 *
		 * Essentially @a recon_geom must be a @a ReconstructedFeatureGeometry.
		 */
		bool
		can_use_as_resolved_line_topological_section(
				const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom);

		
		/**
		 * Returns true if @a recon_geom can be used as a topological section for a resolved boundary.
		 *
		 * Essentially @a recon_geom must be a @a ReconstructedFeatureGeometry or a
		 * resolved topological line (@a ResolvedTopologicalGeometry with a *polyline* geometry).
		 */
		bool
		can_use_as_resolved_boundary_topological_section(
				const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom);

		
		/**
		 * Returns true if @a recon_geom can be used as a topological section for a resolved network.
		 *
		 * Essentially @a recon_geom must be a @a ReconstructedFeatureGeometry or a
		 * resolved topological line (@a ResolvedTopologicalGeometry with a *polyline* geometry).
		 */
		bool
		can_use_as_resolved_network_topological_section(
				const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom);
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYINTERNALUTILS_H
