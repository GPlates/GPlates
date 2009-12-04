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

#include <vector>

#include "TopologyUtils.h"

#include "ReconstructionGeometryUtils.h"
#include "TopologyResolver.h"

#include "AppLogicUtils.h"

#include "maths/PointInPolygon.h"

#include "model/Reconstruction.h"

#include "utils/Profile.h"

#if 0
	#define DEBUG_POINT_IN_POLYGON
#endif


namespace GPlatesAppLogic
{
	namespace TopologyUtils
	{
		/**
		 * Associates a @a ResolvedTopologicalGeometry with its polygon structure
		 * optimised for point-in-polygon tests.
		 */
		struct ResolvedGeometryForPointInclusion
		{
			ResolvedGeometryForPointInclusion(
					const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_geometry_,
					const GPlatesMaths::PointInPolygon::optimised_polygon_type &optimised_polygon_) :
				resolved_topological_geometry(resolved_topological_geometry_),
				optimised_polygon(optimised_polygon_)
			{  }


			const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_geometry;
			GPlatesMaths::PointInPolygon::optimised_polygon_type optimised_polygon;
		};


		/**
		 * A sequence of @a ResolvedGeometryForPointInclusion objects.
		 */
		class ResolvedGeometriesForPointInclusionQuery
		{
		public:
			typedef std::vector<ResolvedGeometryForPointInclusion> resolved_geom_seq_type;

			resolved_geom_seq_type d_resolved_geoms;
		};
	}
}


void
GPlatesAppLogic::TopologyUtils::resolve_topologies(
		const double &reconstruction_time,
		GPlatesModel::Reconstruction &reconstruction,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				topological_features_collection)
{
	PROFILE_FUNC();

	TopologyResolver topology_resolver(
			reconstruction_time, reconstruction);

	AppLogicUtils::visit_feature_collections(
		topological_features_collection.begin(),
		topological_features_collection.end(),
		topology_resolver);
}


GPlatesAppLogic::TopologyUtils::resolved_geometries_for_point_inclusion_query_type
GPlatesAppLogic::TopologyUtils::query_resolved_topologies_for_testing_point_inclusion(
		GPlatesModel::Reconstruction &reconstruction)
{
	// First get all ResolvedTopologicalGeometry objects in the reconstruction.
	resolved_topological_geometry_seq_type all_resolved_geoms;
	ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction.geometries().begin(),
				reconstruction.geometries().end(),
				all_resolved_geoms);

	// Query structure to return to the caller.
	resolved_geometries_for_point_inclusion_query_type resolved_geometries_query;

	if (all_resolved_geoms.empty())
	{
		// Return empty query.
		return resolved_geometries_query;
	}

	// Create a new query to be filled.
	resolved_geometries_query.reset(new ResolvedGeometriesForPointInclusionQuery());

	// Reserve memory.
	resolved_geometries_query->d_resolved_geoms.reserve(all_resolved_geoms.size());

	// Iterate through the resolved topological geometries and
	// generate polygons optimised for point-in-polygon tests.
	resolved_topological_geometry_seq_type::const_iterator rtg_iter = all_resolved_geoms.begin();
	resolved_topological_geometry_seq_type::const_iterator rtg_end = all_resolved_geoms.end();
	for ( ; rtg_iter != rtg_end; ++rtg_iter)
	{
		const GPlatesModel::ResolvedTopologicalGeometry *rtg = *rtg_iter;

		const ResolvedGeometryForPointInclusion resolved_geom_for_point_inclusion(
				rtg,
				GPlatesMaths::PointInPolygon::create_optimised_polygon(
						rtg->resolved_topology_geometry()));

		resolved_geometries_query->d_resolved_geoms.push_back(resolved_geom_for_point_inclusion);
	}

	return resolved_geometries_query;
}


bool
GPlatesAppLogic::TopologyUtils::find_resolved_topologies_containing_point(
		resolved_topological_geometry_seq_type &resolved_topological_geoms_containing_point,
		const GPlatesMaths::PointOnSphere &point,
		const resolved_geometries_for_point_inclusion_query_type &resolved_geoms_query)
{
	// Return early if query does not exist.
	if (!resolved_geoms_query)
	{
		return false;
	}

	bool was_point_contained_by_any_polygons = false;

	// Iterate through the resolved geometries and see which ones (polygon) contain the point.
	ResolvedGeometriesForPointInclusionQuery::resolved_geom_seq_type::const_iterator rtg_iter =
			resolved_geoms_query->d_resolved_geoms.begin();
	ResolvedGeometriesForPointInclusionQuery::resolved_geom_seq_type::const_iterator rtg_end =
			resolved_geoms_query->d_resolved_geoms.end();
	for ( ; rtg_iter != rtg_end; ++rtg_iter)
	{
		const ResolvedGeometryForPointInclusion &rtg = *rtg_iter;

#ifdef DEBUG_POINT_IN_POLYGON
		// Get reconstructionPlateId property value
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

		if ( GPlatesFeatureVisitors::get_property_value(
			rtg.resolved_topological_geometry->get_feature_ref(), property_name, recon_plate_id ) )
		{
			std::cout 
				<< " TopologyUtils::find_resolved_topologies_containing_point(): " 
				<< " reconstructionPlateId = " << recon_plate_id->value() 
				<< " has " << rtg.resolved_topological_geometry->resolved_topology_geometry()
						->number_of_segments() + 1 << " vertices." 
				<< std::endl;
		}
#endif

		// Test the point against the resolved topology polygon.
		GPlatesMaths::PointInPolygon::PointInPolygonResult point_in_poly_result =
				GPlatesMaths::PointInPolygon::test_point_in_polygon(
						point,
						rtg.optimised_polygon);

		if (point_in_poly_result != GPlatesMaths::PointInPolygon::POINT_OUTSIDE_POLYGON)
		{
			was_point_contained_by_any_polygons = true;

			resolved_topological_geoms_containing_point.push_back(rtg.resolved_topological_geometry);
		}
	}

	return was_point_contained_by_any_polygons;
}
