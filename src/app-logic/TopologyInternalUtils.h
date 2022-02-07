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

#include <set>
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QString>

#include "ReconstructedFeatureGeometry.h"
#include "ResolvedTopologicalGeometrySubSegment.h"
#include "TopologyGeometryType.h"

#include "maths/GeometryOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"
#include "model/TopLevelProperty.h"

#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/StructuralType.h"


namespace GPlatesAppLogic
{
	class ReconstructionTree;

	/**
	 * This namespace contains utilities that are used internally in topology-related code.
	 */
	namespace TopologyInternalUtils
	{
		/**
		 * Topological geometry property value types (topological line, polygon and network).
		 */
		typedef boost::variant<
				GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type,
				GPlatesPropertyValues::GpmlTopologicalPolygon::non_null_ptr_type,
				GPlatesPropertyValues::GpmlTopologicalNetwork::non_null_ptr_type>
						topological_geometry_property_value_type;


		/**
		 * Returns the topological geometry property value (topological line, polygon or network)
		 * at the specified reconstruction time (only applies if property value is time-dependent).
		 *
		 * Returns boost::none if the specified property is not topological.
		 */
		boost::optional<topological_geometry_property_value_type>
		get_topology_geometry_property_value(
				GPlatesModel::TopLevelProperty &property,
				const double &reconstruction_time = 0.0);

		boost::optional<topological_geometry_property_value_type>
		get_topology_geometry_property_value(
				const GPlatesModel::FeatureHandle::iterator &property,
				const double &reconstruction_time = 0.0);


		/**
		 * Determines the type of topological geometry property value.
		 *
		 * Returns boost::none if the specified property is not topological.
		 */
		boost::optional<GPlatesPropertyValues::StructuralType>
		get_topology_geometry_property_value_type(
				const GPlatesModel::TopLevelProperty &property);

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
		boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type>
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
		 * Inserts all topological section features referenced by the topological feature
		 * @a topology_feature_ref (which can be a topological line, boundary or network).
		 *
		 * If @a topology_geometry_type is specified then only matching topology geometries in
		 * @a topology_feature_ref are searched.
		 *
		 * If @a reconstruction_time is specified then @a topology_feature_ref is only searched
		 * if it exists at the reconstruction time, in which case only those time-dependent topologies
		 * at the reconstruction time are considered (otherwise all topologies in a time-dependent
		 * sequence are considered).
		 */
		void
		find_topological_sections_referenced(
				std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
				const GPlatesModel::FeatureHandle::weak_ref &topology_feature_ref,
				boost::optional<TopologyGeometry::Type> topology_geometry_type = boost::none,
				boost::optional<double> reconstruction_time = boost::none);

		/**
		 * An overload of @a find_topological_sections_referenced accepting a topological
		 * feature collection instead of a single topological feature.
		 */
		void
		find_topological_sections_referenced(
				std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &topology_feature_collection_ref,
				boost::optional<TopologyGeometry::Type> topology_geometry_type = boost::none,
				boost::optional<double> reconstruction_time = boost::none);

		/**
		 * An overload of @a find_topological_sections_referenced accepting a vector of
		 * topological features instead of a feature collection.
		 */
		void
		find_topological_sections_referenced(
				std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topology_features,
				boost::optional<TopologyGeometry::Type> topology_geometry_type = boost::none,
				boost::optional<double> reconstruction_time = boost::none);


		/**
		 * Finds the reconstruction geometry for the geometry property referenced by the
		 * property delegate @a geometry_delegate.
		 *
		 * If @a reconstruct_handles is specified then an RG is returned only if it has a
		 * reconstruct handle in that set.
		 *
		 * NOTE: If more than one RG is found then the first found is returned.
		 *
		 * NOTE: The RGs must be generated before calling this function otherwise no RGs will be found.
		 * This includes reconstructing static geometries and resolving topological lines since both
		 * types of reconstruction geometry can be used as topological sections.
		 *
		 * Returns false if:
		 * - there are *no* RGs satisfying the specified constraints (reconstruct handles), or
		 * - there are multiple RGs satisfying the specified constraints (reconstruct handles)
		 *   and they came from *different* features (it's ok to have more than one feature with
		 *   same feature id but the constraints, reconstruct handles, must reduce the set of RGs
		 *   such that they come from only *one* feature).
		 *
		 * If there is no RG that is reconstructed from @a geometry_delegate, and satisfying the
		 * other constraints, then either:
		 *  - the reconstruction time @a reconstruction_time is outside the age range of the
		 *    delegate feature (this is OK and useful in some situations), or
		 *  - the reconstruction time @a reconstruction_time is inside the age range of the
		 *    delegate feature but the RG still cannot be found.
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
		 * NOTE: If more than one RG is found then the first found is returned.
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
		 *    delegate feature but the RG still cannot be found.
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
		 * Returns true if @a recon_geom can be used as a topological section for a resolved line.
		 *
		 * Essentially @a recon_geom must be a @a ReconstructedFeatureGeometry
		 * (but not a @a TopologyReconstructedFeatureGeometry).
		 */
		bool
		can_use_as_resolved_line_topological_section(
				const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom);

		
		/**
		 * Returns true if @a recon_geom can be used as a topological section for a resolved boundary.
		 *
		 * Essentially @a recon_geom must be a @a ReconstructedFeatureGeometry
		 * (but not a @a TopologyReconstructedFeatureGeometry) or a resolved topological line (@a ResolvedTopologicalLine).
		 */
		bool
		can_use_as_resolved_boundary_topological_section(
				const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom);

		
		/**
		 * Returns true if @a recon_geom can be used as a topological section for a resolved network.
		 *
		 * Essentially @a recon_geom must be a @a ReconstructedFeatureGeometry
		 * (but not a @a TopologyReconstructedFeatureGeometry) or a resolved topological line (@a ResolvedTopologicalLine).
		 */
		bool
		can_use_as_resolved_network_topological_section(
				const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom);
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYINTERNALUTILS_H
